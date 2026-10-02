#pragma once
#include <CLI/CLI.hpp>

namespace solix::cli {

/**
 * @brief Cleanly signals CLI exit with code by throwing a CLI::RuntimeError,
 * allowing CLI11 to handle exit codes gracefully without killing test runners.
 */
[[noreturn]] inline void cli_exit(int code = 1) {
    throw CLI::RuntimeError(code);
}

} // namespace solix::cli
