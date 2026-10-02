#include "build.hpp"
#include "solix/compilation.hpp"
#include "solix/path_utils.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace solix::cli {

void setup_build_command(CLI::App &app) {
  auto *build_cmd = app.add_subcommand("build", "Build the Solix project");

  auto compilationContext = std::make_shared<CompilationOptions>();

  build_cmd->callback([]() {

  });
}
} // namespace solix::cli
