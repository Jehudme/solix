#include "compile.hpp"
#include "solix/compilation.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

namespace solix::cli {
    void setup_compile_command(CLI::App& app) {
        auto* compile_cmd = app.add_subcommand("compile", "Compile Solix source code to bytecode");
        
        auto opts = std::make_shared<CompilationOptions>();
        auto files = std::make_shared<std::vector<std::string>>();
        auto output = std::make_shared<std::string>("out.slxb");
        
        compile_cmd->add_option("files", *files, "Source files to compile")->required()->check(CLI::ExistingFile);
        compile_cmd->add_option("-e,--entry", opts->entry_point, "Entry point method name (default: main)");
        compile_cmd->add_option("-o,--output", *output, "Output bytecode file (default: out.slxb)");
        
        compile_cmd->callback([opts, files, output]() {
            for (const auto& file_path : *files) {
                std::ifstream t(file_path);
                if (!t.is_open()) {
                    std::cerr << "Error: Could not open file " << file_path << std::endl;
                    exit(1);
                }
                std::stringstream buffer;
                buffer << t.rdbuf();
                opts->sources[file_path] = buffer.str();
            }
            
            try {
                opts->log_level = CompilationOptions::LogLevel::OFF;
                std::vector<uint8_t> bytecode = solix::run(*opts);
                
                std::ofstream out_file(*output, std::ios::binary);
                if (!out_file) {
                    std::cerr << "Error: Could not open output file " << *output << std::endl;
                    exit(1);
                }
                out_file.write(reinterpret_cast<const char*>(bytecode.data()), bytecode.size());
                std::cout << "Successfully compiled to " << *output << std::endl;
                
            } catch (const std::exception& e) {
                std::cerr << "Compilation failed: " << e.what() << std::endl;
                exit(1);
            }
        });
    }
}
