#pragma once

#include <iostream>
#include <string>
#include <filesystem>
#include <memory>
#include <vector>
#include <atomic>
#include <nlohmann/json.hpp>
#include "solix/lsp/transport.hpp"
#include "solix/lsp/document_store.hpp"
#include "solix/lsp/protocol.hpp"

namespace solix::lsp {

class LspServer {
public:
    LspServer(std::istream& in, std::ostream& out);
    ~LspServer() = default;

    // Runs main JSON-RPC message processing loop until shutdown/exit
    int run();

    // Directly process a parsed JSON-RPC message (useful for synchronous unit tests)
    void process_message(const nlohmann::json& msg);

    // Trigger analysis and diagnostics for a specific open document URI
    void analyze_document(const std::string& uri);

    // Accessors
    const DocumentStore& document_store() const { return docs_; }
    DocumentStore& document_store() { return docs_; }
    const std::filesystem::path& workspace_root() const { return workspace_root_; }
    void set_workspace_root(const std::filesystem::path& root);

private:
    void handle_initialize(const nlohmann::json& id, const nlohmann::json& params);
    void handle_shutdown(const nlohmann::json& id);
    void handle_exit();
    void handle_did_open(const nlohmann::json& params);
    void handle_did_change(const nlohmann::json& params);
    void handle_did_close(const nlohmann::json& params);
    void handle_did_save(const nlohmann::json& params);

    void load_project_dependencies();

    JsonRpcTransport transport_;
    DocumentStore docs_;
    std::filesystem::path workspace_root_;
    std::filesystem::path manifest_path_;
    nlohmann::json manifest_;
    std::vector<std::filesystem::path> dependency_roots_;
    std::atomic<bool> is_initialized_{false};
    std::atomic<bool> is_shutdown_{false};
    std::atomic<bool> should_exit_{false};
    int exit_code_{0};
};

} // namespace solix::lsp
