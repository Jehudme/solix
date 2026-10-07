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

TEST_CASE("Suite 11: LSP Navigation & Inspection (Definitions, Type-Definitions & Hover)", "[lsp][navigation]") {
    std::stringstream input_stream;
    std::stringstream output_stream;

    LspServer server(input_stream, output_stream);

    std::string source_text = 
        "class Helper {\n"
        "    int32 count;\n"
        "    public void compute() {\n"
        "        int32 x = 5;\n"
        "        int32 y = x + 1;\n"
        "    }\n"
        "}\n"
        "class Main {\n"
        "    public void run() {\n"
        "        Helper h = new Helper();\n"
        "        h.compute();\n"
        "    }\n"
        "}\n";

    std::string doc_uri = "file:///test/NavTest.slx";

    nlohmann::json open_msg = {
        {"jsonrpc", "2.0"},
        {"method", "textDocument/didOpen"},
        {"params", {
            {"textDocument", {
                {"uri", doc_uri},
                {"languageId", "solix"},
                {"version", 1},
                {"text", source_text}
            }}
        }}
    };
    server.process_message(open_msg);

    // Drain the publishDiagnostics notification from didOpen
    JsonRpcTransport reader(output_stream, input_stream);
    auto diag_notif = reader.read_message();
    REQUIRE(diag_notif.has_value());

    SECTION("Case 11.1: Go-to-Definition for Local Variables") {
        // Line 5: int32 y = x + 1; where 'x' is at line 4 (0-indexed line 4, char 18)
        nlohmann::json def_req = {
            {"jsonrpc", "2.0"},
            {"id", 101},
            {"method", "textDocument/definition"},
            {"params", {
                {"textDocument", {{"uri", doc_uri}}},
                {"position", {{"line", 4}, {"character", 18}}}
            }}
        };
        server.process_message(def_req);

        auto def_resp = reader.read_message();
        REQUIRE(def_resp.has_value());
        REQUIRE(def_resp.value().value("id", 0) == 101);
        REQUIRE_FALSE(def_resp.value()["result"].is_null());

        const auto& result = def_resp.value()["result"];
        REQUIRE(result.contains("range"));
        // Definition should point to 'int32 x = 5;' at line 3 (0-indexed)
        REQUIRE(result["range"]["start"]["line"] == 3);
    }

    SECTION("Case 11.2: Go-to-Definition for Class Declarations & Methods") {
        // Line 10 (0-indexed line 10): "        h.compute();" -> 'compute' starts at index 10
        nlohmann::json def_req = {
            {"jsonrpc", "2.0"},
            {"id", 102},
            {"method", "textDocument/definition"},
            {"params", {
                {"textDocument", {{"uri", doc_uri}}},
                {"position", {{"line", 10}, {"character", 11}}}
            }}
        };
        server.process_message(def_req);

        auto def_resp = reader.read_message();
        REQUIRE(def_resp.has_value());
        REQUIRE(def_resp.value().value("id", 0) == 102);
        REQUIRE_FALSE(def_resp.value()["result"].is_null());

        const auto& result = def_resp.value()["result"];
        // Should point to 'void compute()' at line 2 (0-indexed)
        REQUIRE(result["range"]["start"]["line"] == 2);
    }

    SECTION("Case 11.3: Go-to-Type-Definition for Instance Expressions") {
        // Line 9 (0-indexed line 9): "        Helper h = new Helper();" -> variable 'h' is at index 15
        nlohmann::json type_def_req = {
            {"jsonrpc", "2.0"},
            {"id", 103},
            {"method", "textDocument/typeDefinition"},
            {"params", {
                {"textDocument", {{"uri", doc_uri}}},
                {"position", {{"line", 9}, {"character", 15}}}
            }}
        };
        server.process_message(type_def_req);

        auto type_resp = reader.read_message();
        REQUIRE(type_resp.has_value());
        REQUIRE(type_resp.value().value("id", 0) == 103);
        REQUIRE_FALSE(type_resp.value()["result"].is_null());

        const auto& result = type_resp.value()["result"];
        // Should point to 'class Helper' declaration at line 0 (0-indexed)
        REQUIRE(result["range"]["start"]["line"] == 0);
    }

    SECTION("Case 11.4: Hover Tooltip for Variables, Methods, and Primitives") {
        // Line 2: public void compute() (line 2, char 16)
        nlohmann::json hover_req = {
            {"jsonrpc", "2.0"},
            {"id", 104},
            {"method", "textDocument/hover"},
            {"params", {
                {"textDocument", {{"uri", doc_uri}}},
                {"position", {{"line", 2}, {"character", 16}}}
            }}
        };
        server.process_message(hover_req);

        auto hover_resp = reader.read_message();
        REQUIRE(hover_resp.has_value());
        REQUIRE(hover_resp.value().value("id", 0) == 104);
        REQUIRE_FALSE(hover_resp.value()["result"].is_null());

        const auto& result = hover_resp.value()["result"];
        REQUIRE(result.contains("contents"));
        std::string markdown = result["contents"]["value"];
        REQUIRE_FALSE(markdown.empty());
        REQUIRE(markdown.find("compute") != std::string::npos);
    }

    SECTION("Case 11.5: Definition and Hover on Whitespace / Unresolved Tokens") {
        // Line 12: outside class / whitespace
        nlohmann::json null_req = {
            {"jsonrpc", "2.0"},
            {"id", 105},
            {"method", "textDocument/definition"},
            {"params", {
                {"textDocument", {{"uri", doc_uri}}},
                {"position", {{"line", 13}, {"character", 0}}}
            }}
        };
        server.process_message(null_req);

        auto null_resp = reader.read_message();
        REQUIRE(null_resp.has_value());
        REQUIRE(null_resp.value().value("id", 0) == 105);
        REQUIRE(null_resp.value()["result"].is_null());
    }

    SECTION("Case 11.6: Multi-File Cross-Unit Go-to-Definition") {
        std::stringstream in_s;
        std::stringstream out_s;
        LspServer srv(in_s, out_s);
        JsonRpcTransport rdr(out_s, in_s);

        std::string helper_uri = "file:///test/Helper.slx";
        std::string main_uri = "file:///test/Main.slx";

        std::string helper_src =
            "class ExternalHelper {\n"
            "    public void externalAction() {}\n"
            "}\n";

        std::string main_src =
            "class App {\n"
            "    public void run() {\n"
            "        ExternalHelper eh = new ExternalHelper();\n"
            "        eh.externalAction();\n"
            "    }\n"
            "}\n";

        nlohmann::json open_helper = {
            {"jsonrpc", "2.0"},
            {"method", "textDocument/didOpen"},
            {"params", {
                {"textDocument", {
                    {"uri", helper_uri},
                    {"languageId", "solix"},
                    {"version", 1},
                    {"text", helper_src}
                }}
            }}
        };
        srv.process_message(open_helper);

        nlohmann::json open_main = {
            {"jsonrpc", "2.0"},
            {"method", "textDocument/didOpen"},
            {"params", {
                {"textDocument", {
                    {"uri", main_uri},
                    {"languageId", "solix"},
                    {"version", 1},
                    {"text", main_src}
                }}
            }}
        };
        srv.process_message(open_main);

        // Drain publishDiagnostics notifications from open_helper and open_main
        auto diag1 = rdr.read_message();
        REQUIRE(diag1.has_value());
        auto diag2 = rdr.read_message();
        REQUIRE(diag2.has_value());

        // Go to definition of 'externalAction' in Main.slx (line 3, character 12)
        nlohmann::json def_req = {
            {"jsonrpc", "2.0"},
            {"id", 106},
            {"method", "textDocument/definition"},
            {"params", {
                {"textDocument", {{"uri", main_uri}}},
                {"position", {{"line", 3}, {"character", 12}}}
            }}
        };
        srv.process_message(def_req);

        auto def_resp = rdr.read_message();
        REQUIRE(def_resp.has_value());
        REQUIRE(def_resp.value().value("id", 0) == 106);
        REQUIRE_FALSE(def_resp.value()["result"].is_null());

        const auto& result = def_resp.value()["result"];
        REQUIRE(result.contains("uri"));
        std::string target_uri = result["uri"];
        // Must point to Helper.slx, NOT empty or file:///
        REQUIRE(target_uri.find("Helper.slx") != std::string::npos);
        REQUIRE(result.contains("range"));
        REQUIRE(result["range"]["start"]["line"] == 1);
    }

    SECTION("Case 11.7: TextMate Syntax Grammar Completeness") {
        std::filesystem::path tm_path = std::filesystem::path(SOLIX_PROJECT_ROOT) / "editors" / "vscode" / "syntaxes" / "solix.tmLanguage.json";
        std::ifstream f(tm_path);
        REQUIRE(f.is_open());
        nlohmann::json tm;
        f >> tm;

        std::string json_str = tm.dump();
        // Verify key declaration and control keywords exist
        REQUIRE(json_str.find("implements") != std::string::npos);
        REQUIRE(json_str.find("extends") != std::string::npos);
        REQUIRE(json_str.find("operator") != std::string::npos);
        REQUIRE(json_str.find("assert") != std::string::npos);
        REQUIRE(json_str.find("exit") != std::string::npos);
        REQUIRE(json_str.find("entity.name.type.class.solix") != std::string::npos);
    }

    SECTION("Case 11.8: Variable Reference to Declaration and Declaration-to-Type Chaining") {
        std::stringstream in_s;
        std::stringstream out_s;
        LspServer srv(in_s, out_s);
        JsonRpcTransport rdr(out_s, in_s);

        std::string chain_uri = "file:///test/Chaining.slx";
        std::string chain_src =
            "class Calculator {\n"
            "    public int32 add(int32 a, int32 b) {\n"
            "        return a + b;\n"
            "    }\n"
            "}\n"
            "class App {\n"
            "    public void run() {\n"
            "        Calculator calc = new Calculator();\n"
            "        calc.add(1, 2);\n"
            "    }\n"
            "}\n";

        nlohmann::json open_msg = {
            {"jsonrpc", "2.0"},
            {"method", "textDocument/didOpen"},
            {"params", {
                {"textDocument", {
                    {"uri", chain_uri},
                    {"languageId", "solix"},
                    {"version", 1},
                    {"text", chain_src}
                }}
            }}
        };
        srv.process_message(open_msg);
        auto diag = rdr.read_message();
        REQUIRE(diag.has_value());

        // Step 1: Definition on variable usage 'calc' in 'calc.add(1, 2);' (line 8, char 9)
        nlohmann::json ref_req = {
            {"jsonrpc", "2.0"},
            {"id", 201},
            {"method", "textDocument/definition"},
            {"params", {
                {"textDocument", {{"uri", chain_uri}}},
                {"position", {{"line", 8}, {"character", 9}}}
            }}
        };
        srv.process_message(ref_req);
        auto ref_resp = rdr.read_message();
        REQUIRE(ref_resp.has_value());
        REQUIRE_FALSE(ref_resp.value()["result"].is_null());
        // Jumps to line 7 ('Calculator calc = new Calculator();')
        REQUIRE(ref_resp.value()["result"]["range"]["start"]["line"] == 7);

        // Step 2: Definition on declaration identifier 'calc' at line 7, char 20
        nlohmann::json decl_req = {
            {"jsonrpc", "2.0"},
            {"id", 202},
            {"method", "textDocument/definition"},
            {"params", {
                {"textDocument", {{"uri", chain_uri}}},
                {"position", {{"line", 7}, {"character", 20}}}
            }}
        };
        srv.process_message(decl_req);
        auto decl_resp = rdr.read_message();
        REQUIRE(decl_resp.has_value());
        REQUIRE_FALSE(decl_resp.value()["result"].is_null());
        // Chains to line 0 ('class Calculator')
        REQUIRE(decl_resp.value()["result"]["range"]["start"]["line"] == 0);
    }

    SECTION("Case 11.9: Type Annotation Go-to-Definition Across Constructs") {
        std::stringstream in_s;
        std::stringstream out_s;
        LspServer srv(in_s, out_s);
        JsonRpcTransport rdr(out_s, in_s);

        std::string type_uri = "file:///test/TypeNav.slx";
        std::string type_src =
            "class Calculator {\n"
            "}\n"
            "class App {\n"
            "    public void run() {\n"
            "        Calculator calc = new Calculator();\n"
            "        try {\n"
            "            Calculator c2 = (Calculator) calc;\n"
            "        } catch (Calculator err) {\n"
            "        }\n"
            "    }\n"
            "}\n";

        nlohmann::json open_msg = {
            {"jsonrpc", "2.0"},
            {"method", "textDocument/didOpen"},
            {"params", {
                {"textDocument", {
                    {"uri", type_uri},
                    {"languageId", "solix"},
                    {"version", 1},
                    {"text", type_src}
                }}
            }}
        };
        srv.process_message(open_msg);
        auto diag = rdr.read_message();
        REQUIRE(diag.has_value());

        // 1. Type annotation in variable declaration: 'Calculator' at line 4, char 10
        nlohmann::json var_type_req = {
            {"jsonrpc", "2.0"},
            {"id", 211},
            {"method", "textDocument/definition"},
            {"params", {
                {"textDocument", {{"uri", type_uri}}},
                {"position", {{"line", 4}, {"character", 10}}}
            }}
        };
        srv.process_message(var_type_req);
        auto var_type_resp = rdr.read_message();
        REQUIRE(var_type_resp.has_value());
        REQUIRE_FALSE(var_type_resp.value()["result"].is_null());
        REQUIRE(var_type_resp.value()["result"]["range"]["start"]["line"] == 0);

        // 2. Type in new expression: 'Calculator' at line 4, char 31
        nlohmann::json new_type_req = {
            {"jsonrpc", "2.0"},
            {"id", 212},
            {"method", "textDocument/definition"},
            {"params", {
                {"textDocument", {{"uri", type_uri}}},
                {"position", {{"line", 4}, {"character", 31}}}
            }}
        };
        srv.process_message(new_type_req);
        auto new_type_resp = rdr.read_message();
        REQUIRE(new_type_resp.has_value());
        REQUIRE_FALSE(new_type_resp.value()["result"].is_null());
        REQUIRE(new_type_resp.value()["result"]["range"]["start"]["line"] == 0);

        // 3. Type in cast expression: 'Calculator' at line 6, char 30
        nlohmann::json cast_type_req = {
            {"jsonrpc", "2.0"},
            {"id", 213},
            {"method", "textDocument/definition"},
            {"params", {
                {"textDocument", {{"uri", type_uri}}},
                {"position", {{"line", 6}, {"character", 30}}}
            }}
        };
        srv.process_message(cast_type_req);
        auto cast_type_resp = rdr.read_message();
        REQUIRE(cast_type_resp.has_value());
        REQUIRE_FALSE(cast_type_resp.value()["result"].is_null());
        REQUIRE(cast_type_resp.value()["result"]["range"]["start"]["line"] == 0);

        // 4. Type in catch clause: 'Calculator' at line 7, char 18
        nlohmann::json catch_type_req = {
            {"jsonrpc", "2.0"},
            {"id", 214},
            {"method", "textDocument/definition"},
            {"params", {
                {"textDocument", {{"uri", type_uri}}},
                {"position", {{"line", 7}, {"character", 18}}}
            }}
        };
        srv.process_message(catch_type_req);
        auto catch_type_resp = rdr.read_message();
        REQUIRE(catch_type_resp.has_value());
        REQUIRE_FALSE(catch_type_resp.value()["result"].is_null());
        REQUIRE(catch_type_resp.value()["result"]["range"]["start"]["line"] == 0);
    }

    SECTION("Case 11.10: Inheritance & Override Definition Navigation") {
        std::stringstream in_s;
        std::stringstream out_s;
        LspServer srv(in_s, out_s);
        JsonRpcTransport rdr(out_s, in_s);

        std::string hier_uri = "file:///test/Hier.slx";
        std::string hier_src =
            "interface IRunner {\n"
            "    void run();\n"
            "}\n"
            "class BaseWorker {\n"
            "    public virtual void run() {}\n"
            "}\n"
            "class FastWorker extends BaseWorker implements IRunner {\n"
            "    public override void run() {}\n"
            "}\n";

        nlohmann::json open_msg = {
            {"jsonrpc", "2.0"},
            {"method", "textDocument/didOpen"},
            {"params", {
                {"textDocument", {
                    {"uri", hier_uri},
                    {"languageId", "solix"},
                    {"version", 1},
                    {"text", hier_src}
                }}
            }}
        };
        srv.process_message(open_msg);
        auto diag = rdr.read_message();
        REQUIRE(diag.has_value());

        // 1. Go-to-definition on 'BaseWorker' in extends clause (line 6, char 27)
        nlohmann::json ext_req = {
            {"jsonrpc", "2.0"},
            {"id", 221},
            {"method", "textDocument/definition"},
            {"params", {
                {"textDocument", {{"uri", hier_uri}}},
                {"position", {{"line", 6}, {"character", 27}}}
            }}
        };
        srv.process_message(ext_req);
        auto ext_resp = rdr.read_message();
        REQUIRE(ext_resp.has_value());
        REQUIRE_FALSE(ext_resp.value()["result"].is_null());
        REQUIRE(ext_resp.value()["result"]["range"]["start"]["line"] == 3);

        // 2. Go-to-definition on 'IRunner' in implements clause (line 6, char 49)
        nlohmann::json impl_req = {
            {"jsonrpc", "2.0"},
            {"id", 222},
            {"method", "textDocument/definition"},
            {"params", {
                {"textDocument", {{"uri", hier_uri}}},
                {"position", {{"line", 6}, {"character", 49}}}
            }}
        };
        srv.process_message(impl_req);
        auto impl_resp = rdr.read_message();
        REQUIRE(impl_resp.has_value());
        REQUIRE_FALSE(impl_resp.value()["result"].is_null());
        REQUIRE(impl_resp.value()["result"]["range"]["start"]["line"] == 0);

        // 3. Go-to-definition on 'override void run()' in FastWorker (line 7, char 26)
        nlohmann::json over_req = {
            {"jsonrpc", "2.0"},
            {"id", 223},
            {"method", "textDocument/definition"},
            {"params", {
                {"textDocument", {{"uri", hier_uri}}},
                {"position", {{"line", 7}, {"character", 26}}}
            }}
        };
        srv.process_message(over_req);
        auto over_resp = rdr.read_message();
        REQUIRE(over_resp.has_value());
        REQUIRE_FALSE(over_resp.value()["result"].is_null());
        // Resolves to BaseWorker::run() at line 4
        REQUIRE(over_resp.value()["result"]["range"]["start"]["line"] == 4);
    }
}

