#pragma once

#include <memory>
#include <string>
#include <vector>

namespace spdlog {
    class logger;
}

namespace solix {

struct CompilationContext;

class Diagnostic {
public:
  Diagnostic(CompilationContext &context);
  virtual ~Diagnostic();

  std::shared_ptr<spdlog::logger> get_root_logger() const;
  std::shared_ptr<spdlog::logger> create_process_logger(const std::string& process_name);

private:
  std::shared_ptr<spdlog::logger> root_logger;
  std::vector<std::string> registered_loggers;
};

} // namespace solix
