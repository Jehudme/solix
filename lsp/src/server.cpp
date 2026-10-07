#include "solix/lsp/server.hpp"
#include "solix/compilation.hpp"
#include "solix/path_utils.hpp"
#include "dependency_resolver.hpp"
#include "processes/lexer.hpp"
#include "processes/parser.hpp"
#include "processes/binder.hpp"
#include "utilities/diagnostic.hpp"
#include <fstream>
#include <sstream>

namespace solix::lsp {

LspServer::LspServer(std::istream& in, std::ostream& out)
    : transport_(in, out) {}

int LspServer::run() {
    while (!should_exit_) {
        auto msg = transport_.read_message();
        if (!msg.has_value()) {
            break; // EOF or stream error
        }
        process_message(msg.value());
    }
    return exit_code_;
}

void LspServer::process_message(const nlohmann::json& msg) {
    if (!msg.is_object()) return;

    bool has_id = msg.contains("id") && !msg["id"].is_null();
    std::string method = msg.value("method", "");
    nlohmann::json params = msg.value("params", nlohmann::json::object());

    if (method == "initialize") {
        if (has_id) {
            handle_initialize(msg["id"], params);
        }
    } else if (method == "initialized") {
        is_initialized_ = true;
    } else if (method == "shutdown") {
        if (has_id) {
            handle_shutdown(msg["id"]);
        }
    } else if (method == "exit") {
        handle_exit();
    } else if (method == "textDocument/didOpen") {
        handle_did_open(params);
    } else if (method == "textDocument/didChange") {
        handle_did_change(params);
    } else if (method == "textDocument/didClose") {
        handle_did_close(params);
    } else if (method == "textDocument/didSave") {
        handle_did_save(params);
    } else if (method == "textDocument/definition") {
        if (has_id) handle_definition(msg["id"], params);
    } else if (method == "textDocument/typeDefinition") {
        if (has_id) handle_type_definition(msg["id"], params);
    } else if (method == "textDocument/hover") {
        if (has_id) handle_hover(msg["id"], params);
    } else if (method == "textDocument/completion") {
        if (has_id) handle_completion(msg["id"], params);
    } else if (method == "textDocument/signatureHelp") {
        if (has_id) handle_signature_help(msg["id"], params);
    } else if (method == "textDocument/documentSymbol") {
        if (has_id) handle_document_symbol(msg["id"], params);
    } else {
        if (has_id) {
            // Method not found (-32601)
            transport_.send_error(msg["id"], -32601, "Method not implemented: " + method);
        }
    }
}

void LspServer::set_workspace_root(const std::filesystem::path& root) {
    workspace_root_ = std::filesystem::absolute(root).lexically_normal();
    manifest_path_ = workspace_root_ / "solix.json";
    load_project_dependencies();
}

void LspServer::load_project_dependencies() {
    dependency_roots_.clear();
    manifest_ = nlohmann::json();

    if (!std::filesystem::exists(manifest_path_)) {
        return;
    }

    std::ifstream mf(manifest_path_);
    if (!mf.is_open()) return;

    try {
        mf >> manifest_;
    } catch (...) {
        return;
    }
}

void LspServer::handle_initialize(const nlohmann::json& id, const nlohmann::json& params) {
    std::string root_uri = params.value("rootUri", "");
    std::string root_path = params.value("rootPath", "");

    if (!root_uri.empty()) {
        set_workspace_root(uri_to_path(root_uri));
    } else if (!root_path.empty()) {
        set_workspace_root(root_path);
    } else if (params.contains("workspaceFolders") && params["workspaceFolders"].is_array() && !params["workspaceFolders"].empty()) {
        std::string first_uri = params["workspaceFolders"][0].value("uri", "");
        if (!first_uri.empty()) {
            set_workspace_root(uri_to_path(first_uri));
        }
    } else {
        set_workspace_root(std::filesystem::current_path());
    }

    nlohmann::json result = {
        {"capabilities", {
            {"textDocumentSync", 1}, // 1 = Full sync
            {"hoverProvider", true},
            {"definitionProvider", true},
            {"typeDefinitionProvider", true},
            {"completionProvider", {
                {"resolveProvider", false},
                {"triggerCharacters", {".", "::"}}
            }},
            {"signatureHelpProvider", {
                {"triggerCharacters", {"(", ","}}
            }},
            {"documentSymbolProvider", true}
        }},
        {"serverInfo", {
            {"name", "solix-lsp"},
            {"version", "0.1.0"}
        }}
    };

    transport_.send_response(id, result);
}

void LspServer::handle_shutdown(const nlohmann::json& id) {
    is_shutdown_ = true;
    transport_.send_response(id, nullptr);
}

void LspServer::handle_exit() {
    should_exit_ = true;
    exit_code_ = is_shutdown_ ? 0 : 1;
}

void LspServer::handle_did_open(const nlohmann::json& params) {
    if (!params.contains("textDocument") || !params["textDocument"].is_object()) return;
    const auto& doc = params["textDocument"];
    std::string uri = doc.value("uri", "");
    int version = doc.value("version", 0);
    std::string text = doc.value("text", "");

    docs_.open_document(uri, version, text);
    analyze_document(uri);
}

void LspServer::handle_did_change(const nlohmann::json& params) {
    if (!params.contains("textDocument") || !params["textDocument"].is_object()) return;
    const auto& doc = params["textDocument"];
    std::string uri = doc.value("uri", "");
    int version = doc.value("version", 0);

    if (params.contains("contentChanges") && params["contentChanges"].is_array() && !params["contentChanges"].empty()) {
        // With TextDocumentSyncKind::Full, the last element or only element contains full text
        std::string text = params["contentChanges"].back().value("text", "");
        docs_.update_document(uri, version, text);
        analyze_document(uri);
    }
}

void LspServer::handle_did_close(const nlohmann::json& params) {
    if (!params.contains("textDocument") || !params["textDocument"].is_object()) return;
    std::string uri = params["textDocument"].value("uri", "");
    docs_.close_document(uri);

    // Clear diagnostics on close
    nlohmann::json empty_diag_params = {
        {"uri", uri},
        {"diagnostics", nlohmann::json::array()}
    };
    transport_.send_notification("textDocument/publishDiagnostics", empty_diag_params);
}

void LspServer::handle_did_save(const nlohmann::json& params) {
    if (!params.contains("textDocument") || !params["textDocument"].is_object()) return;
    std::string uri = params["textDocument"].value("uri", "");
    analyze_document(uri);
}

void LspServer::analyze_document(const std::string& target_uri) {
    std::string target_file_path = uri_to_path(target_uri);
    std::filesystem::path target_p(target_file_path);
    target_p = target_p.lexically_normal();

    CompilationOptions opts;
    opts.log_level = CompilationOptions::LogLevel::OFF;
    opts.flush_level = CompilationOptions::LogLevel::OFF;

    // 1. If solix.json exists, resolve project dependencies first
    if (!manifest_.is_null() && manifest_.is_object()) {
        std::string profile_name = "debug";
        nlohmann::json active_profile = nlohmann::json::object();
        if (manifest_.contains("profiles") && manifest_["profiles"].is_object() && manifest_["profiles"].contains(profile_name)) {
            active_profile = manifest_["profiles"][profile_name];
        }

        solix::cli::DependencyManager dep_mgr;
        dep_mgr.resolve_all(manifest_, workspace_root_, active_profile, opts, &dependency_roots_);
    }

    // 2. Ingest all open documents from the DocumentStore (in-memory overrides disk)
    auto open_docs = docs_.get_all_open_documents();
    for (const auto& [doc_uri, doc_text] : open_docs) {
        std::string local_path = uri_to_path(doc_uri);
        opts.sources[local_path] = doc_text;
    }

    // Ensure the target document itself is present in opts.sources
    auto doc_text_opt = docs_.get_document_text(target_uri);
    if (doc_text_opt.has_value()) {
        opts.sources[target_p.string()] = doc_text_opt.value();
    } else if (std::filesystem::exists(target_p)) {
        std::ifstream f(target_p);
        if (f.is_open()) {
            std::stringstream buf;
            buf << f.rdbuf();
            opts.sources[target_p.string()] = buf.str();
        }
    }

    if (opts.sources.empty()) {
        return;
    }

    // Allocate persistent options so Token::source and Node::source pointers remain valid
    last_opts_ = std::make_shared<CompilationOptions>(std::move(opts));

    // 3. Run frontend analysis pipeline: Lexer -> Parser -> Binder
    CompilationContext context(*last_opts_);
    context.diagnostic = std::make_unique<solix::Diagnostic>(context);

    try {
        Lexer lexer(context, "Lexer");
        lexer.execute();

        if (!context.diagnostic->has_errors()) {
            Parser parser(context, "Parser");
            parser.execute();

            if (!context.diagnostic->has_errors()) {
                Binder binder(context, "Binder");
                try {
                    binder.execute();
                } catch (...) {
                    // Binder catch
                }
            }
        }
    } catch (...) {
        // Ignore fatal exceptions, diagnostic reports already captured
    }

    // 4. Map collected reports into LSP diagnostics grouped by source file URI
    std::unordered_map<std::string, std::vector<Diagnostic>> diags_by_uri;
    diags_by_uri[target_uri] = {}; // Ensure target URI key exists

    for (const auto& rep : context.diagnostic->get_reports()) {
        std::string rep_source = rep.source_path;
        if (rep_source.empty()) {
            rep_source = target_p.string();
        }

        std::string rep_uri = path_to_uri(rep_source);

        Diagnostic d;
        d.severity = (rep.severity == ReportSeverity::ERROR) ? 1 : 2;
        d.code = rep.code.empty() ? "E_SOLIX" : rep.code;
        d.source = "solix";
        d.message = rep.message;

        // LSP is 0-indexed; Solix lines and columns are 1-indexed
        int start_line = std::max(0, rep.line - 1);
        int start_col = std::max(0, rep.column - 1);

        int end_line = (rep.end_line > 0) ? std::max(0, rep.end_line - 1) : start_line;
        int end_col = (rep.end_column > 0) ? std::max(0, rep.end_column - 1) : (start_col + 1);

        if (end_line < start_line || (end_line == start_line && end_col < start_col)) {
            end_line = start_line;
            end_col = start_col + 1;
        }

        d.range.start.line = start_line;
        d.range.start.character = start_col;
        d.range.end.line = end_line;
        d.range.end.character = end_col;

        diags_by_uri[rep_uri].push_back(d);
    }

    // Cache compilation context and spatial index
    last_context_ = std::make_shared<CompilationContext>(*last_opts_);
    // Copy nodes and symbols
    last_context_->tokens = std::move(context.tokens);
    last_context_->nodes = std::move(context.nodes);
    last_context_->symbols = std::move(context.symbols);
    last_context_->string_pool = std::move(context.string_pool);

    spatial_index_.clear();
    for (const auto& [src, nodes] : last_context_->nodes) {
        std::string src_path;
        if (std::holds_alternative<std::filesystem::path>(src)) {
            src_path = std::get<std::filesystem::path>(src).string();
        } else {
            src_path = std::get<std::string>(src);
        }
        spatial_index_.index_source(src_path, nodes);
    }

    // 5. Send textDocument/publishDiagnostics for target_uri and any open files
    for (const auto& [uri, diags] : diags_by_uri) {
        nlohmann::json diag_list = nlohmann::json::array();
        for (const auto& d : diags) {
            nlohmann::json dj = d;
            diag_list.push_back(dj);
        }

        nlohmann::json notif_params = {
            {"uri", uri},
            {"diagnostics", diag_list}
        };
        transport_.send_notification("textDocument/publishDiagnostics", notif_params);
    }
}

namespace {

Range node_to_lsp_range(Node* node) {
    Range r;
    if (!node) return r;
    int s_line = std::max(0, static_cast<int>(node->line) - 1);
    int s_col = std::max(0, static_cast<int>(node->column) - 1);
    int e_line = (node->end_line > 0) ? std::max(0, static_cast<int>(node->end_line) - 1) : s_line;
    int e_col = (node->end_column > 0) ? std::max(0, static_cast<int>(node->end_column) - 1) : (s_col + 1);
    r.start.line = s_line;
    r.start.character = s_col;
    r.end.line = e_line;
    r.end.character = e_col;
    return r;
}

Location make_location_from_node(Node* node, CompilationContext* ctx = nullptr) {
    Location loc;
    if (!node) return loc;

    std::string path;
    if (node->source && !node->source->valueless_by_exception()) {
        try {
            if (std::holds_alternative<std::filesystem::path>(*node->source)) {
                path = std::get<std::filesystem::path>(*node->source).string();
            } else if (std::holds_alternative<std::string>(*node->source)) {
                path = std::get<std::string>(*node->source);
            }
        } catch (...) {}
    }

    if (path.empty() && ctx) {
        // Fallback: search which compilation unit owns this AST node
        for (const auto& [src, nodes] : ctx->nodes) {
            bool found = false;
            for (const auto& root : nodes) {
                if (root.get() == node) {
                    found = true;
                    break;
                }
                for (const auto& child : root->children) {
                    if (child.get() == node) {
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
            if (found) {
                if (std::holds_alternative<std::filesystem::path>(src)) {
                    path = std::get<std::filesystem::path>(src).string();
                } else if (std::holds_alternative<std::string>(src)) {
                    path = std::get<std::string>(src);
                }
                break;
            }
        }
    }

    loc.uri = path_to_uri(path);
    loc.range = node_to_lsp_range(node);
    return loc;
}

std::string format_hover_for_node(Node* node, CompilationContext* ctx) {
    if (!node) return "";
    std::string md;

    switch (node->node_type) {
        case NodeType::VAR_DECL: {
            auto* v = static_cast<VariableDeclaration*>(node);
            md = "```solix\n" + (v->is_const ? std::string("const ") : "") + v->type_info.to_string() + " " + v->var_name + "\n```";
            break;
        }
        case NodeType::FIELD_DECL: {
            auto* f = static_cast<FieldDeclaration*>(node);
            md = "```solix\n" + (f->is_static ? std::string("static ") : "") + f->type_info.to_string() + " " + f->field_name + "\n```";
            break;
        }
        case NodeType::METHOD_DECL: {
            auto* m = static_cast<MethodDeclaration*>(node);
            md = "```solix\n" + (m->is_static ? std::string("static ") : "") + m->return_type.to_string() + " " + m->method_name + "(";
            for (size_t i = 0; i < m->parameters.size(); ++i) {
                if (i > 0) md += ", ";
                md += m->parameters[i]->type_info.to_string() + " " + m->parameters[i]->var_name;
            }
            md += ")\n```";
            break;
        }
        case NodeType::CLASS_DECL: {
            auto* c = static_cast<ClassDeclaration*>(node);
            md = "```solix\n" + (c->is_interface ? std::string("interface ") : std::string("class ")) + c->class_name + "\n```";
            break;
        }
        case NodeType::ENUM_DECL: {
            auto* e = static_cast<EnumDeclaration*>(node);
            md = "```solix\nenum " + e->enum_name + "\n```";
            break;
        }
        case NodeType::ALIAS_STMT: {
            auto* a = static_cast<AliasStatement*>(node);
            md = "```solix\nalias " + a->alias_name + " = " + a->target_type.to_string() + "\n```";
            break;
        }
        case NodeType::IDENTIFIER: {
            auto* id = static_cast<IdentifierNode*>(node);
            if (id->resolved_declaration) {
                return format_hover_for_node(id->resolved_declaration, ctx);
            }
            if (!id->expression_type.name.empty()) {
                md = "```solix\n" + id->expression_type.to_string() + " " + id->name + "\n```";
            }
            break;
        }
        default: {
            if (!node->expression_type.name.empty()) {
                md = "```solix\n" + node->expression_type.to_string() + "\n```";
            }
            break;
        }
    }
    return md;
}

} // namespace

void LspServer::handle_definition(const nlohmann::json& id, const nlohmann::json& params) {
    if (!params.contains("textDocument") || !params.contains("position")) {
        transport_.send_response(id, nullptr);
        return;
    }

    std::string uri = params["textDocument"].value("uri", "");
    int line = params["position"].value("line", 0) + 1;       // Convert to 1-indexed
    int character = params["position"].value("character", 0) + 1;

    std::string file_path = uri_to_path(uri);
    Node* hit = spatial_index_.find_node_at(file_path, line, character);

    if (!hit) {
        transport_.send_response(id, nullptr);
        return;
    }

    Node* target = hit->resolved_declaration;
    if (!target && hit->parent && hit->parent->resolved_declaration) {
        target = hit->parent->resolved_declaration;
    }
    if (!target && hit->node_type == NodeType::MEMBER_ACCESS) {
        auto* mem = static_cast<MemberAccessExpression*>(hit);
        if (mem->parent && mem->parent->resolved_declaration) {
            target = mem->parent->resolved_declaration;
        }
    }
    if (!target && hit->node_type == NodeType::METHOD_CALL) {
        auto* mc = static_cast<MethodCallExpression*>(hit);
        if (mc->callee && mc->callee->resolved_declaration) {
            target = mc->callee->resolved_declaration;
        }
    }
    if (!target) {
        // If hit is itself a declaration, it can point to itself
        if (hit->node_type == NodeType::VAR_DECL || hit->node_type == NodeType::METHOD_DECL ||
            hit->node_type == NodeType::CLASS_DECL || hit->node_type == NodeType::FIELD_DECL ||
            hit->node_type == NodeType::ENUM_DECL || hit->node_type == NodeType::ALIAS_STMT) {
            target = hit;
        }
    }

    if (!target) {
        transport_.send_response(id, nullptr);
        return;
    }

    Location loc = make_location_from_node(target, last_context_.get());
    nlohmann::json loc_json = loc;
    transport_.send_response(id, loc_json);
}

void LspServer::handle_type_definition(const nlohmann::json& id, const nlohmann::json& params) {
    if (!params.contains("textDocument") || !params.contains("position")) {
        transport_.send_response(id, nullptr);
        return;
    }

    std::string uri = params["textDocument"].value("uri", "");
    int line = params["position"].value("line", 0) + 1;
    int character = params["position"].value("character", 0) + 1;

    std::string file_path = uri_to_path(uri);
    Node* hit = spatial_index_.find_node_at(file_path, line, character);

    if (!hit) {
        transport_.send_response(id, nullptr);
        return;
    }

    std::string type_name = hit->expression_type.name;
    if (hit->node_type == NodeType::VAR_DECL) {
        type_name = static_cast<VariableDeclaration*>(hit)->type_info.name;
    } else if (hit->node_type == NodeType::FIELD_DECL) {
        type_name = static_cast<FieldDeclaration*>(hit)->type_info.name;
    } else if (type_name.empty() && hit->resolved_declaration) {
        if (hit->resolved_declaration->node_type == NodeType::VAR_DECL) {
            type_name = static_cast<VariableDeclaration*>(hit->resolved_declaration)->type_info.name;
        } else if (hit->resolved_declaration->node_type == NodeType::FIELD_DECL) {
            type_name = static_cast<FieldDeclaration*>(hit->resolved_declaration)->type_info.name;
        } else {
            type_name = hit->resolved_declaration->expression_type.name;
        }
    }

    if (type_name.empty() || !last_context_) {
        transport_.send_response(id, nullptr);
        return;
    }

    // Find class or enum matching type_name
    Node* type_decl = nullptr;
    for (const auto& [name, sym] : last_context_->symbols) {
        if (sym && (sym->node_type == NodeType::CLASS_DECL || sym->node_type == NodeType::ENUM_DECL)) {
            if (sym->mangled_name == type_name || name == type_name) {
                type_decl = sym;
                break;
            }
        }
    }
    if (!type_decl) {
        for (const auto& [src, nodes] : last_context_->nodes) {
            for (const auto& n : nodes) {
                if (!n) continue;
                if (n->node_type == NodeType::CLASS_DECL) {
                    auto* c = static_cast<ClassDeclaration*>(n.get());
                    if (c->class_name == type_name || c->mangled_name == type_name) {
                        type_decl = c;
                        break;
                    }
                } else if (n->node_type == NodeType::ENUM_DECL) {
                    auto* e = static_cast<EnumDeclaration*>(n.get());
                    if (e->enum_name == type_name || e->mangled_name == type_name) {
                        type_decl = e;
                        break;
                    }
                }
            }
            if (type_decl) break;
        }
    }

    if (!type_decl) {
        transport_.send_response(id, nullptr);
        return;
    }

    Location loc = make_location_from_node(type_decl, last_context_.get());
    nlohmann::json loc_json = loc;
    transport_.send_response(id, loc_json);
}

void LspServer::handle_hover(const nlohmann::json& id, const nlohmann::json& params) {
    if (!params.contains("textDocument") || !params.contains("position")) {
        transport_.send_response(id, nullptr);
        return;
    }

    std::string uri = params["textDocument"].value("uri", "");
    int line = params["position"].value("line", 0) + 1;
    int character = params["position"].value("character", 0) + 1;

    std::string file_path = uri_to_path(uri);
    Node* hit = spatial_index_.find_node_at(file_path, line, character);

    if (!hit) {
        transport_.send_response(id, nullptr);
        return;
    }

    std::string md_text = format_hover_for_node(hit, last_context_.get());
    if (md_text.empty()) {
        transport_.send_response(id, nullptr);
        return;
    }

    Hover hover;
    hover.contents.kind = "markdown";
    hover.contents.value = md_text;

    int s_line = std::max(0, static_cast<int>(hit->line) - 1);
    int s_col = std::max(0, static_cast<int>(hit->column) - 1);
    int e_line = (hit->end_line > 0) ? std::max(0, static_cast<int>(hit->end_line) - 1) : s_line;
    int e_col = (hit->end_column > 0) ? std::max(0, static_cast<int>(hit->end_column) - 1) : (s_col + 1);

    Range r;
    r.start.line = s_line;
    r.start.character = s_col;
    r.end.line = e_line;
    r.end.character = e_col;
    hover.range = r;

    nlohmann::json hover_json = hover;
    transport_.send_response(id, hover_json);
}

void LspServer::handle_document_symbol(const nlohmann::json& id, const nlohmann::json& params) {
    if (!last_context_ || !params.contains("textDocument")) {
        transport_.send_response(id, nlohmann::json::array());
        return;
    }

    std::string uri = params["textDocument"].value("uri", "");
    std::string file_path = uri_to_path(uri);

    std::vector<DocumentSymbol> symbols;

    for (const auto& [src, nodes] : last_context_->nodes) {
        std::string src_path;
        if (std::holds_alternative<std::filesystem::path>(src)) {
            src_path = std::get<std::filesystem::path>(src).string();
        } else {
            src_path = std::get<std::string>(src);
        }
        if (src_path != file_path) continue;

        for (const auto& node : nodes) {
            if (!node) continue;
            if (node->node_type == NodeType::CLASS_DECL) {
                auto* cls = static_cast<ClassDeclaration*>(node.get());
                DocumentSymbol cls_sym;
                cls_sym.name = cls->class_name;
                cls_sym.kind = static_cast<int>(cls->is_interface ? SymbolKind::Interface : SymbolKind::Class);
                cls_sym.range = node_to_lsp_range(cls);
                cls_sym.selectionRange = cls_sym.range;

                for (const auto& child : cls->children) {
                    if (!child) continue;
                    if (child->node_type == NodeType::METHOD_DECL) {
                        auto* m = static_cast<MethodDeclaration*>(child.get());
                        DocumentSymbol m_sym;
                        m_sym.name = m->method_name;
                        m_sym.detail = m->return_type.to_string();
                        m_sym.kind = static_cast<int>(SymbolKind::Method);
                        m_sym.range = node_to_lsp_range(m);
                        m_sym.selectionRange = m_sym.range;
                        cls_sym.children.push_back(m_sym);
                    } else if (child->node_type == NodeType::FIELD_DECL) {
                        auto* f = static_cast<FieldDeclaration*>(child.get());
                        DocumentSymbol f_sym;
                        f_sym.name = f->field_name;
                        f_sym.detail = f->type_info.to_string();
                        f_sym.kind = static_cast<int>(SymbolKind::Field);
                        f_sym.range = node_to_lsp_range(f);
                        f_sym.selectionRange = f_sym.range;
                        cls_sym.children.push_back(f_sym);
                    } else if (child->node_type == NodeType::CONSTRUCTOR_DECL) {
                        auto* c = static_cast<ConstructorDeclaration*>(child.get());
                        DocumentSymbol c_sym;
                        c_sym.name = c->class_name;
                        c_sym.kind = static_cast<int>(SymbolKind::Constructor);
                        c_sym.range = node_to_lsp_range(c);
                        c_sym.selectionRange = c_sym.range;
                        cls_sym.children.push_back(c_sym);
                    }
                }
                symbols.push_back(cls_sym);
            } else if (node->node_type == NodeType::ENUM_DECL) {
                auto* en = static_cast<EnumDeclaration*>(node.get());
                DocumentSymbol en_sym;
                en_sym.name = en->enum_name;
                en_sym.kind = static_cast<int>(SymbolKind::Enum);
                en_sym.range = node_to_lsp_range(en);
                en_sym.selectionRange = en_sym.range;
                for (const auto& member_name : en->members) {
                    DocumentSymbol em_sym;
                    em_sym.name = member_name;
                    em_sym.kind = static_cast<int>(SymbolKind::EnumMember);
                    em_sym.range = en_sym.range;
                    em_sym.selectionRange = en_sym.range;
                    en_sym.children.push_back(em_sym);
                }
                symbols.push_back(en_sym);
            } else if (node->node_type == NodeType::METHOD_DECL) {
                auto* m = static_cast<MethodDeclaration*>(node.get());
                DocumentSymbol m_sym;
                m_sym.name = m->method_name;
                m_sym.detail = m->return_type.to_string();
                m_sym.kind = static_cast<int>(SymbolKind::Function);
                m_sym.range = node_to_lsp_range(m);
                m_sym.selectionRange = m_sym.range;
                symbols.push_back(m_sym);
            }
        }
    }

    nlohmann::json res = nlohmann::json::array();
    for (const auto& s : symbols) {
        nlohmann::json sj = s;
        res.push_back(sj);
    }
    transport_.send_response(id, res);
}

void LspServer::handle_completion(const nlohmann::json& id, const nlohmann::json& params) {
    if (!params.contains("textDocument") || !params.contains("position")) {
        CompletionList cl;
        transport_.send_response(id, cl);
        return;
    }

    std::string uri = params["textDocument"].value("uri", "");
    int line = params["position"].value("line", 0);           // 0-indexed
    int character = params["position"].value("character", 0); // 0-indexed

    auto doc_text_opt = docs_.get_document_text(uri);
    std::string current_line;
    if (doc_text_opt.has_value()) {
        std::istringstream stream(doc_text_opt.value());
        std::string l;
        int cur = 0;
        while (std::getline(stream, l)) {
            if (cur == line) {
                current_line = l;
                break;
            }
            cur++;
        }
    }

    std::string prefix = (character <= static_cast<int>(current_line.size())) 
                            ? current_line.substr(0, character) 
                            : current_line;

    // Check if this is a dot completion (e.g. "obj." or "obj.part")
    size_t last_dot = prefix.rfind('.');
    bool is_dot_access = false;
    std::string receiver_name;

    if (last_dot != std::string::npos) {
        // Ensure characters between last_dot and end of prefix are valid identifier characters
        bool valid_suffix = true;
        for (size_t i = last_dot + 1; i < prefix.size(); ++i) {
            char c = prefix[i];
            if (!std::isalnum(c) && c != '_') {
                valid_suffix = false;
                break;
            }
        }
        if (valid_suffix) {
            // Find receiver word ending right before last_dot
            size_t end_recv = last_dot;
            while (end_recv > 0 && std::isspace(prefix[end_recv - 1])) end_recv--;
            size_t start_recv = end_recv;
            while (start_recv > 0 && (std::isalnum(prefix[start_recv - 1]) || prefix[start_recv - 1] == '_')) {
                start_recv--;
            }
            if (end_recv > start_recv) {
                receiver_name = prefix.substr(start_recv, end_recv - start_recv);
                is_dot_access = true;
            }
        }
    }

    CompletionList comp_list;

    if (is_dot_access && !receiver_name.empty()) {
        std::string file_path = uri_to_path(uri);
        // Find receiver type via spatial index at receiver position
        int recv_col = static_cast<int>(last_dot); // 1-indexed column of the char before '.'
        Node* recv_node = spatial_index_.find_node_at(file_path, line + 1, recv_col);

        std::string target_class_name;
        if (recv_node) {
            if (recv_node->resolved_declaration) {
                Node* decl = recv_node->resolved_declaration;
                if (decl->node_type == NodeType::VAR_DECL) {
                    target_class_name = static_cast<VariableDeclaration*>(decl)->type_info.name;
                } else if (decl->node_type == NodeType::FIELD_DECL) {
                    target_class_name = static_cast<FieldDeclaration*>(decl)->type_info.name;
                } else if (decl->node_type == NodeType::CLASS_DECL) {
                    target_class_name = static_cast<ClassDeclaration*>(decl)->class_name;
                }
            } else if (!recv_node->expression_type.name.empty()) {
                target_class_name = recv_node->expression_type.name;
            }
        }

        // If spatial index did not directly resolve receiver, search context for local variable or class
        if (target_class_name.empty() && last_context_) {
            for (const auto& [src, nodes] : last_context_->nodes) {
                for (const auto& node : nodes) {
                    if (!node || node->node_type != NodeType::CLASS_DECL) continue;
                    auto* cd = static_cast<ClassDeclaration*>(node.get());
                    if (cd->class_name == receiver_name) {
                        target_class_name = cd->class_name;
                        break;
                    }
                    for (const auto& member : cd->children) {
                        if (!member || member->node_type != NodeType::METHOD_DECL) continue;
                        auto* md = static_cast<MethodDeclaration*>(member.get());
                        for (const auto& child : md->children) {
                            if (!child) continue;
                            auto check_var = [&](Node* n) {
                                if (n && n->node_type == NodeType::VAR_DECL) {
                                    auto* vd = static_cast<VariableDeclaration*>(n);
                                    if (vd->var_name == receiver_name) {
                                        target_class_name = vd->type_info.name;
                                    }
                                }
                            };
                            check_var(child.get());
                            for (const auto& nested : child->children) {
                                check_var(nested.get());
                            }
                        }
                    }
                }
            }
        }

        if (!target_class_name.empty() && last_context_) {
            // Find class declaration and populate members
            for (const auto& [src, nodes] : last_context_->nodes) {
                for (const auto& node : nodes) {
                    if (!node || node->node_type != NodeType::CLASS_DECL) continue;
                    auto* cd = static_cast<ClassDeclaration*>(node.get());
                    if (cd->class_name == target_class_name || cd->mangled_name == target_class_name) {
                        for (const auto& member : cd->children) {
                            if (!member) continue;
                            if (member->node_type == NodeType::METHOD_DECL) {
                                auto* md = static_cast<MethodDeclaration*>(member.get());
                                CompletionItem item;
                                item.label = md->method_name;
                                item.kind = static_cast<int>(CompletionItemKind::Method);
                                item.detail = md->return_type.to_string() + " " + md->method_name + "()";
                                item.insertText = md->method_name + "()";
                                comp_list.items.push_back(item);
                            } else if (member->node_type == NodeType::FIELD_DECL) {
                                auto* fd = static_cast<FieldDeclaration*>(member.get());
                                CompletionItem item;
                                item.label = fd->field_name;
                                item.kind = static_cast<int>(CompletionItemKind::Field);
                                item.detail = fd->type_info.to_string();
                                item.insertText = fd->field_name;
                                comp_list.items.push_back(item);
                            }
                        }
                    }
                }
            }
        }

        // Return completions for dot access (empty if unresolved)
        transport_.send_response(id, comp_list);
        return;
    }

    // General scope completion: Keywords, visible locals, class members, types
    static const std::vector<std::string> keywords = {
        "class", "interface", "enum", "struct", "public", "private", "protected", "internal",
        "static", "virtual", "override", "abstract", "native", "const",
        "if", "else", "while", "do", "for", "switch", "case", "default", "break", "continue",
        "return", "throw", "try", "catch", "finally", "new", "null", "true", "false",
        "this", "super", "import", "package", "alias", "var", "int32", "int64", "string", "bool", "void"
    };

    for (const auto& kw : keywords) {
        CompletionItem item;
        item.label = kw;
        item.kind = static_cast<int>(CompletionItemKind::Keyword);
        item.insertText = kw;
        comp_list.items.push_back(item);
    }

    if (last_context_) {
        std::string file_path = uri_to_path(uri);
        Node* hit = spatial_index_.find_node_at(file_path, line + 1, std::max(1, character));
        // Find enclosing method
        Node* curr = hit;
        while (curr && curr->node_type != NodeType::METHOD_DECL && curr->node_type != NodeType::CLASS_DECL) {
            curr = curr->parent;
        }

        if (curr && curr->node_type == NodeType::METHOD_DECL) {
            auto* md = static_cast<MethodDeclaration*>(curr);
            for (const auto& param : md->parameters) {
                CompletionItem item;
                item.label = param->var_name;
                item.kind = static_cast<int>(CompletionItemKind::Variable);
                item.detail = param->type_info.to_string();
                item.insertText = param->var_name;
                comp_list.items.push_back(item);
            }
        }

        // Add class names
        for (const auto& [src, nodes] : last_context_->nodes) {
            for (const auto& n : nodes) {
                if (n && n->node_type == NodeType::CLASS_DECL) {
                    auto* cd = static_cast<ClassDeclaration*>(n.get());
                    CompletionItem item;
                    item.label = cd->class_name;
                    item.kind = static_cast<int>(CompletionItemKind::Class);
                    item.insertText = cd->class_name;
                    comp_list.items.push_back(item);
                }
            }
        }
    }

    transport_.send_response(id, comp_list);
}

void LspServer::handle_signature_help(const nlohmann::json& id, const nlohmann::json& params) {
    if (!params.contains("textDocument") || !params.contains("position") || !last_context_) {
        transport_.send_response(id, nullptr);
        return;
    }

    std::string uri = params["textDocument"].value("uri", "");
    int line = params["position"].value("line", 0);
    int character = params["position"].value("character", 0);

    auto doc_text_opt = docs_.get_document_text(uri);
    if (!doc_text_opt.has_value()) {
        transport_.send_response(id, nullptr);
        return;
    }

    // Scan backwards from cursor to find open '(' and count commas
    std::istringstream stream(doc_text_opt.value());
    std::string l;
    int cur_line_idx = 0;
    std::string target_line;
    while (std::getline(stream, l)) {
        if (cur_line_idx == line) {
            target_line = l;
            break;
        }
        cur_line_idx++;
    }

    if (character > static_cast<int>(target_line.size())) {
        character = static_cast<int>(target_line.size());
    }

    int depth = 0;
    int comma_count = 0;
    int open_paren_char = -1;

    for (int i = character - 1; i >= 0; --i) {
        char c = target_line[i];
        if (c == ')') {
            depth++;
        } else if (c == '(') {
            if (depth == 0) {
                open_paren_char = i;
                break;
            } else {
                depth--;
            }
        } else if (c == ',' && depth == 0) {
            comma_count++;
        }
    }

    if (open_paren_char < 0) {
        transport_.send_response(id, nullptr);
        return;
    }

    // Extract callee name immediately preceding '('
    size_t end_callee = open_paren_char;
    while (end_callee > 0 && std::isspace(target_line[end_callee - 1])) end_callee--;
    size_t start_callee = end_callee;
    while (start_callee > 0 && (std::isalnum(target_line[start_callee - 1]) || target_line[start_callee - 1] == '_')) {
        start_callee--;
    }
    std::string method_name = target_line.substr(start_callee, end_callee - start_callee);

    if (method_name.empty()) {
        transport_.send_response(id, nullptr);
        return;
    }

    // Search context for method declaration
    MethodDeclaration* target_method = nullptr;
    for (const auto& [src, nodes] : last_context_->nodes) {
        for (const auto& n : nodes) {
            if (!n) continue;
            if (n->node_type == NodeType::CLASS_DECL) {
                auto* cd = static_cast<ClassDeclaration*>(n.get());
                for (const auto& member : cd->children) {
                    if (member && member->node_type == NodeType::METHOD_DECL) {
                        auto* md = static_cast<MethodDeclaration*>(member.get());
                        if (md->method_name == method_name) {
                            target_method = md;
                            break;
                        }
                    }
                }
            } else if (n->node_type == NodeType::METHOD_DECL) {
                auto* md = static_cast<MethodDeclaration*>(n.get());
                if (md->method_name == method_name) {
                    target_method = md;
                    break;
                }
            }
            if (target_method) break;
        }
        if (target_method) break;
    }

    if (!target_method) {
        transport_.send_response(id, nullptr);
        return;
    }

    SignatureInformation sig;
    std::string sig_label = target_method->return_type.to_string() + " " + target_method->method_name + "(";
    for (size_t i = 0; i < target_method->parameters.size(); ++i) {
        if (i > 0) sig_label += ", ";
        std::string p_str = target_method->parameters[i]->type_info.to_string() + " " + target_method->parameters[i]->var_name;
        sig_label += p_str;

        ParameterInformation param_info;
        param_info.label = p_str;
        param_info.documentation = "Parameter " + target_method->parameters[i]->var_name;
        sig.parameters.push_back(param_info);
    }
    sig_label += ")";
    sig.label = sig_label;
    sig.activeParameter = comma_count;

    SignatureHelp help;
    help.signatures.push_back(sig);
    help.activeSignature = 0;
    help.activeParameter = comma_count;

    nlohmann::json help_json = help;
    transport_.send_response(id, help_json);
}

} // namespace solix::lsp
