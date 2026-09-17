#include "solix/utilities/diagnostic.hpp"
#include "solix/compilation.hpp"
#include <iostream>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace solix {

Diagnostic::Diagnostic(CompilationContext &context) {
  const auto &options = context.options;
  std::vector<spdlog::sink_ptr> sinks;

  if (options.sink_type == CompilationOptions::LogSinkType::STDOUT ||
      options.sink_type == CompilationOptions::LogSinkType::CONSOLE_AND_FILE) {
    if (options.use_multithreading) {
      sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    } else {
      sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_st>());
    }
  } else if (options.sink_type == CompilationOptions::LogSinkType::STDERR) {
    if (options.use_multithreading) {
      sinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
    } else {
      sinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_st>());
    }
  }

  if (options.sink_type == CompilationOptions::LogSinkType::BASIC_FILE ||
      options.sink_type == CompilationOptions::LogSinkType::CONSOLE_AND_FILE) {
    if (options.log_file_path.has_value()) {
      if (options.use_multithreading) {
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            options.log_file_path.value().string(), true));
      } else {
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_st>(
            options.log_file_path.value().string(), true));
      }
    }
  }

  std::string root_name = "Compiler";

  // Ensure the logger name is unique in case of multiple context instances
  // alive concurrently
  int counter = 0;
  std::string unique_root_name = root_name;
  while (spdlog::get(unique_root_name)) {
    counter++;
    unique_root_name = root_name + "_" + std::to_string(counter);
  }

  root_logger = std::make_shared<spdlog::logger>(unique_root_name,
                                                 sinks.begin(), sinks.end());

  root_logger->set_pattern(options.log_pattern);

  spdlog::level::level_enum level = spdlog::level::info;
  if (options.log_level == CompilationOptions::LogLevel::TRACE)
    level = spdlog::level::trace;
  else if (options.log_level == CompilationOptions::LogLevel::DEBUG)
    level = spdlog::level::debug;
  else if (options.log_level == CompilationOptions::LogLevel::WARN)
    level = spdlog::level::warn;
  else if (options.log_level == CompilationOptions::LogLevel::ERR)
    level = spdlog::level::err;
  else if (options.log_level == CompilationOptions::LogLevel::CRITICAL)
    level = spdlog::level::critical;
  else if (options.log_level == CompilationOptions::LogLevel::OFF)
    level = spdlog::level::off;

  root_logger->set_level(level);

  spdlog::level::level_enum flush = spdlog::level::err;
  if (options.flush_level == CompilationOptions::LogLevel::TRACE)
    flush = spdlog::level::trace;
  else if (options.flush_level == CompilationOptions::LogLevel::DEBUG)
    flush = spdlog::level::debug;
  else if (options.flush_level == CompilationOptions::LogLevel::WARN)
    flush = spdlog::level::warn;
  else if (options.flush_level == CompilationOptions::LogLevel::CRITICAL)
    flush = spdlog::level::critical;
  else if (options.flush_level == CompilationOptions::LogLevel::OFF)
    flush = spdlog::level::off;

  root_logger->flush_on(flush);

  spdlog::register_logger(root_logger);
  registered_loggers.push_back(unique_root_name);

  if (options.flush_every_seconds.count() > 0) {
    spdlog::flush_every(options.flush_every_seconds);
  }
}

Diagnostic::~Diagnostic() {
  for (const auto &name : registered_loggers) {
    spdlog::drop(name);
  }
}

std::shared_ptr<spdlog::logger> Diagnostic::get_root_logger() const {
  return root_logger;
}

std::shared_ptr<spdlog::logger>
Diagnostic::create_process_logger(const std::string &process_name) {
  if (!root_logger)
    return nullptr;

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


void Diagnostic::record_report(const Report& report) {
    reports.push_back(report);
}

bool Diagnostic::has_errors() const {
    for (const auto& r : reports) {
        if (r.severity == ReportSeverity::ERROR) return true;
    }
    return false;
}

const std::vector<Report>& Diagnostic::get_reports() const {
    return reports;
}

void Diagnostic::print_reports(bool disable_color) const {
    const char* RESET = disable_color ? "" : "\033[0m";
    const char* RED = disable_color ? "" : "\033[31m";
    const char* YELLOW = disable_color ? "" : "\033[33m";
    const char* CYAN = disable_color ? "" : "\033[36m";
    const char* BOLD = disable_color ? "" : "\033[1m";

    for (const auto& r : reports) {
        std::string severity_str;
        std::string color;
        if (r.severity == ReportSeverity::ERROR) {
            severity_str = "error";
            color = RED;
        } else if (r.severity == ReportSeverity::WARNING) {
            severity_str = "warning";
            color = YELLOW;
        } else {
            severity_str = "note";
            color = CYAN;
        }

        std::cout << BOLD << r.source_path << ":" << r.line << ":" << r.column << ": "
                  << color << severity_str << RESET << BOLD << ": " 
                  << (r.code.empty() ? "" : "[" + r.code + "] ") << r.message << RESET << "\n";
    }
}
} // namespace solix

