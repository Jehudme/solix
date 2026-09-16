#include "solix/new/processes/diagnostic.hpp"
#include "solix/new/compilation.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <vector>
#include <iostream>

namespace solix {

Diagnostic::Diagnostic(CompilationContext &context) {
    const auto& options = context.options;
    std::vector<spdlog::sink_ptr> sinks;

    if (options.sink_type == CompilationOptions::LogSinkType::STDOUT || options.sink_type == CompilationOptions::LogSinkType::CONSOLE_AND_FILE) {
        if (options.use_multithreading) {
            sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        } else {
            sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_st>());
        }
    }
    
    if (options.sink_type == CompilationOptions::LogSinkType::BASIC_FILE || options.sink_type == CompilationOptions::LogSinkType::CONSOLE_AND_FILE) {
        if (options.log_file_path.has_value()) {
            if (options.use_multithreading) {
                sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(options.log_file_path.value().string(), true));
            } else {
                sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_st>(options.log_file_path.value().string(), true));
            }
        }
    }

    root_logger = std::make_shared<spdlog::logger>("Compiler", sinks.begin(), sinks.end());
    
    root_logger->set_pattern(options.log_pattern);
    
    spdlog::level::level_enum level = spdlog::level::info;
    if (options.log_level == CompilationOptions::LogLevel::TRACE) level = spdlog::level::trace;
    else if (options.log_level == CompilationOptions::LogLevel::DEBUG) level = spdlog::level::debug;
    else if (options.log_level == CompilationOptions::LogLevel::WARN) level = spdlog::level::warn;
    else if (options.log_level == CompilationOptions::LogLevel::ERR) level = spdlog::level::err;
    else if (options.log_level == CompilationOptions::LogLevel::CRITICAL) level = spdlog::level::critical;
    else if (options.log_level == CompilationOptions::LogLevel::OFF) level = spdlog::level::off;
    
    root_logger->set_level(level);
    
    spdlog::level::level_enum flush = spdlog::level::err;
    if (options.flush_level == CompilationOptions::LogLevel::TRACE) flush = spdlog::level::trace;
    else if (options.flush_level == CompilationOptions::LogLevel::DEBUG) flush = spdlog::level::debug;
    else if (options.flush_level == CompilationOptions::LogLevel::WARN) flush = spdlog::level::warn;
    else if (options.flush_level == CompilationOptions::LogLevel::CRITICAL) flush = spdlog::level::critical;
    else if (options.flush_level == CompilationOptions::LogLevel::OFF) flush = spdlog::level::off;
    
    root_logger->flush_on(flush);
    
    if (options.flush_every_seconds.count() > 0) {
        spdlog::flush_every(options.flush_every_seconds);
    }
}

Diagnostic::~Diagnostic() {
    spdlog::drop_all();
}

std::shared_ptr<spdlog::logger> Diagnostic::get_root_logger() const {
    return root_logger;
}

std::shared_ptr<spdlog::logger> Diagnostic::create_process_logger(const std::string& process_name) {
    if (!root_logger) return nullptr;
    auto new_logger = root_logger->clone(process_name);
    return new_logger;
}

} // namespace solix
