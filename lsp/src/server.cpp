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
            {"hoverProvider", false},
            {"definitionProvider", false},
            {"completionProvider", {
                {"resolveProvider", false},
                {"triggerCharacters", {".", "::"}}
            }}
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

    // 3. Run frontend analysis pipeline: Lexer -> Parser -> Binder
    CompilationContext context(opts);
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

} // namespace solix::lsp
