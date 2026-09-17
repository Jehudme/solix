#include <CLI/CLI.hpp>
#include "commands/compile.hpp"
#include "commands/execute.hpp"
#include <iostream>

int main(int argc, char** argv) {
    CLI::App app{"Solix Programming Language Compiler and VM"};
    
    app.require_subcommand(1);
    
    solix::cli::setup_compile_command(app);
    solix::cli::setup_execute_command(app);
    
    CLI11_PARSE(app, argc, argv);
    
    return 0;
}
