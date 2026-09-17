#include "execute.hpp"
#include "solix/runtime.hpp"
#include <iostream>

namespace solix::cli {
    void setup_execute_command(CLI::App& app) {
        auto* execute_cmd = app.add_subcommand("run", "Execute a compiled Solix bytecode file");
        
        auto opts = std::make_shared<RuntimeOptions>();
        auto bytecode_file = std::make_shared<std::string>();
        
        execute_cmd->add_option("file", *bytecode_file, "Bytecode file to execute")->required()->check(CLI::ExistingFile);
        execute_cmd->add_option("-s,--stack", opts->stack_capacity, "Stack capacity in words (default: 1048576)");
        execute_cmd->add_option("-p,--heap", opts->heap_capacity, "Heap capacity in words (default: 16777216)");
        execute_cmd->add_option("args", opts->program_args, "Arguments passed to the Solix program");
        
        execute_cmd->callback([opts, bytecode_file]() {
            opts->bytecode_source = std::filesystem::path(*bytecode_file);
            
            // For testing purposes, we register a standard print function
            opts->native_functions["com.solix.advanced.test.Engine.print"] = [](solix::RuntimeContext& ctx, uint64_t self_address, uint64_t* args, size_t arg_count) {
                uint64_t val = ctx.pop();
                solix::Address addr = static_cast<solix::Address>(val);
                
                uint32_t len = static_cast<uint32_t>(ctx.memory.heap[addr]);
                std::string str = "";
                for (uint32_t i = 0; i < len; ++i) {
                    str += static_cast<char>(ctx.memory.heap[addr + 1 + i]);
                }
                
                std::cout << str << std::endl;
                ctx.push(0);
            };
            
            try {
                solix::run(*opts);
            } catch (const std::exception& e) {
                std::cerr << "Execution failed: " << e.what() << std::endl;
                exit(1);
            }
        });
    }
}
