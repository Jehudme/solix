#include <CLI/CLI.hpp>
#include "commands/compile.hpp"
#include "commands/execute.hpp"
#include "commands/build.hpp"
#include "commands/new.hpp"
#include "commands/package.hpp"
#include "commands/version.hpp"
#include "commands/lsp.hpp"
#include "commands/ide.hpp"
#include <iostream>

int main(int argc, char** argv) {
    CLI::App app{"Solix Programming Language Compiler and VM"};
    
    app.require_subcommand(1);
    
    solix::cli::setup_compile_command(app);
    solix::cli::setup_execute_command(app);
    solix::cli::setup_build_command(app);
    solix::cli::setup_new_command(app);
    solix::cli::setup_package_commands(app);
    solix::cli::setup_version_command(app);
    solix::cli::setup_lsp_command(app);
    solix::cli::setup_ide_command(app);
    
    CLI11_PARSE(app, argc, argv);
    
    return 0;
}
