#include "version.hpp"
#include <iostream>

#ifndef SOLIX_VERSION
#define SOLIX_VERSION "0.1.0"
#endif

namespace solix::cli {

void setup_version_command(CLI::App& app) {
    auto print_version = []() {
        std::cout << "Solix version " << SOLIX_VERSION << std::endl;
    };

    // Subcommand: solix version
    auto* ver_cmd = app.add_subcommand("version", "Display Solix version");
    ver_cmd->callback([print_version]() {
        print_version();
    });

    // Top-level flag: -v, --version
    app.set_version_flag("-v,--version", std::string("Solix version ") + SOLIX_VERSION);
}

} // namespace solix::cli
