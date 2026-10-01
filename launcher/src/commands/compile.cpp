#include "compile.hpp"
#include "solix/path_utils.hpp"
#include "solix/compilation.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace solix::cli {
    void setup_compile_command(CLI::App& app) {
        auto* compile_cmd = app.add_subcommand("compile", "Compile Solix source code to bytecode");
        
        auto opts = std::make_shared<CompilationOptions>();
        auto files = std::make_shared<std::vector<std::string>>();
        auto output = std::make_shared<std::string>("out.slxb");
        
        auto log_level_str = std::make_shared<std::string>("INFO");
        auto flush_level_str = std::make_shared<std::string>("ERR");
        auto sink_type_str = std::make_shared<std::string>("STDOUT");
        auto log_file_path = std::make_shared<std::string>();
        auto flush_every = std::make_shared<int>(0);

        auto asm_output = std::make_shared<std::string>();

        compile_cmd->add_option("files", *files, "Source files to compile")->required()->check(CLI::ExistingFile);
        compile_cmd->add_option("-e,--entry", opts->entry_point, "Entry point method name (default: main)");
        compile_cmd->add_option("-o,--output", *output, "Output bytecode file (default: out.slxb)");
        compile_cmd->add_option("-a,--asm", *asm_output, "Output assembly code to file");
        
        // Advanced compilation options
        compile_cmd->add_option("--log-level", *log_level_str, "Log level: TRACE, DEBUG, INFO, WARN, ERR, CRITICAL, OFF")->check(CLI::IsMember({"TRACE", "DEBUG", "INFO", "WARN", "ERR", "CRITICAL", "OFF"}));
        compile_cmd->add_option("--flush-level", *flush_level_str, "Flush level: TRACE, DEBUG, INFO, WARN, ERR, CRITICAL, OFF")->check(CLI::IsMember({"TRACE", "DEBUG", "INFO", "WARN", "ERR", "CRITICAL", "OFF"}));
        compile_cmd->add_option("--sink-type", *sink_type_str, "Log sink type: STDOUT, STDERR, BASIC_FILE, CONSOLE_AND_FILE")->check(CLI::IsMember({"STDOUT", "STDERR", "BASIC_FILE", "CONSOLE_AND_FILE"}));
        compile_cmd->add_option("--log-pattern", opts->log_pattern, "Custom log pattern");
        compile_cmd->add_option("--log-file", *log_file_path, "Path to log file (used with BASIC_FILE or CONSOLE_AND_FILE)");
        compile_cmd->add_flag("--multithreaded", opts->use_multithreading, "Enable multithreading");
        compile_cmd->add_option("--flush-every", *flush_every, "Flush logs every N seconds");

        compile_cmd->callback([opts, files, output, asm_output, log_level_str, flush_level_str, sink_type_str, log_file_path, flush_every]() {
            // Map log levels
            auto map_level = [](const std::string& s) {
                if (s == "TRACE") return CompilationOptions::LogLevel::TRACE;
                if (s == "DEBUG") return CompilationOptions::LogLevel::DEBUG;
                if (s == "INFO") return CompilationOptions::LogLevel::INFO;
                if (s == "WARN") return CompilationOptions::LogLevel::WARN;
                if (s == "ERR") return CompilationOptions::LogLevel::ERR;
                if (s == "CRITICAL") return CompilationOptions::LogLevel::CRITICAL;
                return CompilationOptions::LogLevel::OFF;
            };
            
            opts->log_level = map_level(*log_level_str);
            opts->flush_level = map_level(*flush_level_str);
            
            if (*sink_type_str == "STDOUT") opts->sink_type = CompilationOptions::LogSinkType::STDOUT;
            else if (*sink_type_str == "STDERR") opts->sink_type = CompilationOptions::LogSinkType::STDERR;
            else if (*sink_type_str == "BASIC_FILE") opts->sink_type = CompilationOptions::LogSinkType::BASIC_FILE;
            else if (*sink_type_str == "CONSOLE_AND_FILE") opts->sink_type = CompilationOptions::LogSinkType::CONSOLE_AND_FILE;
            
            if (!log_file_path->empty()) opts->log_file_path = std::filesystem::path(*log_file_path).lexically_normal();
            if (!asm_output->empty()) opts->assembly_output_path = std::filesystem::path(*asm_output).lexically_normal();
            opts->flush_every_seconds = std::chrono::seconds(*flush_every);

            std::filesystem::path out_path = std::filesystem::path(*output).lexically_normal();

            for (const auto& file_str : *files) {
                std::filesystem::path file_path = std::filesystem::path(file_str).lexically_normal();
                if (!std::filesystem::exists(file_path)) {
                    std::cerr << "Error: File does not exist: " << file_path << std::endl;
                    exit(1);
                }
                std::ifstream t(file_path);
                if (!t.is_open()) {
                    std::cerr << "Error: Could not open file " << file_path << std::endl;
                    exit(1);
                }
                std::stringstream buffer;
                buffer << t.rdbuf();
                opts->sources[file_path.string()] = buffer.str();
            }
            
            try {
                std::vector<uint8_t> bytecode = solix::run(*opts);
                
                std::ofstream out_file(out_path, std::ios::binary);
                if (!out_file) {
                    std::cerr << "Error: Could not open output file " << out_path << std::endl;
                    exit(1);
                }
                out_file.write(reinterpret_cast<const char*>(bytecode.data()), bytecode.size());
                std::cout << "Successfully compiled to " << out_path.string() << std::endl;
                
            } catch (const std::exception& e) {
                std::error_code ec;
                std::filesystem::remove(out_path, ec);
                std::cerr << "Compilation failed: " << e.what() << std::endl;
                exit(1);
            }
        });
    }
}
