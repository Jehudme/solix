#include "execute.hpp"
#include "solix/compilation.hpp"
#include "solix/runtime.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace solix::cli {

void setup_execute_command(CLI::App &app) {
  // TODO: It must only run the .slxb file
  auto *execute_cmd = app.add_subcommand("run", "run compiled bytecode");

  auto opts = std::make_shared<RuntimeOptions>();
  auto input_file = std::make_shared<std::string>();
  auto no_builtins = std::make_shared<bool>(false);

  execute_cmd->add_option("file", *input_file, "Solix source compiled bytecode")
      ->required()
      ->check(CLI::ExistingFile);

  execute_cmd->add_option("-s,--stack", opts->stack_capacity,
                          "Stack capacity in words (default: 1048576)");

  execute_cmd->add_option("-p,--heap", opts->heap_capacity,
                          "Heap capacity in words (default: 16777216)");

  execute_cmd->add_flag("--no-builtins", *no_builtins,
                        "Do not automatically register builtin native functions");

  execute_cmd->add_option("args", opts->program_args,
                          "Arguments passed to the Solix program");

  execute_cmd->callback([opts, input_file, no_builtins]() {
    std::filesystem::path path(*input_file);

    if (!std::filesystem::exists(path)) {
      std::cerr << "Error: File does not exist: " << path << std::endl;
      exit(1);
    }

    opts->bytecode_source = path;
    
    // Register builtin native functions unless explicitly disabled
    if (!*no_builtins) {
      for (const auto &[name, func] : solix::get_builtin_natives()) {
        if (!opts->native_functions.count(name)) {
          opts->native_functions[name] = func;
        }
      }
    }

    try {
      run(*opts);
    } catch (const std::exception &e) {
      std::cerr << "Runtime error: " << e.what() << std::endl;
      exit(1);
    }
  });
}
} // namespace solix::cli
