#include "execute.hpp"
#include "solix/compilation.hpp"
#include "solix/runtime.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

namespace solix::cli {

    void setup_execute_command(CLI::App& app) {
        auto* execute_cmd = app.add_subcommand("run", "Compile and execute a Solix source file (.slx) or run compiled bytecode (.slxb)");

        auto opts         = std::make_shared<RuntimeOptions>();
        auto input_file   = std::make_shared<std::string>();

        execute_cmd->add_option("file", *input_file, "Solix source (.slx) or compiled bytecode (.slxb)")->required()->check(CLI::ExistingFile);
        execute_cmd->add_option("-s,--stack", opts->stack_capacity, "Stack capacity in words (default: 1048576)");
        execute_cmd->add_option("-p,--heap",  opts->heap_capacity,  "Heap capacity in words (default: 16777216)");
        execute_cmd->add_option("args",       opts->program_args,   "Arguments passed to the Solix program");

        execute_cmd->callback([opts, input_file]() {
            std::filesystem::path path(*input_file);
            std::string ext = path.extension().string();

            if (ext == ".slx") {
                // ── Compile source → bytecode in memory, then execute ──────────
                std::ifstream t(path);
                if (!t.is_open()) {
                    std::cerr << "Error: Could not open " << path << std::endl;
                    exit(1);
                }
                std::ostringstream buf;
                buf << t.rdbuf();
                std::string source = buf.str();

                // Extract package prefix for native function names and register standard natives
                std::string pkg = solix::extract_package_prefix(source);
                solix::register_standard_natives(*opts, pkg);

                CompilationOptions copts;
                copts.log_level = CompilationOptions::LogLevel::OFF;
                copts.sources[path] = source;

                try {
                    Bytecode bytecode = solix::run(copts);
                    opts->bytecode_source = bytecode;
                    solix::run(*opts);
                } catch (const std::exception& e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                    exit(1);
                }

            } else {
                // ── Load pre-compiled .slxb bytecode ──────────────────────────
                // Register natives with empty prefix (package unknown)
                solix::register_standard_natives(*opts, "");
                // Also register the well-known test package prefix
                solix::register_standard_natives(*opts, "com.solix.advanced.test.");

                opts->bytecode_source = path;

                try {
                    solix::run(*opts);
                } catch (const std::exception& e) {
                    std::cerr << "Execution failed: " << e.what() << std::endl;
                    exit(1);
                }
            }
        });
    }
}
