#include "solix/lsp/server.hpp"
#include <iostream>

int main() {
    // Solix Language Server standalone entry point
    solix::lsp::LspServer server(std::cin, std::cout);
    return server.run();
}
