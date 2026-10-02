#include "compile.hpp"
#include "../cli_utils.hpp"
#include "solix/path_utils.hpp"
#include "solix/compilation.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace solix::cli {

    CompilationOptions::LogLevel map_log_level(const std::string& s) {
        std::string upper = s;
        for (auto& c : upper) c = std::toupper(static_cast<unsigned char>(c));
        if (upper == "TRACE") return CompilationOptions::LogLevel::TRACE;
        if (upper == "DEBUG") return CompilationOptions::LogLevel::DEBUG;
        if (upper == "INFO") return CompilationOptions::LogLevel::INFO;
        if (upper == "WARN" || upper == "WARNING") return CompilationOptions::LogLevel::WARN;
        if (upper == "ERR" || upper == "ERROR") return CompilationOptions::LogLevel::ERR;
        if (upper == "CRITICAL") return CompilationOptions::LogLevel::CRITICAL;
        return CompilationOptions::LogLevel::OFF;
    }

    CompilationOptions::LogSinkType map_log_sink_type(const std::string& s) {
        std::string upper = s;
        for (auto& c : upper) c = std::toupper(static_cast<unsigned char>(c));
        if (upper == "STDOUT") return CompilationOptions::LogSinkType::STDOUT;
        if (upper == "STDERR") return CompilationOptions::LogSinkType::STDERR;
        if (upper == "BASIC_FILE") return CompilationOptions::LogSinkType::BASIC_FILE;
        if (upper == "CONSOLE_AND_FILE") return CompilationOptions::LogSinkType::CONSOLE_AND_FILE;
        return CompilationOptions::LogSinkType::STDOUT;
    }

    int execute_compilation_and_write(CompilationOptions& opts, const std::filesystem::path& out_path) {
        try {
            std::vector<uint8_t> bytecode = solix::run(opts);
            
            std::filesystem::path parent = out_path.parent_path();
            if (!parent.empty() && !std::filesystem::exists(parent)) {
                std::filesystem::create_directories(parent);
            }

            std::ofstream out_file(out_path, std::ios::binary);
            if (!out_file) {
                std::cerr << "Error: Could not open output file " << out_path.string() << std::endl;
                return 1;
            }
            out_file.write(reinterpret_cast<const char*>(bytecode.data()), bytecode.size());
            std::cout << "Successfully compiled to " << out_path.string() << std::endl;
            return 0;
            
        } catch (const std::exception& e) {
            std::error_code ec;
            std::filesystem::remove(out_path, ec);
            std::cerr << "Compilation failed: " << e.what() << std::endl;
            return 1;
        }
    }

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
            opts->log_level = map_log_level(*log_level_str);
            opts->flush_level = map_log_level(*flush_level_str);
            opts->sink_type = map_log_sink_type(*sink_type_str);
            
            if (!log_file_path->empty()) opts->log_file_path = std::filesystem::path(*log_file_path).lexically_normal();
            if (!asm_output->empty()) opts->assembly_output_path = std::filesystem::path(*asm_output).lexically_normal();
            opts->flush_every_seconds = std::chrono::seconds(*flush_every);

            std::filesystem::path out_path = std::filesystem::path(*output).lexically_normal();

            for (const auto& file_str : *files) {
                std::filesystem::path file_path = std::filesystem::path(file_str).lexically_normal();
                if (!std::filesystem::exists(file_path)) {
                    std::cerr << "Error: File does not exist: " << file_path << std::endl;
                    cli_exit(1);
                }
                std::ifstream t(file_path);
                if (!t.is_open()) {
                    std::cerr << "Error: Could not open file " << file_path << std::endl;
                    cli_exit(1);
                }
                std::stringstream buffer;
                buffer << t.rdbuf();
                opts->sources[file_path.string()] = buffer.str();
            }
            
            int exit_code = execute_compilation_and_write(*opts, out_path);
            if (exit_code != 0) {
                cli_exit(exit_code);
            }
        });
    }
}