TEST_CASE("Suite 12: LSP Intelligence (Completions, Signature Help & Document Symbols)", "[lsp][intelligence]") {
    std::stringstream input_stream;
    std::stringstream output_stream;

    LspServer server(input_stream, output_stream);

    std::string source_text = 
        "class Helper {\n"
        "    public int32 count;\n"
        "    public void compute(int32 delta, string tag) {\n"
        "        int32 x = delta + 1;\n"
        "    }\n"
        "}\n"
        "class Main {\n"
        "    public void run() {\n"
        "        Helper h = new Helper();\n"
        "        h.compute(10, \"test\");\n"
        "    }\n"
        "}\n";

    std::string doc_uri = "file:///test/IntelTest.slx";

    nlohmann::json open_msg = {
        {"jsonrpc", "2.0"},
        {"method", "textDocument/didOpen"},
        {"params", {
            {"textDocument", {
                {"uri", doc_uri},
                {"languageId", "solix"},
                {"version", 1},
                {"text", source_text}
            }}
        }}
    };
    server.process_message(open_msg);

    JsonRpcTransport reader(output_stream, input_stream);
    auto diag_notif = reader.read_message();
    REQUIRE(diag_notif.has_value());

    SECTION("Case 12.1: Member Completion on Dot Access") {
        // Line 9 (0-indexed line 9): "        h.compute(10, \"test\");" -> right after 'h.' is col 10
        nlohmann::json comp_req = {
            {"jsonrpc", "2.0"},
            {"id", 201},
            {"method", "textDocument/completion"},
            {"params", {
                {"textDocument", {{"uri", doc_uri}}},
                {"position", {{"line", 9}, {"character", 10}}}
            }}
        };
        server.process_message(comp_req);

        auto comp_resp = reader.read_message();
        REQUIRE(comp_resp.has_value());
        REQUIRE(comp_resp.value().value("id", 0) == 201);
        const auto& result = comp_resp.value()["result"];
        REQUIRE(result.contains("items"));
        REQUIRE(result["items"].is_array());

        bool found_compute = false;
        bool found_count = false;
        for (const auto& item : result["items"]) {
            std::string label = item.value("label", "");
            if (label == "compute") {
                found_compute = true;
                REQUIRE(item.value("kind", 0) == 2); // CompletionItemKind::Method
            } else if (label == "count") {
                found_count = true;
                REQUIRE(item.value("kind", 0) == 5); // CompletionItemKind::Field
            }
        }
        REQUIRE(found_compute);
        REQUIRE(found_count);
    }

    SECTION("Case 12.2: Scope & Keyword Completion") {
        // Inside run() body at line 8, char 8
        nlohmann::json comp_req = {
            {"jsonrpc", "2.0"},
            {"id", 202},
            {"method", "textDocument/completion"},
            {"params", {
                {"textDocument", {{"uri", doc_uri}}},
                {"position", {{"line", 8}, {"character", 8}}}
            }}
        };
        server.process_message(comp_req);

        auto comp_resp = reader.read_message();
        REQUIRE(comp_resp.has_value());
        REQUIRE(comp_resp.value().value("id", 0) == 202);
        const auto& result = comp_resp.value()["result"];
        REQUIRE(result.contains("items"));

        bool found_kw = false;
        bool found_class = false;
        for (const auto& item : result["items"]) {
            std::string label = item.value("label", "");
            if (label == "return" || label == "class" || label == "if") {
                found_kw = true;
            }
            if (label == "Helper" || label == "Main") {
                found_class = true;
            }
        }
        REQUIRE(found_kw);
        REQUIRE(found_class);
    }

    SECTION("Case 12.3: Signature Help on Method Call") {
        // Inside h.compute(10, "test") at char 19 (after comma, activeParameter = 1)
        nlohmann::json sig_req = {
            {"jsonrpc", "2.0"},
            {"id", 203},
            {"method", "textDocument/signatureHelp"},
            {"params", {
                {"textDocument", {{"uri", doc_uri}}},
                {"position", {{"line", 9}, {"character", 22}}}
            }}
        };
        server.process_message(sig_req);

        auto sig_resp = reader.read_message();
        REQUIRE(sig_resp.has_value());
        REQUIRE(sig_resp.value().value("id", 0) == 203);
        REQUIRE_FALSE(sig_resp.value()["result"].is_null());

        const auto& result = sig_resp.value()["result"];
        REQUIRE(result.contains("signatures"));
        REQUIRE(!result["signatures"].empty());
        REQUIRE(result["signatures"][0]["label"].get<std::string>().find("compute") != std::string::npos);
        REQUIRE(result["signatures"][0]["parameters"].size() == 2);
        REQUIRE(result["activeParameter"] == 1);
    }

    SECTION("Case 12.4: Hierarchical Document Symbols Outline") {
        nlohmann::json sym_req = {
            {"jsonrpc", "2.0"},
            {"id", 204},
            {"method", "textDocument/documentSymbol"},
            {"params", {
                {"textDocument", {{"uri", doc_uri}}}
            }}
        };
        server.process_message(sym_req);

        auto sym_resp = reader.read_message();
        REQUIRE(sym_resp.has_value());
        REQUIRE(sym_resp.value().value("id", 0) == 204);
        REQUIRE(sym_resp.value()["result"].is_array());

        const auto& symbols = sym_resp.value()["result"];
        REQUIRE(symbols.size() >= 2);

        bool found_helper = false;
        bool found_main = false;
        for (const auto& s : symbols) {
            std::string name = s.value("name", "");
            if (name == "Helper") {
                found_helper = true;
                REQUIRE(s.value("kind", 0) == 5); // Class
                REQUIRE(s.contains("children"));
                REQUIRE(s["children"].size() >= 2); // count and compute
            } else if (name == "Main") {
                found_main = true;
                REQUIRE(s.value("kind", 0) == 5); // Class
                REQUIRE(s.contains("children"));
                REQUIRE(s["children"].size() >= 1); // run
            }
        }
        REQUIRE(found_helper);
        REQUIRE(found_main);
    }

    SECTION("Case 12.5: Completion on Unresolved Expression") {
        // Test didChange with invalid receiver "unknown."
        std::string modified_text = source_text + "\nclass Dummy { void test() { unknown. } }\n";
        nlohmann::json change_msg = {
            {"jsonrpc", "2.0"},
            {"method", "textDocument/didChange"},
            {"params", {
                {"textDocument", {{"uri", doc_uri}, {"version", 2}}},
                {"contentChanges", {{{"text", modified_text}}}}
            }}
        };
        server.process_message(change_msg);
        auto ch_notif = reader.read_message(); // drain publishDiagnostics

        nlohmann::json unk_req = {
            {"jsonrpc", "2.0"},
            {"id", 205},
            {"method", "textDocument/completion"},
            {"params", {
                {"textDocument", {{"uri", doc_uri}}},
                {"position", {{"line", 13}, {"character", 36}}}
            }}
        };
        server.process_message(unk_req);

        auto unk_resp = reader.read_message();
        REQUIRE(unk_resp.has_value());
        REQUIRE(unk_resp.value().value("id", 0) == 205);
        REQUIRE(unk_resp.value()["result"]["items"].empty());
    }

    SECTION("Case 12.6: Class Declaration Name Completion Suppression") {
        std::stringstream in_s;
        std::stringstream out_s;
        LspServer srv(in_s, out_s);
        JsonRpcTransport rdr(out_s, in_s);

        std::string test_uri = "file:///test/DeclComp.slx";
        std::string test_src = "class ExistingClass {}\nclass ";

        nlohmann::json open_doc = {
            {"jsonrpc", "2.0"},
            {"method", "textDocument/didOpen"},
            {"params", {
                {"textDocument", {
                    {"uri", test_uri},
                    {"languageId", "solix"},
                    {"version", 1},
                    {"text", test_src}
                }}
            }}
        };
        srv.process_message(open_doc);
        auto notif = rdr.read_message(); // drain publishDiagnostics

        // Cursor immediately after "class " on line 1, char 6
        nlohmann::json req = {
            {"jsonrpc", "2.0"},
            {"id", 206},
            {"method", "textDocument/completion"},
            {"params", {
                {"textDocument", {{"uri", test_uri}}},
                {"position", {{"line", 1}, {"character", 6}}}
            }}
        };
        srv.process_message(req);
        auto resp = rdr.read_message();
        REQUIRE(resp.has_value());
        REQUIRE(resp.value().value("id", 0) == 206);
        const auto& items = resp.value()["result"]["items"];
        // Must NOT suggest ExistingClass when declaring a new class name
        bool found_existing = false;
        for (const auto& item : items) {
            if (item.value("label", "") == "ExistingClass") {
                found_existing = true;
            }
        }
        REQUIRE_FALSE(found_existing);
    }

    SECTION("Case 12.7: Extends Context Class-Only Completion") {
        std::stringstream in_s;
        std::stringstream out_s;
        LspServer srv(in_s, out_s);
        JsonRpcTransport rdr(out_s, in_s);

        std::string test_uri = "file:///test/ExtendsComp.slx";
        std::string test_src =
            "interface IWorker {}\n"
            "class BaseService {}\n"
            "class ChildService extends \n";

        nlohmann::json open_doc = {
            {"jsonrpc", "2.0"},
            {"method", "textDocument/didOpen"},
            {"params", {
                {"textDocument", {
                    {"uri", test_uri},
                    {"languageId", "solix"},
                    {"version", 1},
                    {"text", test_src}
                }}
            }}
        };
        srv.process_message(open_doc);
        auto notif = rdr.read_message();

        // Cursor after "extends " on line 2, char 27
        nlohmann::json req = {
            {"jsonrpc", "2.0"},
            {"id", 207},
            {"method", "textDocument/completion"},
            {"params", {
                {"textDocument", {{"uri", test_uri}}},
                {"position", {{"line", 2}, {"character", 27}}}
            }}
        };
        srv.process_message(req);
        auto resp = rdr.read_message();
        REQUIRE(resp.has_value());
        REQUIRE(resp.value().value("id", 0) == 207);
        const auto& items = resp.value()["result"]["items"];

        bool found_base = false;
        bool found_interface = false;
        for (const auto& item : items) {
            std::string label = item.value("label", "");
            if (label == "BaseService") found_base = true;
            if (label == "IWorker") found_interface = true;
        }
        REQUIRE(found_base);
        REQUIRE_FALSE(found_interface); // interfaces excluded from extends
    }

    SECTION("Case 12.8: Implements Context Interface-Only Completion") {
        std::stringstream in_s;
        std::stringstream out_s;
        LspServer srv(in_s, out_s);
        JsonRpcTransport rdr(out_s, in_s);

        std::string test_uri = "file:///test/ImplComp.slx";
        std::string test_src =
            "interface IRunner {}\n"
            "class NormalClass {}\n"
            "class Worker implements \n";

        nlohmann::json open_doc = {
            {"jsonrpc", "2.0"},
            {"method", "textDocument/didOpen"},
            {"params", {
                {"textDocument", {
                    {"uri", test_uri},
                    {"languageId", "solix"},
                    {"version", 1},
                    {"text", test_src}
                }}
            }}
        };
        srv.process_message(open_doc);
        auto notif = rdr.read_message();

        // Cursor after "implements " on line 2, char 24
        nlohmann::json req = {
            {"jsonrpc", "2.0"},
            {"id", 208},
            {"method", "textDocument/completion"},
            {"params", {
                {"textDocument", {{"uri", test_uri}}},
                {"position", {{"line", 2}, {"character", 24}}}
            }}
        };
        srv.process_message(req);
        auto resp = rdr.read_message();
        REQUIRE(resp.has_value());
        REQUIRE(resp.value().value("id", 0) == 208);
        const auto& items = resp.value()["result"]["items"];

        bool found_interface = false;
        bool found_class = false;
        for (const auto& item : items) {
            std::string label = item.value("label", "");
            if (label == "IRunner") found_interface = true;
            if (label == "NormalClass") found_class = true;
        }
        REQUIRE(found_interface);
        REQUIRE_FALSE(found_class); // regular classes excluded from implements
    }

    SECTION("Case 12.9: New-Instance Completion with Constructor Snippets") {
        std::stringstream in_s;
        std::stringstream out_s;
        LspServer srv(in_s, out_s);
        JsonRpcTransport rdr(out_s, in_s);

        std::string test_uri = "file:///test/NewComp.slx";
        std::string test_src =
            "interface IService {}\n"
            "abstract class AbstractTask {}\n"
            "class ConcreteTask {}\n"
            "class App {\n"
            "    void run() {\n"
            "        var t = new \n"
            "    }\n"
            "}\n";

        nlohmann::json open_doc = {
            {"jsonrpc", "2.0"},
            {"method", "textDocument/didOpen"},
            {"params", {
                {"textDocument", {
                    {"uri", test_uri},
                    {"languageId", "solix"},
                    {"version", 1},
                    {"text", test_src}
                }}
            }}
        };
        srv.process_message(open_doc);
        auto notif = rdr.read_message();

        // Cursor after "new " on line 5, char 20
        nlohmann::json req = {
            {"jsonrpc", "2.0"},
            {"id", 209},
            {"method", "textDocument/completion"},
            {"params", {
                {"textDocument", {{"uri", test_uri}}},
                {"position", {{"line", 5}, {"character", 20}}}
            }}
        };
        srv.process_message(req);
        auto resp = rdr.read_message();
        REQUIRE(resp.has_value());
        REQUIRE(resp.value().value("id", 0) == 209);
        const auto& items = resp.value()["result"]["items"];

        bool found_concrete = false;
        bool found_abstract = false;
        bool found_interface = false;
        for (const auto& item : items) {
            std::string label = item.value("label", "");
            if (label == "ConcreteTask") {
                found_concrete = true;
                REQUIRE(item.value("insertText", "") == "ConcreteTask()");
            }
            if (label == "AbstractTask") found_abstract = true;
            if (label == "IService") found_interface = true;
        }
        REQUIRE(found_concrete);
        REQUIRE_FALSE(found_abstract);  // abstract classes excluded from new
        REQUIRE_FALSE(found_interface); // interfaces excluded from new
    }

    SECTION("Case 12.10: Case Context Enum Member Completion") {
        std::stringstream in_s;
        std::stringstream out_s;
        LspServer srv(in_s, out_s);
        JsonRpcTransport rdr(out_s, in_s);

        std::string test_uri = "file:///test/CaseComp.slx";
        std::string test_src =
            "enum Color { RED, GREEN, BLUE }\n"
            "class Switcher {\n"
            "    void test(Color c) {\n"
            "        switch (c) {\n"
            "            case \n"
            "        }\n"
            "    }\n"
            "}\n";

        nlohmann::json open_doc = {
            {"jsonrpc", "2.0"},
            {"method", "textDocument/didOpen"},
            {"params", {
                {"textDocument", {
                    {"uri", test_uri},
                    {"languageId", "solix"},
                    {"version", 1},
                    {"text", test_src}
                }}
            }}
        };
        srv.process_message(open_doc);
        auto notif = rdr.read_message();

        // Cursor after "case " on line 4, char 17
        nlohmann::json req = {
            {"jsonrpc", "2.0"},
            {"id", 210},
            {"method", "textDocument/completion"},
            {"params", {
                {"textDocument", {{"uri", test_uri}}},
                {"position", {{"line", 4}, {"character", 17}}}
            }}
        };
        srv.process_message(req);
        auto resp = rdr.read_message();
        REQUIRE(resp.has_value());
        REQUIRE(resp.value().value("id", 0) == 210);
        const auto& items = resp.value()["result"]["items"];

        bool found_red = false;
        bool found_green = false;
        bool found_blue = false;
        for (const auto& item : items) {
            std::string label = item.value("label", "");
            if (label == "Color.RED") found_red = true;
            if (label == "Color.GREEN") found_green = true;
            if (label == "Color.BLUE") found_blue = true;
        }
        REQUIRE(found_red);
        REQUIRE(found_green);
        REQUIRE(found_blue);
    }
}



