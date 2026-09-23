#include "execute.hpp"
#include "solix/compilation.hpp"
#include "solix/runtime.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace solix::cli {

// Temporary helper to register required native functions with full symbols
static void register_temp_natives(RuntimeOptions& opts) {
    opts.native_functions["com.solix.advanced.test.Engine.print(char[])"] = [](solix::RuntimeContext& ctx, uint64_t self_address, uint64_t* args, size_t count) -> uint64_t {
        solix::Address addr = static_cast<solix::Address>(args[0]);
        if (addr == 0) {
            std::cout << "null" << std::endl;
            return 0;
        }
        uint32_t len = static_cast<uint32_t>(ctx.memory.heap[addr - 1] >> 32);
        std::string str = "";
        for (uint32_t i = 0; i < len; ++i) {
            str += static_cast<char>(ctx.memory.heap[addr + i]);
        }
        std::cout << str << std::endl;
        return 0; // Automatically pushed by VM
    };

    opts.native_functions["com.solix.advanced.test.Engine.print(int32)"] = [](solix::RuntimeContext& ctx, uint64_t self_address, uint64_t* args, size_t count) -> uint64_t {
        std::cout << static_cast<int32_t>(args[0]) << std::endl;
        return 0; // Automatically pushed by VM
    };
}

void setup_execute_command(CLI::App &app) {
  // TODO: It must only run the .slxb file
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
      exit(1);
    }

    opts->bytecode_source = path;
    
    // Register temporary native functions with full symbol
    register_temp_natives(*opts);

    try {
      run(*opts);
    } catch (const std::exception &e) {
      std::cerr << "Runtime error: " << e.what() << std::endl;
      exit(1);
    }
  });
}
} // namespace solix::cli
