#include "solix/new/processes/diagnostic.hpp"
#include "solix/new/compilation.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <iostream>

namespace solix {

Diagnostic::Diagnostic(CompilationContext &context) {
    const auto& options = context.options;
    std::vector<spdlog::sink_ptr> sinks;

    if (options.log_sink == CompilationOptions::LogSinkType::STDOUT || options.log_sink == CompilationOptions::LogSinkType::CONSOLE_AND_FILE) {
        if (options.use_multithreading) {
            sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        } else {
            sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_st>());
        }
    } else if (options.log_sink == CompilationOptions::LogSinkType::STDERR) {
        if (options.use_multithreading) {
            sinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
        } else {
            sinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_st>());
        }
    }
    
    if (options.log_sink == CompilationOptions::LogSinkType::BASIC_FILE || options.log_sink == CompilationOptions::LogSinkType::CONSOLE_AND_FILE) {
        if (options.log_file_path.has_value()) {
            if (options.use_multithreading) {
                sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(options.log_file_path.value().string(), true));
            } else {
                sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_st>(options.log_file_path.value().string(), true));
            }
        }
    }

    std::string root_name = "Compiler";
    
    // Ensure the logger name is unique in case of multiple context instances alive concurrently
    int counter = 0;
    std::string unique_root_name = root_name;
    while (spdlog::get(unique_root_name)) {
        counter++;
        unique_root_name = root_name + "_" + std::to_string(counter);
    }
    
    root_logger = std::make_shared<spdlog::logger>(unique_root_name, sinks.begin(), sinks.end());
    
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
    
    spdlog::register_logger(root_logger);
    registered_loggers.push_back(unique_root_name);
    
    if (options.flush_every_seconds.count() > 0) {
        spdlog::flush_every(options.flush_every_seconds);
    }
}

Diagnostic::~Diagnostic() {
    for (const auto& name : registered_loggers) {
        spdlog::drop(name);
    }
}

std::shared_ptr<spdlog::logger> Diagnostic::get_root_logger() const {
    return root_logger;
}

std::shared_ptr<spdlog::logger> Diagnostic::create_process_logger(const std::string& process_name) {
    if (!root_logger) return nullptr;
    
    std::string unique_name = process_name;
    int counter = 0;
    while (spdlog::get(unique_name)) {
        counter++;
        unique_name = process_name + "_" + std::to_string(counter);
    }
    
    auto new_logger = root_logger->clone(unique_name);
    spdlog::register_logger(new_logger);
    registered_loggers.push_back(unique_name);
    
    return new_logger;
}

} // namespace solix
