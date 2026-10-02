#pragma once
#include <CLI/CLI.hpp>
#include <string>
#include <filesystem>
#include "solix/compilation.hpp"

namespace solix::cli {
    CompilationOptions::LogLevel map_log_level(const std::string& s);
    CompilationOptions::LogSinkType map_log_sink_type(const std::string& s);
    int execute_compilation_and_write(CompilationOptions& opts, const std::filesystem::path& out_path);
    void setup_compile_command(CLI::App& app);
}
