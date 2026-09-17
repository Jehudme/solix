#pragma once

#include <memory>
#include <string>
#include <spdlog/spdlog.h>

namespace solix {

struct CompilationContext;

class CompilationProcess {
public:
  CompilationProcess(CompilationContext &context, std::string name);
  virtual ~CompilationProcess();

protected:
  template <typename... Args>
  void log_info(const std::string &format, Args &&...args) {
    if (logger) logger->info(fmt::runtime(format), std::forward<Args>(args)...);
  }

  template <typename... Args>
  void log_debug(const std::string &format, Args &&...args) {
    if (logger) logger->debug(fmt::runtime(format), std::forward<Args>(args)...);
  }

  template <typename... Args>
  void log_trace(const std::string &format, Args &&...args) {
    if (logger) logger->trace(fmt::runtime(format), std::forward<Args>(args)...);
  }

  template <typename... Args>
  void log_warn(const std::string &format, Args &&...args) {
    if (logger) logger->warn(fmt::runtime(format), std::forward<Args>(args)...);
  }

  template <typename... Args>
  void log_error(const std::string &format, Args &&...args) {
    if (logger) logger->error(fmt::runtime(format), std::forward<Args>(args)...);
  }

  CompilationContext &context;

private:
  std::shared_ptr<spdlog::logger> logger;

private:
  virtual void execute() = 0;
};

} // namespace solix
