#include <catch2/catch_test_macros.hpp>
#include "solix/lsp/protocol.hpp"
#include "solix/lsp/transport.hpp"
#include "solix/lsp/server.hpp"
#include <sstream>
#include <filesystem>
#include <fstream>

using namespace solix::lsp;

TEST_CASE("Suite 10: LSP Command & Server Functionality", "[lsp][commands]") {

    SECTION("Case 10.1: LSP Lifecycle Handshake") {
        std::stringstream input_stream;
        std::stringstream output_stream;

        JsonRpcTransport client_transport(output_stream, input_stream);

        // Send initialize
        nlohmann::json init_params = {
            {"rootUri", "file:///tmp/workspace"},
            {"capabilities", nlohmann::json::object()}
        };
        client_transport.send_notification("initialize", init_params);
        // Note: initialize as request has id
        nlohmann::json init_req = {
            {"jsonrpc", "2.0"},
            {"id", 1},
            {"method", "initialize"},
            {"params", init_params}
        };
        client_transport.write_message(init_req);

        // Send initialized notification
        client_transport.send_notification("initialized", nlohmann::json::object());

        // Send shutdown request
        nlohmann::json shutdown_req = {
            {"jsonrpc", "2.0"},
            {"id", 2},
            {"method", "shutdown"},
            {"params", nlohmann::json::object()}
        };
        client_transport.write_message(shutdown_req);

        // Send exit notification
        client_transport.send_notification("exit", nlohmann::json::object());

        // Run server on stream
        LspServer server(input_stream, output_stream);
        int exit_code = server.run();

        REQUIRE(exit_code == 0);

        // Read responses from server in output_stream
        JsonRpcTransport server_reader(output_stream, input_stream);

        auto resp1 = server_reader.read_message();
        REQUIRE(resp1.has_value());
        REQUIRE(resp1.value().value("id", 0) == 1);
        REQUIRE(resp1.value().contains("result"));
        REQUIRE(resp1.value()["result"]["capabilities"]["textDocumentSync"] == 1);

        auto resp2 = server_reader.read_message();
        REQUIRE(resp2.has_value());
        REQUIRE(resp2.value().value("id", 0) == 2);
        REQUIRE(resp2.value()["result"].is_null());
    }

    SECTION("Case 10.2: LSP Document Sync & Clean Diagnostics Publication") {
        std::stringstream input_stream;
        std::stringstream output_stream;

        LspServer server(input_stream, output_stream);

        std::string sample_code = "class Test { int32 x; void run() { int32 y = 10; } }";
        std::string doc_uri = "file:///test/Test.slx";

        nlohmann::json open_msg = {
            {"jsonrpc", "2.0"},
            {"method", "textDocument/didOpen"},
            {"params", {
                {"textDocument", {
                    {"uri", doc_uri},
                    {"languageId", "solix"},
                    {"version", 1},
                    {"text", sample_code}
                }}
            }}
        };

        server.process_message(open_msg);

        // Read published diagnostics
        JsonRpcTransport reader(output_stream, input_stream);
        auto notif = reader.read_message();
        REQUIRE(notif.has_value());
        REQUIRE(notif.value()["method"] == "textDocument/publishDiagnostics");
        REQUIRE(notif.value()["params"]["uri"] == doc_uri);
        REQUIRE(notif.value()["params"]["diagnostics"].empty());
    }

    SECTION("Case 10.3: LSP Live Syntax Error Diagnostics") {
        std::stringstream input_stream;
        std::stringstream output_stream;

        LspServer server(input_stream, output_stream);

        std::string broken_code = "class Test { void run() { int32 a = ; } }";
        std::string doc_uri = "file:///test/Broken.slx";

        nlohmann::json open_msg = {
            {"jsonrpc", "2.0"},
            {"method", "textDocument/didOpen"},
            {"params", {
                {"textDocument", {
                    {"uri", doc_uri},
                    {"languageId", "solix"},
                    {"version", 1},
                    {"text", broken_code}
                }}
            }}
        };

        server.process_message(open_msg);

        JsonRpcTransport reader(output_stream, input_stream);
        auto notif = reader.read_message();
        REQUIRE(notif.has_value());
        REQUIRE(notif.value()["method"] == "textDocument/publishDiagnostics");
        REQUIRE(notif.value()["params"]["uri"] == doc_uri);

        const auto& diags = notif.value()["params"]["diagnostics"];
        REQUIRE_FALSE(diags.empty());
        REQUIRE(diags[0]["severity"] == 1); // Error
        REQUIRE(diags[0].contains("range"));
        REQUIRE(diags[0]["range"]["start"]["line"].is_number());
    }

    SECTION("Case 10.4: LSP Live Diagnostic Resolution on Edit") {
        std::stringstream input_stream;
        std::stringstream output_stream;

        LspServer server(input_stream, output_stream);

        std::string broken_code = "class Test { void run() { int32 a = ; } }";
        std::string fixed_code = "class Test { void run() { int32 a = 42; } }";
        std::string doc_uri = "file:///test/Edit.slx";

        nlohmann::json open_msg = {
            {"jsonrpc", "2.0"},
            {"method", "textDocument/didOpen"},
            {"params", {
                {"textDocument", {
                    {"uri", doc_uri},
                    {"languageId", "solix"},
                    {"version", 1},
                    {"text", broken_code}
                }}
            }}
        };
        server.process_message(open_msg);

        // Read initial error diagnostic
        JsonRpcTransport reader(output_stream, input_stream);
        auto notif1 = reader.read_message();
        REQUIRE(notif1.has_value());
        REQUIRE_FALSE(notif1.value()["params"]["diagnostics"].empty());

        // Send didChange with fixed code
        nlohmann::json change_msg = {
            {"jsonrpc", "2.0"},
            {"method", "textDocument/didChange"},
            {"params", {
                {"textDocument", {
                    {"uri", doc_uri},
                    {"version", 2}
                }},
                {"contentChanges", {
                    {{"text", fixed_code}}
                }}
            }}
        };
        server.process_message(change_msg);

        // Read cleared diagnostic
        auto notif2 = reader.read_message();
        REQUIRE(notif2.has_value());
        REQUIRE(notif2.value()["method"] == "textDocument/publishDiagnostics");
        REQUIRE(notif2.value()["params"]["diagnostics"].empty());
    }

    SECTION("Case 10.5: LSP Project Manifest Integration (solix.json)") {
        // Create a temporary project directory
        std::filesystem::path temp_dir = std::filesystem::temp_directory_path() / "solix_lsp_proj_test";
        std::error_code ec;
        std::filesystem::remove_all(temp_dir, ec);
        std::filesystem::create_directories(temp_dir / "src");

        // Write solix.json
        nlohmann::json manifest = {
            {"project", "lsp_project"},
            {"version", "1.0.0"},
            {"profiles", {
                {"debug", {
                    {"compilation", {
                        {"entry_point", "main"}
                    }}
                }}
            }}
        };
        {
            std::ofstream mf(temp_dir / "solix.json");
            mf << manifest.dump(4);
        }

        // Initialize server with workspace root
        std::stringstream input_stream;
        std::stringstream output_stream;
        LspServer server(input_stream, output_stream);

        nlohmann::json init_msg = {
            {"jsonrpc", "2.0"},
            {"id", 1},
            {"method", "initialize"},
            {"params", {
                {"rootUri", path_to_uri(temp_dir.string())}
            }}
        };
        server.process_message(init_msg);

        // Open a file in the project
        std::filesystem::path src_file = temp_dir / "src" / "Main.slx";
        std::string doc_uri = path_to_uri(src_file.string());
        std::string src_content = "class Main { void main() { int32 x = 100; } }";

        nlohmann::json open_msg = {
            {"jsonrpc", "2.0"},
            {"method", "textDocument/didOpen"},
            {"params", {
                {"textDocument", {
                    {"uri", doc_uri},
                    {"languageId", "solix"},
                    {"version", 1},
                    {"text", src_content}
                }}
            }}
        };
        server.process_message(open_msg);

        JsonRpcTransport reader(output_stream, input_stream);
        // Consume init response
        auto init_resp = reader.read_message();
        REQUIRE(init_resp.has_value());

        // Read publishDiagnostics
        auto notif = reader.read_message();
        REQUIRE(notif.has_value());
        REQUIRE(notif.value()["method"] == "textDocument/publishDiagnostics");
        REQUIRE(notif.value()["params"]["diagnostics"].empty());

        // Cleanup
        std::filesystem::remove_all(temp_dir, ec);
    }
}