// ============================================================================
// Suite 13: LSP Hover Signatures & Keyword Suppression (Phase 52)
// Cases 11.11 – 11.16
// ============================================================================

TEST_CASE("Suite 13: LSP Hover Signatures & Keyword Suppression", "[lsp][hover]") {
    // Source layout (0-indexed lines):
    // 0:  public interface IWorker {
    // 1:      void work(int32 amount);
    // 2:  }
    // 3:  public abstract class BaseJob {
    // 4:      protected static int32 total;
    // 5:      public virtual void doWork(int32 n, string label) {}
    // 6:  }
    // 7:  public class ConcreteJob extends BaseJob implements IWorker {
    // 8:      private const int32 limit;
    // 9:      public ConcreteJob(int32 cap, string name) {}
    // 10:     public override void doWork(int32 n, string label) {
    // 11:         const int32 localMax = 0;
    // 12:     }
    // 13:     public void work(int32 amount) {}
    // 14: }
    // 15: class Launcher {
    // 16:     public void start() {
    // 17:         ConcreteJob job = new ConcreteJob(10, "run");
    // 18:     }
    // 19: }
    std::string source_text =
        "public interface IWorker {\n"
        "    void work(int32 amount);\n"
        "}\n"
        "public abstract class BaseJob {\n"
        "    protected static int32 total;\n"
        "    public virtual void doWork(int32 n, string label) {}\n"
        "}\n"
        "public class ConcreteJob extends BaseJob implements IWorker {\n"
        "    private const int32 limit;\n"
        "    public ConcreteJob(int32 cap, string name) {}\n"
        "    public override void doWork(int32 n, string label) {\n"
        "        const int32 localMax = 0;\n"
        "    }\n"
        "    public void work(int32 amount) {}\n"
        "}\n"
        "class Launcher {\n"
        "    public void start() {\n"
        "        ConcreteJob job = new ConcreteJob(10, \"run\");\n"
        "    }\n"
        "}\n";

    std::string doc_uri = "file:///test/HoverSig.slx";

    std::stringstream input_stream;
    std::stringstream output_stream;
    LspServer server(input_stream, output_stream);

    nlohmann::json open_msg = {
        {"jsonrpc", "2.0"},
        {"method", "textDocument/didOpen"},
        {"params", {
            {"textDocument", {
                {"uri", doc_uri},
                {"languageId", "solix"},
                {"version", 1},
                {"text", source_text}
            }}
        }}
    };
    server.process_message(open_msg);

    JsonRpcTransport reader(output_stream, input_stream);
    auto diag_notif = reader.read_message(); // drain publishDiagnostics
    REQUIRE(diag_notif.has_value());

    // Helper lambda: send hover, return result JSON
    auto send_hover = [&](int id, int line, int character) -> nlohmann::json {
        nlohmann::json req = {
            {"jsonrpc", "2.0"},
            {"id", id},
            {"method", "textDocument/hover"},
            {"params", {
                {"textDocument", {{"uri", doc_uri}}},
                {"position", {{"line", line}, {"character", character}}}
            }}
        };
        server.process_message(req);
        auto resp = reader.read_message();
        REQUIRE(resp.has_value());
        REQUIRE(resp.value().value("id", 0) == id);
        return resp.value()["result"];
    };

    SECTION("Case 11.11: Method Hover Shows Complete Signature") {
        // Hover on 'doWork' in BaseJob at line 5, char 24
        // Expected sig: "public virtual void doWork(int32 n, string label)"
        auto result = send_hover(1101, 5, 24);
        REQUIRE_FALSE(result.is_null());
        REQUIRE(result.contains("contents"));
        std::string md = result["contents"]["value"];
        REQUIRE(md.find("```solix")    != std::string::npos);
        REQUIRE(md.find("public")      != std::string::npos);
        REQUIRE(md.find("virtual")     != std::string::npos);
        REQUIRE(md.find("void")        != std::string::npos);
        REQUIRE(md.find("doWork")      != std::string::npos);
        REQUIRE(md.find("int32 n")     != std::string::npos);
        REQUIRE(md.find("string")      != std::string::npos);
        REQUIRE(md.find("label")       != std::string::npos);
    }

    SECTION("Case 11.12: Field Hover Shows Access Modifiers and Type") {
        // Hover on 'limit' field at line 8, char 26
        // Expected sig: "private const int32 limit"
        auto result = send_hover(1102, 8, 26);
        REQUIRE_FALSE(result.is_null());
        REQUIRE(result.contains("contents"));
        std::string md = result["contents"]["value"];
        REQUIRE(md.find("```solix") != std::string::npos);
        REQUIRE(md.find("private")  != std::string::npos);
        REQUIRE(md.find("const")    != std::string::npos);
        REQUIRE(md.find("int32")    != std::string::npos);
        REQUIRE(md.find("limit")    != std::string::npos);
    }

    SECTION("Case 11.13: Variable Hover Shows Full Declaration Signature") {
        // Hover on 'localMax' at line 11, char 20
        // Expected: "const int32 localMax"  (no access modifier)
        auto result = send_hover(1103, 11, 20);
        REQUIRE_FALSE(result.is_null());
        REQUIRE(result.contains("contents"));
        std::string md = result["contents"]["value"];
        REQUIRE(md.find("```solix") != std::string::npos);
        REQUIRE(md.find("const")    != std::string::npos);
        REQUIRE(md.find("int32")    != std::string::npos);
        REQUIRE(md.find("localMax") != std::string::npos);
        // Local variables have no access modifier
        REQUIRE(md.find("public")   == std::string::npos);
        REQUIRE(md.find("private")  == std::string::npos);
    }

    SECTION("Case 11.14: Class Hover Shows Full Hierarchy Signature") {
        // Hover on 'ConcreteJob' class at line 7, char 14
        // Expected: "public class ConcreteJob extends BaseJob implements IWorker"
        auto result = send_hover(1104, 7, 14);
        REQUIRE_FALSE(result.is_null());
        REQUIRE(result.contains("contents"));
        std::string md = result["contents"]["value"];
        REQUIRE(md.find("```solix")    != std::string::npos);
        REQUIRE(md.find("public")      != std::string::npos);
        REQUIRE(md.find("class")       != std::string::npos);
        REQUIRE(md.find("ConcreteJob") != std::string::npos);
        REQUIRE(md.find("extends")     != std::string::npos);
        REQUIRE(md.find("BaseJob")     != std::string::npos);
        REQUIRE(md.find("implements")  != std::string::npos);
        REQUIRE(md.find("IWorker")     != std::string::npos);
    }

    SECTION("Case 11.15: New-Instance Hover Shows Constructor Signature") {
        // Hover on 'ConcreteJob' inside 'new ConcreteJob(...)' at line 17, char 32
        // Expected: "public ConcreteJob(int32 cap, string name)"
        auto result = send_hover(1105, 17, 32);
        REQUIRE_FALSE(result.is_null());
        REQUIRE(result.contains("contents"));
        std::string md = result["contents"]["value"];
        REQUIRE(md.find("```solix")    != std::string::npos);
        REQUIRE(md.find("ConcreteJob") != std::string::npos);
        REQUIRE(md.find("int32")       != std::string::npos);
        REQUIRE(md.find("cap")         != std::string::npos);
        REQUIRE(md.find("string")      != std::string::npos);
        REQUIRE(md.find("name")        != std::string::npos);
    }

    SECTION("Case 11.16: Hover on Keyword Returns Null") {
        // "public" at line 0, char 2
        REQUIRE(send_hover(1106, 0, 2).is_null());
        // "class" in "public class ConcreteJob" at line 7, char 7
        REQUIRE(send_hover(1107, 7, 7).is_null());
        // "extends" at line 7, char 26
        REQUIRE(send_hover(1108, 7, 26).is_null());
        // "implements" at line 7, char 44
        REQUIRE(send_hover(1109, 7, 44).is_null());
        // "override" at line 10, char 11
        REQUIRE(send_hover(1110, 10, 11).is_null());
        // "const" at line 11, char 8
        REQUIRE(send_hover(1111, 11, 8).is_null());
    }
}
