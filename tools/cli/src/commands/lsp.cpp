#include "lsp.hpp"
#include "solix/lsp/server.hpp"
#include <iostream>

namespace solix::cli {

void setup_lsp_command(CLI::App &app) {
    auto *lsp_cmd = app.add_subcommand("lsp", "Start the Solix Language Server (LSP) over stdio");

    static bool use_stdio = true;
    lsp_cmd->add_flag("--stdio", use_stdio, "Use standard input/output for JSON-RPC communication (default)");
    lsp_cmd->allow_extras(true);

    lsp_cmd->callback([]() {
        solix::lsp::LspServer server(std::cin, std::cout);
        int code = server.run();
        if (code != 0) {
            exit(code);
        }
    });
}

} // namespace solix::cli
