#pragma once

#include <memory>
#include <string>
#include <vector>

namespace spdlog {
    class logger;
}


namespace solix {

enum class ReportSeverity { NOTE, WARNING, ERROR };

struct Report {
    ReportSeverity severity;
    std::string code;
    std::string message;
    std::string source_path;
    int line;
    int column;
};

struct CompilationContext;

class Diagnostic {
public:
  Diagnostic(CompilationContext &context);
  virtual ~Diagnostic();

  std::shared_ptr<spdlog::logger> get_root_logger() const;
  std::shared_ptr<spdlog::logger> create_process_logger(const std::string& process_name);

  void record_report(const Report& report);
  void record_warning(const std::string &message, const std::string &source_path = "", int line = 0, int column = 0, const std::string &code = "W_WARN");
  bool has_errors() const;
  bool has_warnings() const;
  size_t error_count() const;
  size_t warning_count() const;
  const std::vector<Report>& get_reports() const;
  void print_reports(bool disable_color = false) const;

private:
  CompilationContext &context;
  std::shared_ptr<spdlog::logger> root_logger;
  std::vector<std::string> registered_loggers;
  std::vector<Report> reports;
};

} // namespace solix
