#include "execute.hpp"
#include "../cli_utils.hpp"
#include "solix/compilation.hpp"
#include "solix/runtime.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace solix::cli {

void setup_execute_command(CLI::App &app) {
  auto *execute_cmd = app.add_subcommand("run", "run compiled bytecode");

  auto opts = std::make_shared<RuntimeOptions>();
  auto input_file = std::make_shared<std::string>();

  execute_cmd->add_option("file", *input_file, "Solix source compiled bytecode")
      ->required()
      ->check(CLI::ExistingFile);

  execute_cmd->add_option("-s,--stack", opts->stack_capacity,
                          "Stack capacity in words (default: 1048576)");

  execute_cmd->add_option("-p,--heap", opts->heap_capacity,
                          "Heap capacity in words (default: 16777216)");

  execute_cmd->add_option("args", opts->program_args,
                          "Arguments passed to the Solix program");

  execute_cmd->callback([opts, input_file]() {
    std::filesystem::path path(*input_file);

    if (!std::filesystem::exists(path)) {
      std::cerr << "Error: File does not exist: " << path << std::endl;
      cli_exit(1);
    }

    if (std::filesystem::file_size(path) == 0) {
      std::cerr << "Error: Bytecode file is empty: " << path << std::endl;
      cli_exit(1);
    }

    opts->bytecode_source = path;

    int32_t exit_code = 0;
    try {
      exit_code = run(*opts);
    } catch (const std::exception &e) {
      std::cerr << "Runtime error: " << e.what() << std::endl;
      cli_exit(1);
    }

    if (exit_code != 0) {
      cli_exit(exit_code);
    }
  });
}
} // namespace solix::cli
