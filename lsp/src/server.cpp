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
#include <unordered_set>

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
    workspace_root_ = root.lexically_normal();
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

    // 1. Discover project manifest (check upwards from target_p, falling back to workspace_root_)
    std::filesystem::path current_proj_root = workspace_root_;
    nlohmann::json current_manifest = manifest_;

    std::filesystem::path search_dir = target_p.parent_path();
    while (!search_dir.empty()) {
        std::filesystem::path candidate = search_dir / "solix.json";
        if (std::filesystem::exists(candidate)) {
            current_proj_root = search_dir;
            std::ifstream mf(candidate);
            if (mf.is_open()) {
                try {
                    current_manifest = nlohmann::json();
                    mf >> current_manifest;
                } catch (...) {}
            }
            break;
        }
        if (search_dir == search_dir.parent_path()) break;
        search_dir = search_dir.parent_path();
    }

    if (!current_manifest.is_null() && current_manifest.is_object()) {
        std::string profile_name = "debug";
        nlohmann::json active_profile = nlohmann::json::object();
        if (current_manifest.contains("profiles") && current_manifest["profiles"].is_object() && current_manifest["profiles"].contains(profile_name)) {
            active_profile = current_manifest["profiles"][profile_name];
        }

        solix::cli::DependencyManager dep_mgr;
        dep_mgr.resolve_all(current_manifest, current_proj_root, active_profile, opts, &dependency_roots_);
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

    // Preserve previously known valid classes if the current parse cycle encountered syntax errors
    if (last_context_) {
        for (const auto& [prev_src, prev_nodes] : last_context_->nodes) {
            auto it = context.nodes.find(prev_src);
            if (it == context.nodes.end() || it->second.empty()) {
                for (const auto& pn : prev_nodes) {
                    if (pn) context.nodes[prev_src].push_back(pn->clone());
                }
            } else {
                for (const auto& pn : prev_nodes) {
                    if (pn && pn->node_type == NodeType::CLASS_DECL) {
                        auto* prev_cd = static_cast<ClassDeclaration*>(pn.get());
                        bool found = false;
                        for (const auto& cur_n : it->second) {
                            if (cur_n && cur_n->node_type == NodeType::CLASS_DECL) {
                                auto* cur_cd = static_cast<ClassDeclaration*>(cur_n.get());
                                if (cur_cd->class_name == prev_cd->class_name) {
                                    found = true;
                                    break;
                                }
                            }
                        }
                        if (!found) {
                            it->second.push_back(prev_cd->clone());
                        }
                    }
                }
            }
        }
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
    Node* cur = node;
    while (cur && path.empty()) {
        if (cur->source && !cur->source->valueless_by_exception()) {
            try {
                if (std::holds_alternative<std::filesystem::path>(*cur->source)) {
                    path = std::get<std::filesystem::path>(*cur->source).string();
                } else if (std::holds_alternative<std::string>(*cur->source)) {
                    path = std::get<std::string>(*cur->source);
                }
            } catch (...) {}
        }
        cur = cur->parent;
    }

    if (path.empty() && ctx) {
        std::function<bool(Node*)> contains_node = [&](Node* n) -> bool {
            if (!n) return false;
            if (n == node) return true;
            if (n->node_type == NodeType::METHOD_DECL) {
                auto* m = static_cast<MethodDeclaration*>(n);
                for (const auto& p : m->parameters) {
                    if (p.get() == node) return true;
                }
            } else if (n->node_type == NodeType::CONSTRUCTOR_DECL) {
                auto* c = static_cast<ConstructorDeclaration*>(n);
                for (const auto& p : c->parameters) {
                    if (p.get() == node) return true;
                }
            }
            for (const auto& ch : n->children) {
                if (contains_node(ch.get())) return true;
            }
            return false;
        };

        for (const auto& [src, nodes] : ctx->nodes) {
            bool found = false;
            for (const auto& root : nodes) {
                if (contains_node(root.get())) {
                    found = true;
                    break;
                }
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

// Forward declarations of anonymous-namespace helpers used by format_hover_for_node
Node* find_type_declaration(const std::string& full_type_name, CompilationContext* context);

static std::string access_modifier_str(TokenType t) {
    switch (t) {
        case TokenType::KEYWORD_PUBLIC:    return "public";
        case TokenType::KEYWORD_PRIVATE:   return "private";
        case TokenType::KEYWORD_PROTECTED: return "protected";
        case TokenType::KEYWORD_INTERNAL:  return "internal";
        default:                           return "";
    }
}

std::string format_hover_for_node(Node* node, CompilationContext* ctx) {
    if (!node) return "";
    std::string md;

    switch (node->node_type) {
        case NodeType::VAR_DECL: {
            auto* v = static_cast<VariableDeclaration*>(node);
            std::string sig;
            if (v->is_const) sig += "const ";
            if (v->is_weak) sig += "weak ";
            sig += v->type_info.to_string();
            if (v->is_reference_type) sig += "&";
            sig += " " + v->var_name;
            md = "```solix\n" + sig + "\n```";
            break;
        }
        case NodeType::FIELD_DECL: {
            auto* f = static_cast<FieldDeclaration*>(node);
            std::string sig = access_modifier_str(f->access_modifier);
            if (!sig.empty()) sig += " ";
            if (f->is_static) sig += "static ";
            if (f->is_const) sig += "const ";
            if (f->is_weak) sig += "weak ";
            sig += f->type_info.to_string();
            if (f->is_reference_type) sig += "&";
            sig += " " + f->field_name;
            md = "```solix\n" + sig + "\n```";
            break;
        }
        case NodeType::METHOD_DECL: {
            auto* m = static_cast<MethodDeclaration*>(node);
            std::string sig = access_modifier_str(m->access_modifier);
            if (!sig.empty()) sig += " ";
            if (m->is_static) sig += "static ";
            if (m->is_inline) sig += "inline ";
            if (m->is_native) sig += "native ";
            if (m->is_virtual) sig += "virtual ";
            if (m->is_override) sig += "override ";
            if (m->is_abstract) sig += "abstract ";
            sig += m->return_type.to_string() + " " + m->method_name + "(";
            for (size_t i = 0; i < m->parameters.size(); ++i) {
                if (i > 0) sig += ", ";
                sig += m->parameters[i]->type_info.to_string();
                if (m->parameters[i]->is_reference_type) sig += "&";
                sig += " " + m->parameters[i]->var_name;
            }
            sig += ")";
            md = "```solix\n" + sig + "\n```";
            break;
        }
        case NodeType::CONSTRUCTOR_DECL: {
            auto* c = static_cast<ConstructorDeclaration*>(node);
            std::string sig = access_modifier_str(c->access_modifier);
            if (!sig.empty()) sig += " ";
            sig += c->class_name + "(";
            for (size_t i = 0; i < c->parameters.size(); ++i) {
                if (i > 0) sig += ", ";
                sig += c->parameters[i]->type_info.to_string();
                if (c->parameters[i]->is_reference_type) sig += "&";
                sig += " " + c->parameters[i]->var_name;
            }
            sig += ")";
            md = "```solix\n" + sig + "\n```";
            break;
        }
        case NodeType::CLASS_DECL: {
            auto* c = static_cast<ClassDeclaration*>(node);
            std::string sig = access_modifier_str(c->access_modifier);
            if (!sig.empty()) sig += " ";
            if (c->is_abstract && !c->is_interface) sig += "abstract ";
            sig += (c->is_interface ? std::string("interface ") : std::string("class "));
            sig += c->class_name;
            if (!c->template_parameters.empty()) {
                sig += "<";
                for (size_t i = 0; i < c->template_parameters.size(); ++i) {
                    if (i > 0) sig += ", ";
                    sig += c->template_parameters[i];
                }
                sig += ">";
            }
            if (!c->base_class_name.empty()) {
                sig += " extends " + c->base_class_name;
            }
            if (!c->implemented_interfaces.empty()) {
                sig += " implements ";
                for (size_t i = 0; i < c->implemented_interfaces.size(); ++i) {
                    if (i > 0) sig += ", ";
                    sig += c->implemented_interfaces[i];
                }
            }
            md = "```solix\n" + sig + "\n```";
            break;
        }
        case NodeType::ENUM_DECL: {
            auto* e = static_cast<EnumDeclaration*>(node);
            std::string sig = access_modifier_str(e->access_modifier);
            if (!sig.empty()) sig += " ";
            sig += "enum " + e->enum_name;
            md = "```solix\n" + sig + "\n```";
            break;
        }
        case NodeType::PACKAGE_STMT: {
            auto* p = static_cast<PackageStatement*>(node);
            md = "```solix\npackage " + p->package_name + ";\n```";
            break;
        }
        case NodeType::IMPORT_STMT: {
            auto* imp = static_cast<ImportStatement*>(node);
            md = "```solix\nimport " + imp->package_name + "." + imp->symbol_name + ";\n```";
            break;
        }
        case NodeType::ALIAS_STMT: {
            auto* a = static_cast<AliasStatement*>(node);
            md = "```solix\nalias " + a->alias_name + " = " + a->target_type.to_string() + "\n```";
            break;
        }
        case NodeType::NEW_INSTANCE: {
            auto* ni = static_cast<NewInstanceExpression*>(node);
            // Show the constructor signature if we can find it
            if (ctx) {
                Node* cls_node = find_type_declaration(ni->type_info.name, ctx);
                if (cls_node && cls_node->node_type == NodeType::CLASS_DECL) {
                    auto* cls = static_cast<ClassDeclaration*>(cls_node);
                    for (const auto& ch : cls->children) {
                        if (ch && ch->node_type == NodeType::CONSTRUCTOR_DECL) {
                            return format_hover_for_node(ch.get(), ctx);
                        }
                    }
                }
            }
            md = "```solix\nnew " + ni->type_info.to_string() + "()\n```";
            break;
        }
        case NodeType::MEMBER_ACCESS: {
            auto* mem = static_cast<MemberAccessExpression*>(node);
            if (mem->resolved_declaration) {
                return format_hover_for_node(mem->resolved_declaration, ctx);
            }
            if (mem->parent && mem->parent->resolved_declaration) {
                return format_hover_for_node(mem->parent->resolved_declaration, ctx);
            }
            if (!mem->expression_type.name.empty()) {
                md = "```solix\n" + mem->expression_type.to_string() + " " + mem->member_name + "\n```";
            }
            break;
        }
        case NodeType::METHOD_CALL: {
            auto* mc = static_cast<MethodCallExpression*>(node);
            if (mc->callee && mc->callee->resolved_declaration) {
                return format_hover_for_node(mc->callee->resolved_declaration, ctx);
            }
            if (!mc->expression_type.name.empty()) {
                md = "```solix\n" + mc->expression_type.to_string() + "\n```";
            }
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
            if (node->resolved_declaration) {
                return format_hover_for_node(node->resolved_declaration, ctx);
            }
            if (!node->expression_type.name.empty()) {
                md = "```solix\n" + node->expression_type.to_string() + "\n```";
            }
            break;
        }
    }
    return md;
}

std::string get_word_at_position(const std::string& text, int line, int character) {
    if (text.empty() || line < 1 || character < 1) return "";
    
    int current_line = 1;
    size_t line_start = 0;
    while (current_line < line && line_start < text.size()) {
        size_t next_nl = text.find('\n', line_start);
        if (next_nl == std::string::npos) return "";
        line_start = next_nl + 1;
        current_line++;
    }
    if (current_line != line || line_start >= text.size()) return "";

    size_t line_end = text.find('\n', line_start);
    if (line_end == std::string::npos) line_end = text.size();
    if (line_end > line_start && text[line_end - 1] == '\r') line_end--;

    size_t col_idx = line_start + static_cast<size_t>(character - 1);
    if (col_idx > line_end) return "";

    auto is_ident_char = [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
    };

    if (col_idx == line_end || !is_ident_char(text[col_idx])) {
        if (col_idx > line_start && is_ident_char(text[col_idx - 1])) {
            col_idx--;
        } else {
            return "";
        }
    }

    size_t start = col_idx;
    while (start > line_start && is_ident_char(text[start - 1])) {
        start--;
    }

    size_t end = col_idx;
    while (end < line_end && is_ident_char(text[end])) {
        end++;
    }

    return text.substr(start, end - start);
}

bool is_keyword(const std::string& w) {
    static const std::unordered_set<std::string> kw = {
        "if", "else", "while", "for", "do", "switch", "case", "default",
        "break", "continue", "return", "throw", "try", "catch", "finally",
        "class", "interface", "enum", "package", "import", "alias",
        "public", "private", "protected", "internal", "static", "inline",
        "native", "virtual", "override", "const", "weak", "abstract",
        "new", "extends", "implements", "operator", "assert", "exit",
        "int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64",
        "f32", "f64", "bool", "string", "void", "auto", "null", "true", "false",
        "instanceof", "sizeof", "super", "this"
    };
    return kw.find(w) != kw.end();
}

Node* find_type_declaration(const std::string& full_type_name, CompilationContext* context) {
    if (!context || full_type_name.empty()) return nullptr;

    std::string base = full_type_name;
    auto lt = base.find('<');
    if (lt != std::string::npos) base = base.substr(0, lt);
    auto brk = base.find('[');
    if (brk != std::string::npos) base = base.substr(0, brk);

    if (base.empty()) return nullptr;

    for (const auto& [name, sym] : context->symbols) {
        if (!sym) continue;
        if (sym->node_type == NodeType::CLASS_DECL || sym->node_type == NodeType::ENUM_DECL || sym->node_type == NodeType::ALIAS_STMT) {
            if (name == base || sym->mangled_name == base) {
                return sym;
            }
        }
    }

    for (const auto& [src, nodes] : context->nodes) {
        for (const auto& n : nodes) {
            if (!n) continue;
            if (n->node_type == NodeType::CLASS_DECL) {
                auto* c = static_cast<ClassDeclaration*>(n.get());
                if (c->class_name == base || c->mangled_name == base) {
                    return c;
                }
            } else if (n->node_type == NodeType::ENUM_DECL) {
                auto* e = static_cast<EnumDeclaration*>(n.get());
                if (e->enum_name == base || e->mangled_name == base) {
                    return e;
                }
            } else if (n->node_type == NodeType::ALIAS_STMT) {
                auto* a = static_cast<AliasStatement*>(n.get());
                if (a->alias_name == base || a->mangled_name == base) {
                    return a;
                }
            }
        }
    }

    return nullptr;
}

Node* find_overridden_method(MethodDeclaration* method, CompilationContext* context) {
    if (!method || !context) return nullptr;
    auto* parent_cls = dynamic_cast<ClassDeclaration*>(method->parent);
    if (!parent_cls) return nullptr;

    if (!parent_cls->base_class_name.empty()) {
        Node* base_node = find_type_declaration(parent_cls->base_class_name, context);
        if (base_node && base_node->node_type == NodeType::CLASS_DECL) {
            auto* base_cls = static_cast<ClassDeclaration*>(base_node);
            for (const auto& ch : base_cls->children) {
                if (ch && ch->node_type == NodeType::METHOD_DECL) {
                    auto* m = static_cast<MethodDeclaration*>(ch.get());
                    if (m->method_name == method->method_name) {
                        return m;
                    }
                }
            }
        }
    }

    for (const auto& iface_name : parent_cls->implemented_interfaces) {
        Node* iface_node = find_type_declaration(iface_name, context);
        if (iface_node && iface_node->node_type == NodeType::CLASS_DECL) {
            auto* iface_cls = static_cast<ClassDeclaration*>(iface_node);
            for (const auto& ch : iface_cls->children) {
                if (ch && ch->node_type == NodeType::METHOD_DECL) {
                    auto* m = static_cast<MethodDeclaration*>(ch.get());
                    if (m->method_name == method->method_name) {
                        return m;
                    }
                }
            }
        }
    }

    return nullptr;
}

ClassDeclaration* find_enclosing_class_at(const std::string& file_path, int line, CompilationContext* context) {
    if (!context || file_path.empty() || line <= 0) return nullptr;
    std::string norm_target = std::filesystem::path(file_path).lexically_normal().string();

    for (const auto& [src, nodes] : context->nodes) {
        std::string src_path;
        if (std::holds_alternative<std::filesystem::path>(src)) {
            src_path = std::get<std::filesystem::path>(src).lexically_normal().string();
        } else if (std::holds_alternative<std::string>(src)) {
            src_path = std::filesystem::path(std::get<std::string>(src)).lexically_normal().string();
        }
        if (src_path != norm_target) continue;

        for (const auto& n : nodes) {
            if (n && n->node_type == NodeType::CLASS_DECL) {
                auto* cd = static_cast<ClassDeclaration*>(n.get());
                uint32_t uline = static_cast<uint32_t>(line);
                if (uline >= cd->line && (cd->end_line == 0 || uline <= cd->end_line)) {
                    return cd;
                }
            }
        }
    }
    return nullptr;
}

Node* find_enclosing_callable_at(const std::string& file_path, int line, CompilationContext* context) {
    ClassDeclaration* cd = find_enclosing_class_at(file_path, line, context);
    if (!cd) return nullptr;

    uint32_t uline = static_cast<uint32_t>(line);
    for (const auto& ch : cd->children) {
        if (!ch) continue;
        if (ch->node_type == NodeType::METHOD_DECL) {
            auto* md = static_cast<MethodDeclaration*>(ch.get());
            if (uline >= md->line && (md->end_line == 0 || uline <= md->end_line)) {
                return md;
            }
        } else if (ch->node_type == NodeType::CONSTRUCTOR_DECL) {
            auto* ctor = static_cast<ConstructorDeclaration*>(ch.get());
            if (uline >= ctor->line && (ctor->end_line == 0 || uline <= ctor->end_line)) {
                return ctor;
            }
        }
    }
    return nullptr;
}

Node* find_symbol_in_scope(const std::string& name, const std::string& file_path, int line, CompilationContext* context) {
    if (name.empty() || !context) return nullptr;

    std::function<Node*(Node*)> scan_locals = [&](Node* n) -> Node* {
        if (!n) return nullptr;
        if (n->node_type == NodeType::VAR_DECL) {
            auto* vd = static_cast<VariableDeclaration*>(n);
            if (vd->var_name == name) return vd;
        }
        for (const auto& child : n->children) {
            Node* found = scan_locals(child.get());
            if (found) return found;
        }
        return nullptr;
    };

    // 1. Check enclosing callable (Method or Constructor)
    Node* callable = find_enclosing_callable_at(file_path, line, context);
    if (callable) {
        if (callable->node_type == NodeType::METHOD_DECL) {
            auto* md = static_cast<MethodDeclaration*>(callable);
            for (const auto& param : md->parameters) {
                if (param && param->var_name == name) return param.get();
            }
            for (const auto& ch : md->children) {
                Node* found = scan_locals(ch.get());
                if (found) return found;
            }
        } else if (callable->node_type == NodeType::CONSTRUCTOR_DECL) {
            auto* ctor = static_cast<ConstructorDeclaration*>(callable);
            for (const auto& param : ctor->parameters) {
                if (param && param->var_name == name) return param.get();
            }
            for (const auto& ch : ctor->children) {
                Node* found = scan_locals(ch.get());
                if (found) return found;
            }
        }
    }

    // 2. Check enclosing class
    ClassDeclaration* enc_cls = find_enclosing_class_at(file_path, line, context);
    if (enc_cls) {
        for (const auto& member : enc_cls->children) {
            if (!member) continue;
            if (member->node_type == NodeType::FIELD_DECL) {
                auto* fd = static_cast<FieldDeclaration*>(member.get());
                if (fd->field_name == name) return fd;
            } else if (member->node_type == NodeType::METHOD_DECL) {
                auto* m = static_cast<MethodDeclaration*>(member.get());
                if (m->method_name == name) return m;
            }
        }
    }

    // 3. Check all classes in context
    for (const auto& [src, nodes] : context->nodes) {
        for (const auto& n : nodes) {
            if (!n || n->node_type != NodeType::CLASS_DECL) continue;
            auto* cd = static_cast<ClassDeclaration*>(n.get());
            for (const auto& member : cd->children) {
                if (!member) continue;
                if (member->node_type == NodeType::FIELD_DECL) {
                    auto* fd = static_cast<FieldDeclaration*>(member.get());
                    if (fd->field_name == name) return fd;
                } else if (member->node_type == NodeType::METHOD_DECL) {
                    auto* m = static_cast<MethodDeclaration*>(member.get());
                    if (m->method_name == name) return m;
                }
            }
        }
    }

    // 4. Check types
    return find_type_declaration(name, context);
}

Node* resolve_member_access(MemberAccessExpression* mem, const std::string& file_path, int line, CompilationContext* ctx) {
    if (!mem || !ctx) return nullptr;
    if (mem->resolved_declaration) return mem->resolved_declaration;

    std::string receiver_type_name;
    if (mem->object) {
        if (mem->object->resolved_declaration) {
            Node* d = mem->object->resolved_declaration;
            if (d->node_type == NodeType::VAR_DECL) receiver_type_name = static_cast<VariableDeclaration*>(d)->type_info.name;
            else if (d->node_type == NodeType::FIELD_DECL) receiver_type_name = static_cast<FieldDeclaration*>(d)->type_info.name;
        } else if (!mem->object->expression_type.name.empty()) {
            receiver_type_name = mem->object->expression_type.name;
        } else if (mem->object->node_type == NodeType::IDENTIFIER) {
            auto* id = static_cast<IdentifierNode*>(mem->object.get());
            if (id->name == "this") {
                auto* enc = find_enclosing_class_at(file_path, line, ctx);
                if (enc) receiver_type_name = enc->class_name;
            } else {
                Node* sym = find_symbol_in_scope(id->name, file_path, line, ctx);
                if (sym) {
                    if (sym->node_type == NodeType::VAR_DECL) receiver_type_name = static_cast<VariableDeclaration*>(sym)->type_info.name;
                    else if (sym->node_type == NodeType::FIELD_DECL) receiver_type_name = static_cast<FieldDeclaration*>(sym)->type_info.name;
                }
            }
        } else if (mem->object->node_type == NodeType::MEMBER_ACCESS) {
            Node* parent_member = resolve_member_access(static_cast<MemberAccessExpression*>(mem->object.get()), file_path, line, ctx);
            if (parent_member) {
                if (parent_member->node_type == NodeType::FIELD_DECL) {
                    receiver_type_name = static_cast<FieldDeclaration*>(parent_member)->type_info.name;
                } else if (parent_member->node_type == NodeType::METHOD_DECL) {
                    receiver_type_name = static_cast<MethodDeclaration*>(parent_member)->return_type.name;
                }
            }
        }
    }

    if (receiver_type_name.empty()) return nullptr;

    auto lt = receiver_type_name.find('<');
    if (lt != std::string::npos) {
        receiver_type_name = receiver_type_name.substr(0, lt);
    }

    Node* type_decl = find_type_declaration(receiver_type_name, ctx);
    if (!type_decl || type_decl->node_type != NodeType::CLASS_DECL) return nullptr;

    auto* cd = static_cast<ClassDeclaration*>(type_decl);
    for (const auto& ch : cd->children) {
        if (!ch) continue;
        if (ch->node_type == NodeType::METHOD_DECL) {
            auto* md = static_cast<MethodDeclaration*>(ch.get());
            if (md->method_name == mem->member_name) return md;
        } else if (ch->node_type == NodeType::FIELD_DECL) {
            auto* fd = static_cast<FieldDeclaration*>(ch.get());
            if (fd->field_name == mem->member_name) return fd;
        }
    }

    return nullptr;
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
    auto doc_opt = docs_.get_document_text(uri);
    std::string source_text = doc_opt.value_or("");
    if (source_text.empty() && last_opts_) {
        auto it = last_opts_->sources.find(file_path);
        if (it != last_opts_->sources.end()) {
            source_text = it->second.value_or("");
        }
    }

    std::string word = get_word_at_position(source_text, line, character);
    Node* target = nullptr;

    Node* hit = spatial_index_.find_node_at(file_path, line, character);
    if (hit && hit->node_type == NodeType::PACKAGE_STMT) {
        target = hit;
    }

    // Reserved keywords suppress Go-to-Definition unless hovering/clicking on package statement
    if (!target && is_keyword(word)) {
        transport_.send_response(id, nullptr);
        return;
    }

    // 1. Direct type resolution (e.g. extends Base, implements IFoo, type annotations, casts, catch)
    if (!word.empty()) {
        Node* type_match = find_type_declaration(word, last_context_.get());
        if (type_match) {
            // If the cursor is on the declaration of the class/enum itself, do not target itself
            if (type_match->node_type == NodeType::CLASS_DECL) {
                auto* c = static_cast<ClassDeclaration*>(type_match);
                if (c->class_name == word && c->line == static_cast<uint32_t>(line)) {
                    target = nullptr;
                } else {
                    target = type_match;
                }
            } else if (type_match->node_type == NodeType::ENUM_DECL) {
                auto* e = static_cast<EnumDeclaration*>(type_match);
                if (e->enum_name == word && e->line == static_cast<uint32_t>(line)) {
                    target = nullptr;
                } else {
                    target = type_match;
                }
            } else {
                target = type_match;
            }
        }
    }

    // 2. Spatial AST query with multi-step definition chaining
    if (!target) {
        Node* hit = spatial_index_.find_node_at(file_path, line, character);
        if (hit) {
            if (hit->node_type == NodeType::PACKAGE_STMT) {
                target = hit;
            } else if (hit->node_type == NodeType::METHOD_DECL) {
                auto* md = static_cast<MethodDeclaration*>(hit);
                if (md->is_override) {
                    target = find_overridden_method(md, last_context_.get());
                }
                if (!target) target = md;
            } else if (hit->node_type == NodeType::VAR_DECL) {
                auto* vd = static_cast<VariableDeclaration*>(hit);
                target = find_type_declaration(vd->type_info.name, last_context_.get());
                if (!target) target = vd;
            } else if (hit->node_type == NodeType::FIELD_DECL) {
                auto* fd = static_cast<FieldDeclaration*>(hit);
                target = find_type_declaration(fd->type_info.name, last_context_.get());
                if (!target) target = fd;
            } else if (hit->node_type == NodeType::NEW_INSTANCE) {
                auto* ni = static_cast<NewInstanceExpression*>(hit);
                target = find_type_declaration(ni->type_info.name, last_context_.get());
            } else if (hit->node_type == NodeType::CAST_EXPR) {
                auto* ce = static_cast<CastExpression*>(hit);
                target = find_type_declaration(ce->target_type.name, last_context_.get());
            } else if (hit->node_type == NodeType::INSTANCEOF_EXPR) {
                auto* ie = static_cast<InstanceofExpression*>(hit);
                target = find_type_declaration(ie->target_type.name, last_context_.get());
            } else if (hit->node_type == NodeType::CATCH_CLAUSE) {
                auto* cc = static_cast<CatchClause*>(hit);
                target = find_type_declaration(cc->exception_type.name, last_context_.get());
            } else if (hit->node_type == NodeType::IMPORT_STMT) {
                auto* imp = static_cast<ImportStatement*>(hit);
                if (imp->symbol_name != "*") {
                    target = find_type_declaration(imp->symbol_name, last_context_.get());
                    if (!target) {
                        target = find_type_declaration(imp->package_name + "." + imp->symbol_name, last_context_.get());
                    }
                }
                if (!target) {
                    target = hit;
                }
            } else {
                target = hit->resolved_declaration;
                if (!target && hit->parent && hit->parent->resolved_declaration) {
                    target = hit->parent->resolved_declaration;
                }
                if (!target && hit->node_type == NodeType::MEMBER_ACCESS) {
                    target = resolve_member_access(static_cast<MemberAccessExpression*>(hit), file_path, line, last_context_.get());
                }
                if (!target && hit->node_type == NodeType::METHOD_CALL) {
                    auto* mc = static_cast<MethodCallExpression*>(hit);
                    if (mc->callee && mc->callee->node_type == NodeType::MEMBER_ACCESS) {
                        target = resolve_member_access(static_cast<MemberAccessExpression*>(mc->callee.get()), file_path, line, last_context_.get());
                    }
                }
                if (!target) {
                    if (hit->node_type == NodeType::VAR_DECL || hit->node_type == NodeType::METHOD_DECL ||
                        hit->node_type == NodeType::CLASS_DECL || hit->node_type == NodeType::FIELD_DECL ||
                        hit->node_type == NodeType::ENUM_DECL || hit->node_type == NodeType::ALIAS_STMT ||
                        hit->node_type == NodeType::PACKAGE_STMT || hit->node_type == NodeType::IMPORT_STMT) {
                        target = hit;
                    }
                }
            }
        }
    }

    if (!target && !word.empty()) {
        target = find_symbol_in_scope(word, file_path, line, last_context_.get());
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
    auto doc_opt = docs_.get_document_text(uri);
    std::string source_text = doc_opt.value_or("");
    if (source_text.empty() && last_opts_) {
        auto it = last_opts_->sources.find(file_path);
        if (it != last_opts_->sources.end()) {
            source_text = it->second.value_or("");
        }
    }

    std::string word = get_word_at_position(source_text, line, character);
    Node* type_decl = nullptr;

    if (!word.empty() && !is_keyword(word)) {
        type_decl = find_type_declaration(word, last_context_.get());
    }

    if (!type_decl) {
        Node* hit = spatial_index_.find_node_at(file_path, line, character);
        if (hit) {
            std::string type_name = hit->expression_type.name;
            if (hit->node_type == NodeType::VAR_DECL) {
                type_name = static_cast<VariableDeclaration*>(hit)->type_info.name;
            } else if (hit->node_type == NodeType::FIELD_DECL) {
                type_name = static_cast<FieldDeclaration*>(hit)->type_info.name;
            } else if (hit->node_type == NodeType::NEW_INSTANCE) {
                type_name = static_cast<NewInstanceExpression*>(hit)->type_info.name;
            } else if (hit->node_type == NodeType::CAST_EXPR) {
                type_name = static_cast<CastExpression*>(hit)->target_type.name;
            } else if (hit->node_type == NodeType::INSTANCEOF_EXPR) {
                type_name = static_cast<InstanceofExpression*>(hit)->target_type.name;
            } else if (hit->node_type == NodeType::CATCH_CLAUSE) {
                type_name = static_cast<CatchClause*>(hit)->exception_type.name;
            } else if (type_name.empty() && hit->resolved_declaration) {
                if (hit->resolved_declaration->node_type == NodeType::VAR_DECL) {
                    type_name = static_cast<VariableDeclaration*>(hit->resolved_declaration)->type_info.name;
                } else if (hit->resolved_declaration->node_type == NodeType::FIELD_DECL) {
                    type_name = static_cast<FieldDeclaration*>(hit->resolved_declaration)->type_info.name;
                } else {
                    type_name = hit->resolved_declaration->expression_type.name;
                }
            }
            type_decl = find_type_declaration(type_name, last_context_.get());
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

    // Suppress hover on keywords and punctuation
    auto doc_opt = docs_.get_document_text(uri);
    std::string source_text = doc_opt.value_or("");
    if (source_text.empty() && last_opts_) {
        auto it = last_opts_->sources.find(file_path);
        if (it != last_opts_->sources.end()) {
            source_text = it->second.value_or("");
        }
    }
    std::string word = get_word_at_position(source_text, line, character);
    Node* hit = spatial_index_.find_node_at(file_path, line, character);

    if (!word.empty() && is_keyword(word)) {
        if (!hit || (hit->node_type != NodeType::PACKAGE_STMT && hit->node_type != NodeType::IMPORT_STMT)) {
            transport_.send_response(id, nullptr);
            return;
        }
    }

    if (hit && hit->node_type == NodeType::VAR_DECL) {
        auto* vd = static_cast<VariableDeclaration*>(hit);
        if (vd->initializer) {
            uint32_t init_line = vd->initializer->line;
            uint32_t init_col = vd->initializer->column;
            if (static_cast<uint32_t>(line) == init_line && static_cast<uint32_t>(character) >= init_col) {
                hit = vd->initializer.get();
            }
        }
    }

    if (hit && hit->node_type == NodeType::MEMBER_ACCESS) {
        Node* resolved_mem = resolve_member_access(static_cast<MemberAccessExpression*>(hit), file_path, line, last_context_.get());
        if (resolved_mem) hit = resolved_mem;
    } else if (hit && hit->node_type == NodeType::METHOD_CALL) {
        auto* mc = static_cast<MethodCallExpression*>(hit);
        if (mc->callee && mc->callee->node_type == NodeType::MEMBER_ACCESS) {
            Node* resolved_mem = resolve_member_access(static_cast<MemberAccessExpression*>(mc->callee.get()), file_path, line, last_context_.get());
            if (resolved_mem) hit = resolved_mem;
        }
    }

    std::string md_text;
    if (hit) {
        md_text = format_hover_for_node(hit, last_context_.get());
    }

    if (md_text.empty() && !word.empty()) {
        hit = find_symbol_in_scope(word, file_path, line, last_context_.get());
        if (hit) md_text = format_hover_for_node(hit, last_context_.get());
    }

    if (md_text.empty() && !word.empty()) {
        hit = find_type_declaration(word, last_context_.get());
        if (hit) md_text = format_hover_for_node(hit, last_context_.get());
    }

    if (!hit || md_text.empty()) {
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
            } else if (node->node_type == NodeType::ALIAS_STMT) {
                auto* as = static_cast<AliasStatement*>(node.get());
                DocumentSymbol as_sym;
                as_sym.name = as->alias_name;
                as_sym.detail = as->target_type.to_string();
                as_sym.kind = static_cast<int>(SymbolKind::TypeParameter);
                as_sym.range = node_to_lsp_range(as);
                as_sym.selectionRange = as_sym.range;
                symbols.push_back(as_sym);
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

    // Check if this is a declaration directive line (package, import, alias)
    bool is_decl_directive_line = false;
    {
        size_t first_non_ws = prefix.find_first_not_of(" \t");
        if (first_non_ws != std::string::npos) {
            std::string rem = prefix.substr(first_non_ws);
            if (rem == "import" || rem.rfind("import ", 0) == 0 || rem.rfind("import\t", 0) == 0 ||
                rem == "package" || rem.rfind("package ", 0) == 0 || rem.rfind("package\t", 0) == 0 ||
                rem == "alias" || rem.rfind("alias ", 0) == 0 || rem.rfind("alias\t", 0) == 0) {
                is_decl_directive_line = true;
            }
        }
    }

    // Check if this is a dot completion (e.g. "obj." or "obj.part")
    size_t last_dot = prefix.rfind('.');
    bool is_dot_access = false;
    std::string receiver_name;

    if (!is_decl_directive_line && last_dot != std::string::npos) {
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

        // 1. If receiver is "this", resolve to the enclosing class
        if (receiver_name == "this") {
            ClassDeclaration* enc = find_enclosing_class_at(file_path, line + 1, last_context_.get());
            if (enc) {
                target_class_name = enc->class_name;
            }
        }

        // 2. If spatial index did not directly resolve receiver, search scope for variable, field, method, or class
        if (target_class_name.empty() && last_context_) {
            Node* sym = find_symbol_in_scope(receiver_name, file_path, line + 1, last_context_.get());
            if (sym) {
                if (sym->node_type == NodeType::VAR_DECL) target_class_name = static_cast<VariableDeclaration*>(sym)->type_info.name;
                else if (sym->node_type == NodeType::FIELD_DECL) target_class_name = static_cast<FieldDeclaration*>(sym)->type_info.name;
                else if (sym->node_type == NodeType::METHOD_DECL) target_class_name = static_cast<MethodDeclaration*>(sym)->return_type.name;
                else if (sym->node_type == NodeType::CLASS_DECL) target_class_name = static_cast<ClassDeclaration*>(sym)->class_name;
            }
        }

        auto lt = target_class_name.find('<');
        if (lt != std::string::npos) {
            target_class_name = target_class_name.substr(0, lt);
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

    // Tokenize prefix before cursor into words and punctuation
    std::vector<std::string> tokens;
    {
        size_t i = 0;
        while (i < prefix.size()) {
            if (std::isspace(prefix[i])) {
                i++;
                continue;
            }
            if (std::isalnum(prefix[i]) || prefix[i] == '_') {
                size_t st = i;
                while (i < prefix.size() && (std::isalnum(prefix[i]) || prefix[i] == '_')) {
                    i++;
                }
                tokens.push_back(prefix.substr(st, i - st));
            } else {
                tokens.push_back(std::string(1, prefix[i]));
                i++;
            }
        }
    }

    // Determine completion context
    enum class CompletionContext {
        GENERAL,
        PACKAGE_DECL,
        IMPORT_STMT,
        ALIAS_NAME_DECL,
        ALIAS_TARGET,
        CLASS_NAME_DECL,
        EXTENDS,
        IMPLEMENTS,
        NEW_INSTANCE,
        CASE_EXPR
    };

    CompletionContext ctx = CompletionContext::GENERAL;
    if (!tokens.empty()) {
        bool has_package_keyword = false;
        bool has_import_keyword = false;
        bool has_alias_keyword = false;
        bool has_equals_after_alias = false;
        for (const auto& t : tokens) {
            if (t == "package") {
                has_package_keyword = true;
            } else if (t == "import") {
                has_import_keyword = true;
            } else if (t == "alias") {
                has_alias_keyword = true;
            } else if (has_alias_keyword && t == "=") {
                has_equals_after_alias = true;
            }
        }

        bool trailing_space = prefix.empty() || std::isspace(prefix.back());
        std::string last_token = tokens.back();
        std::string prev_token = (tokens.size() >= 2) ? tokens[tokens.size() - 2] : "";

        if (has_package_keyword) {
            ctx = CompletionContext::PACKAGE_DECL;
        } else if (has_import_keyword) {
            ctx = CompletionContext::IMPORT_STMT;
        } else if (has_alias_keyword && has_equals_after_alias) {
            ctx = CompletionContext::ALIAS_TARGET;
        } else if (has_alias_keyword && !has_equals_after_alias) {
            ctx = CompletionContext::ALIAS_NAME_DECL;
        } else if (trailing_space) {
            if (last_token == "class") {
                ctx = CompletionContext::CLASS_NAME_DECL;
            } else if (last_token == "extends") {
                ctx = CompletionContext::EXTENDS;
            } else if (last_token == "implements" || (last_token == "," && tokens.size() >= 3)) {
                // Check if implements was seen earlier on the line
                bool seen_implements = false;
                for (const auto& t : tokens) {
                    if (t == "implements") { seen_implements = true; break; }
                }
                if (seen_implements) ctx = CompletionContext::IMPLEMENTS;
            } else if (last_token == "new") {
                ctx = CompletionContext::NEW_INSTANCE;
            } else if (last_token == "case") {
                ctx = CompletionContext::CASE_EXPR;
            }
        } else {
            // Typing in progress, e.g. "class MyFoo", "extends Base", "new Cls", "case Mem"
            if (prev_token == "class") {
                ctx = CompletionContext::CLASS_NAME_DECL;
            } else if (prev_token == "extends") {
                ctx = CompletionContext::EXTENDS;
            } else if (prev_token == "implements" || (prev_token == "," && tokens.size() >= 3)) {
                bool seen_implements = false;
                for (const auto& t : tokens) {
                    if (t == "implements") { seen_implements = true; break; }
                }
                if (seen_implements) ctx = CompletionContext::IMPLEMENTS;
            } else if (prev_token == "new") {
                ctx = CompletionContext::NEW_INSTANCE;
            } else if (prev_token == "case") {
                ctx = CompletionContext::CASE_EXPR;
            }
        }
    }

    // 0. PACKAGE_DECL: Suggest packages inferred from file path and known project packages
    if (ctx == CompletionContext::PACKAGE_DECL) {
        std::string file_path = uri_to_path(uri);
        std::unordered_set<std::string> pkg_suggestions;

        // Path heuristic: relative path from workspace_root_ / "src"
        std::filesystem::path fp = std::filesystem::path(file_path).lexically_normal();
        std::filesystem::path pdir = fp.parent_path();
        if (!workspace_root_.empty()) {
            std::filesystem::path rel;
            try {
                rel = std::filesystem::relative(pdir, workspace_root_);
            } catch (...) {}

            std::string rel_str = rel.generic_string();
            if (rel_str.rfind("src/", 0) == 0) {
                rel_str = rel_str.substr(4);
            } else if (rel_str == "src") {
                rel_str = "";
            }

            if (!rel_str.empty() && rel_str != ".") {
                std::string pkg_from_path;
                for (char c : rel_str) {
                    if (c == '/' || c == '\\') pkg_from_path += '.';
                    else pkg_from_path += c;
                }
                if (!pkg_from_path.empty()) pkg_suggestions.insert(pkg_from_path);
            }
        }

        // Collect known packages from last_context_
        if (last_context_) {
            for (const auto& [src, nodes] : last_context_->nodes) {
                for (const auto& n : nodes) {
                    if (n && n->node_type == NodeType::PACKAGE_STMT) {
                        auto* ps = static_cast<PackageStatement*>(n.get());
                        if (!ps->package_name.empty()) {
                            pkg_suggestions.insert(ps->package_name);
                        }
                    }
                }
            }
        }

        for (const auto& pkg : pkg_suggestions) {
            CompletionItem item;
            item.label = pkg;
            item.kind = static_cast<int>(CompletionItemKind::Module);
            item.detail = "package " + pkg;
            item.insertText = pkg;
            comp_list.items.push_back(item);
        }

        transport_.send_response(id, comp_list);
        return;
    }

    // IMPORT_STMT: Suggest packages, subpackages, package members, and wildcard
    if (ctx == CompletionContext::IMPORT_STMT) {
        std::unordered_set<std::string> seen_labels;

        size_t imp_pos = prefix.find("import");
        std::string after_import;
        if (imp_pos != std::string::npos) {
            after_import = prefix.substr(imp_pos + 6);
        }
        size_t start_idx = after_import.find_first_not_of(" \t");
        std::string typed_path = (start_idx != std::string::npos) ? after_import.substr(start_idx) : "";

        // Collect all known packages
        std::unordered_set<std::string> known_pkgs;
        static const std::vector<std::string> stdlib_packages = {
            "solix.core",
            "solix.system",
            "solix.collections",
            "solix.io",
            "solix.io.filesystem",
            "solix.math",
            "solix.time",
            "solix.exceptions",
            "solix.crypto"
        };
        for (const auto& sp : stdlib_packages) known_pkgs.insert(sp);

        if (last_context_) {
            for (const auto& [src, nodes] : last_context_->nodes) {
                for (const auto& n : nodes) {
                    if (n && n->node_type == NodeType::PACKAGE_STMT) {
                        auto* ps = static_cast<PackageStatement*>(n.get());
                        if (!ps->package_name.empty()) {
                            known_pkgs.insert(ps->package_name);
                        }
                    }
                }
            }
        }

        // Relative path heuristic from workspace_root_ / "src"
        if (!workspace_root_.empty()) {
            try {
                std::filesystem::path src_dir = std::filesystem::path(workspace_root_) / "src";
                if (std::filesystem::exists(src_dir) && std::filesystem::is_directory(src_dir)) {
                    for (const auto& entry : std::filesystem::recursive_directory_iterator(src_dir)) {
                        if (entry.is_directory()) {
                            std::filesystem::path rel = std::filesystem::relative(entry.path(), src_dir);
                            std::string rel_str = rel.string();
                            std::string pkg_str;
                            for (char c : rel_str) {
                                if (c == '/' || c == '\\') pkg_str += '.';
                                else pkg_str += c;
                            }
                            if (!pkg_str.empty()) known_pkgs.insert(pkg_str);
                        }
                    }
                }
            } catch (...) {}
        }

        size_t last_dot_in_path = typed_path.rfind('.');
        if (last_dot_in_path == std::string::npos) {
            // Root level: suggest known packages and top-level packages
            for (const auto& pkg : known_pkgs) {
                if (seen_labels.insert(pkg).second) {
                    CompletionItem item;
                    item.label = pkg;
                    item.kind = static_cast<int>(CompletionItemKind::Module);
                    item.detail = "package " + pkg;
                    item.insertText = pkg;
                    comp_list.items.push_back(item);
                }
                size_t first_dot = pkg.find('.');
                if (first_dot != std::string::npos) {
                    std::string root_seg = pkg.substr(0, first_dot);
                    if (seen_labels.insert(root_seg).second) {
                        CompletionItem item;
                        item.label = root_seg;
                        item.kind = static_cast<int>(CompletionItemKind::Module);
                        item.detail = "package " + root_seg;
                        item.insertText = root_seg;
                        comp_list.items.push_back(item);
                    }
                }
            }
        } else {
            // Package member completion: after dot
            std::string target_pkg = typed_path.substr(0, last_dot_in_path);

            // 1. Wildcard import
            CompletionItem wc_item;
            wc_item.label = "*";
            wc_item.kind = static_cast<int>(CompletionItemKind::Keyword);
            wc_item.detail = "Wildcard import (all symbols in " + target_pkg + ")";
            wc_item.insertText = "*";
            comp_list.items.push_back(wc_item);
            seen_labels.insert("*");

            // 2. Subpackages under target_pkg
            std::string prefix_match = target_pkg + ".";
            for (const auto& pkg : known_pkgs) {
                if (pkg.rfind(prefix_match, 0) == 0) {
                    std::string rest = pkg.substr(prefix_match.size());
                    size_t next_dot = rest.find('.');
                    std::string next_segment = (next_dot != std::string::npos) ? rest.substr(0, next_dot) : rest;
                    if (seen_labels.insert(next_segment).second) {
                        CompletionItem item;
                        item.label = next_segment;
                        item.kind = static_cast<int>(CompletionItemKind::Module);
                        item.detail = "package " + target_pkg + "." + next_segment;
                        item.insertText = next_segment;
                        comp_list.items.push_back(item);
                    }
                }
            }

            // 3. Symbols declared in target_pkg
            if (last_context_) {
                for (const auto& [src, nodes] : last_context_->nodes) {
                    std::string file_pkg;
                    for (const auto& n : nodes) {
                        if (n && n->node_type == NodeType::PACKAGE_STMT) {
                            file_pkg = static_cast<PackageStatement*>(n.get())->package_name;
                            break;
                        }
                    }

                    for (const auto& n : nodes) {
                        if (!n) continue;
                        if (n->node_type == NodeType::CLASS_DECL) {
                            auto* cd = static_cast<ClassDeclaration*>(n.get());
                            bool in_pkg = (!file_pkg.empty() && file_pkg == target_pkg) ||
                                          (cd->mangled_name.rfind(prefix_match, 0) == 0);
                            if (in_pkg && seen_labels.insert(cd->class_name).second) {
                                CompletionItem item;
                                item.label = cd->class_name;
                                item.kind = cd->is_interface ? static_cast<int>(CompletionItemKind::Interface) : static_cast<int>(CompletionItemKind::Class);
                                item.detail = (cd->is_interface ? "interface " : "class ") + cd->class_name;
                                item.insertText = cd->class_name;
                                comp_list.items.push_back(item);
                            }
                        } else if (n->node_type == NodeType::ENUM_DECL) {
                            auto* ed = static_cast<EnumDeclaration*>(n.get());
                            bool in_pkg = (!file_pkg.empty() && file_pkg == target_pkg) ||
                                          (ed->mangled_name.rfind(prefix_match, 0) == 0);
                            if (in_pkg && seen_labels.insert(ed->enum_name).second) {
                                CompletionItem item;
                                item.label = ed->enum_name;
                                item.kind = static_cast<int>(CompletionItemKind::Enum);
                                item.detail = "enum " + ed->enum_name;
                                item.insertText = ed->enum_name;
                                comp_list.items.push_back(item);
                            }
                        } else if (n->node_type == NodeType::ALIAS_STMT) {
                            auto* as = static_cast<AliasStatement*>(n.get());
                            bool in_pkg = (!file_pkg.empty() && file_pkg == target_pkg) ||
                                          (as->mangled_name.rfind(prefix_match, 0) == 0);
                            if (in_pkg && seen_labels.insert(as->alias_name).second) {
                                CompletionItem item;
                                item.label = as->alias_name;
                                item.kind = static_cast<int>(CompletionItemKind::Reference);
                                item.detail = "alias " + as->alias_name + " = " + as->target_type.to_string();
                                item.insertText = as->alias_name;
                                comp_list.items.push_back(item);
                            }
                        }
                    }
                }
            }
        }

        transport_.send_response(id, comp_list);
        return;
    }

    // ALIAS_NAME_DECL: when typing new alias name, do NOT suggest existing type names
    if (ctx == CompletionContext::ALIAS_NAME_DECL) {
        transport_.send_response(id, comp_list);
        return;
    }

    // ALIAS_TARGET: Suggest primitive types, classes, interfaces, enums, existing aliases
    if (ctx == CompletionContext::ALIAS_TARGET) {
        std::unordered_set<std::string> seen_labels;

        // 1. Primitive types
        static const std::vector<std::string> primitive_types = {
            "int8", "int16", "int32", "int64",
            "uint8", "uint16", "uint32", "uint64",
            "float32", "float64",
            "bool", "char", "string", "void", "any"
        };
        for (const auto& pt : primitive_types) {
            if (seen_labels.insert(pt).second) {
                CompletionItem item;
                item.label = pt;
                item.kind = static_cast<int>(CompletionItemKind::Keyword);
                item.detail = "primitive type";
                item.insertText = pt;
                comp_list.items.push_back(item);
            }
        }

        // 2. Classes, Interfaces, Enums, and Aliases from context
        if (last_context_) {
            for (const auto& [src, nodes] : last_context_->nodes) {
                for (const auto& n : nodes) {
                    if (!n) continue;
                    if (n->node_type == NodeType::CLASS_DECL) {
                        auto* cd = static_cast<ClassDeclaration*>(n.get());
                        if (seen_labels.insert(cd->class_name).second) {
                            CompletionItem item;
                            item.label = cd->class_name;
                            item.kind = cd->is_interface ? static_cast<int>(CompletionItemKind::Interface) : static_cast<int>(CompletionItemKind::Class);
                            item.detail = cd->is_interface ? "interface " + cd->class_name : "class " + cd->class_name;
                            item.insertText = cd->class_name;
                            comp_list.items.push_back(item);
                        }
                    } else if (n->node_type == NodeType::ENUM_DECL) {
                        auto* ed = static_cast<EnumDeclaration*>(n.get());
                        if (seen_labels.insert(ed->enum_name).second) {
                            CompletionItem item;
                            item.label = ed->enum_name;
                            item.kind = static_cast<int>(CompletionItemKind::Enum);
                            item.detail = "enum " + ed->enum_name;
                            item.insertText = ed->enum_name;
                            comp_list.items.push_back(item);
                        }
                    } else if (n->node_type == NodeType::ALIAS_STMT) {
                        auto* as = static_cast<AliasStatement*>(n.get());
                        if (seen_labels.insert(as->alias_name).second) {
                            CompletionItem item;
                            item.label = as->alias_name;
                            item.kind = static_cast<int>(CompletionItemKind::Reference);
                            item.detail = "alias " + as->alias_name + " = " + as->target_type.to_string();
                            item.insertText = as->alias_name;
                            comp_list.items.push_back(item);
                        }
                    }
                }
            }
        }

        transport_.send_response(id, comp_list);
        return;
    }

    // 1. CLASS_NAME_DECL: when typing new class name, do NOT suggest existing class names
    if (ctx == CompletionContext::CLASS_NAME_DECL) {
        // Return empty or non-class items (e.g. no existing class names)
        transport_.send_response(id, comp_list);
        return;
    }

    // 2. EXTENDS: Suggest non-interface classes only
    if (ctx == CompletionContext::EXTENDS) {
        if (last_context_) {
            for (const auto& [src, nodes] : last_context_->nodes) {
                for (const auto& n : nodes) {
                    if (n && n->node_type == NodeType::CLASS_DECL) {
                        auto* cd = static_cast<ClassDeclaration*>(n.get());
                        if (!cd->is_interface) {
                            CompletionItem item;
                            item.label = cd->class_name;
                            item.kind = static_cast<int>(CompletionItemKind::Class);
                            item.insertText = cd->class_name;
                            comp_list.items.push_back(item);
                        }
                    }
                }
            }
        }
        transport_.send_response(id, comp_list);
        return;
    }

    // 3. IMPLEMENTS: Suggest interfaces only
    if (ctx == CompletionContext::IMPLEMENTS) {
        if (last_context_) {
            for (const auto& [src, nodes] : last_context_->nodes) {
                for (const auto& n : nodes) {
                    if (n && n->node_type == NodeType::CLASS_DECL) {
                        auto* cd = static_cast<ClassDeclaration*>(n.get());
                        if (cd->is_interface) {
                            CompletionItem item;
                            item.label = cd->class_name;
                            item.kind = static_cast<int>(CompletionItemKind::Interface);
                            item.insertText = cd->class_name;
                            comp_list.items.push_back(item);
                        }
                    }
                }
            }
        }
        transport_.send_response(id, comp_list);
        return;
    }

    // 4. NEW_INSTANCE: Suggest instantiable classes with "()" constructor snippet
    if (ctx == CompletionContext::NEW_INSTANCE) {
        if (last_context_) {
            for (const auto& [src, nodes] : last_context_->nodes) {
                for (const auto& n : nodes) {
                    if (n && n->node_type == NodeType::CLASS_DECL) {
                        auto* cd = static_cast<ClassDeclaration*>(n.get());
                        if (!cd->is_interface && !cd->is_abstract) {
                            CompletionItem item;
                            item.label = cd->class_name;
                            item.kind = static_cast<int>(CompletionItemKind::Constructor);
                            item.detail = cd->class_name + "()";
                            item.insertText = cd->class_name + "()";
                            comp_list.items.push_back(item);
                        }
                    }
                }
            }
        }
        transport_.send_response(id, comp_list);
        return;
    }

    // 5. CASE_EXPR: Suggest enum members of the enclosing switch condition
    if (ctx == CompletionContext::CASE_EXPR) {
        if (last_context_) {
            std::string file_path = uri_to_path(uri);
            Node* hit = spatial_index_.find_node_at(file_path, line + 1, std::max(1, character));
            Node* curr = hit;
            while (curr && curr->node_type != NodeType::SWITCH_STMT) {
                curr = curr->parent;
            }

            std::string enum_type_name;
            if (curr && curr->node_type == NodeType::SWITCH_STMT) {
                auto* sw = static_cast<SwitchStatement*>(curr);
                if (sw->condition) {
                    if (sw->condition->resolved_declaration) {
                        Node* decl = sw->condition->resolved_declaration;
                        if (decl->node_type == NodeType::VAR_DECL) {
                            enum_type_name = static_cast<VariableDeclaration*>(decl)->type_info.name;
                        }
                    } else if (!sw->condition->expression_type.name.empty()) {
                        enum_type_name = sw->condition->expression_type.name;
                    }
                }
            }

            // Find all matching enum members (or all enum members if switch condition unresolved)
            for (const auto& [src, nodes] : last_context_->nodes) {
                for (const auto& n : nodes) {
                    if (n && n->node_type == NodeType::ENUM_DECL) {
                        auto* ed = static_cast<EnumDeclaration*>(n.get());
                        if (enum_type_name.empty() || ed->enum_name == enum_type_name) {
                            for (const auto& mem : ed->members) {
                                CompletionItem item;
                                item.label = ed->enum_name + "." + mem;
                                item.kind = static_cast<int>(CompletionItemKind::EnumMember);
                                item.detail = ed->enum_name;
                                item.insertText = ed->enum_name + "." + mem;
                                comp_list.items.push_back(item);
                            }
                        }
                    }
                }
            }
        }
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

        // 1. Enclosing callable (Method or Constructor): parameters and local variables
        Node* callable = find_enclosing_callable_at(file_path, line + 1, last_context_.get());
        if (callable) {
            std::function<void(Node*)> add_locals = [&](Node* n) {
                if (!n) return;
                if (n->node_type == NodeType::VAR_DECL) {
                    auto* vd = static_cast<VariableDeclaration*>(n);
                    CompletionItem item;
                    item.label = vd->var_name;
                    item.kind = static_cast<int>(CompletionItemKind::Variable);
                    item.detail = vd->type_info.to_string();
                    item.insertText = vd->var_name;
                    comp_list.items.push_back(item);
                }
                for (const auto& ch : n->children) {
                    add_locals(ch.get());
                }
            };

            if (callable->node_type == NodeType::METHOD_DECL) {
                auto* md = static_cast<MethodDeclaration*>(callable);
                for (const auto& param : md->parameters) {
                    CompletionItem item;
                    item.label = param->var_name;
                    item.kind = static_cast<int>(CompletionItemKind::Variable);
                    item.detail = param->type_info.to_string();
                    item.insertText = param->var_name;
                    comp_list.items.push_back(item);
                }
                for (const auto& ch : md->children) {
                    add_locals(ch.get());
                }
            } else if (callable->node_type == NodeType::CONSTRUCTOR_DECL) {
                auto* ctor = static_cast<ConstructorDeclaration*>(callable);
                for (const auto& param : ctor->parameters) {
                    CompletionItem item;
                    item.label = param->var_name;
                    item.kind = static_cast<int>(CompletionItemKind::Variable);
                    item.detail = param->type_info.to_string();
                    item.insertText = param->var_name;
                    comp_list.items.push_back(item);
                }
                for (const auto& ch : ctor->children) {
                    add_locals(ch.get());
                }
            }
        }

        // 2. Enclosing class: fields and methods
        ClassDeclaration* enc_cls = find_enclosing_class_at(file_path, line + 1, last_context_.get());
        if (enc_cls) {
            for (const auto& member : enc_cls->children) {
                if (!member) continue;
                if (member->node_type == NodeType::FIELD_DECL) {
                    auto* fd = static_cast<FieldDeclaration*>(member.get());
                    CompletionItem item;
                    item.label = fd->field_name;
                    item.kind = static_cast<int>(CompletionItemKind::Field);
                    item.detail = fd->type_info.to_string();
                    item.insertText = fd->field_name;
                    comp_list.items.push_back(item);
                } else if (member->node_type == NodeType::METHOD_DECL) {
                    auto* md = static_cast<MethodDeclaration*>(member.get());
                    CompletionItem item;
                    item.label = md->method_name;
                    item.kind = static_cast<int>(CompletionItemKind::Method);
                    item.detail = md->return_type.to_string() + " " + md->method_name + "()";
                    item.insertText = md->method_name + "()";
                    comp_list.items.push_back(item);
                }
            }
        }

        // 3. Add class and enum names across context
        for (const auto& [src, nodes] : last_context_->nodes) {
            for (const auto& n : nodes) {
                if (n && n->node_type == NodeType::CLASS_DECL) {
                    auto* cd = static_cast<ClassDeclaration*>(n.get());
                    CompletionItem item;
                    item.label = cd->class_name;
                    item.kind = cd->is_interface ? static_cast<int>(CompletionItemKind::Interface) : static_cast<int>(CompletionItemKind::Class);
                    item.insertText = cd->class_name;
                    comp_list.items.push_back(item);
                } else if (n && n->node_type == NodeType::ENUM_DECL) {
                    auto* ed = static_cast<EnumDeclaration*>(n.get());
                    CompletionItem item;
                    item.label = ed->enum_name;
                    item.kind = static_cast<int>(CompletionItemKind::Enum);
                    item.insertText = ed->enum_name;
                    comp_list.items.push_back(item);
                } else if (n && n->node_type == NodeType::ALIAS_STMT) {
                    auto* as = static_cast<AliasStatement*>(n.get());
                    CompletionItem item;
                    item.label = as->alias_name;
                    item.kind = static_cast<int>(CompletionItemKind::Reference);
                    item.detail = "alias " + as->alias_name + " = " + as->target_type.to_string();
                    item.insertText = as->alias_name;
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
