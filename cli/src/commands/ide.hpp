#pragma once

#include <CLI/CLI.hpp>

namespace solix::cli {

/**
 * @brief Registers the `ide` subcommand and its child actions (`install`).
 */
void setup_ide_command(CLI::App& app);

} // namespace solix::cli
