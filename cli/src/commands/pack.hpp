#pragma once

#include <CLI/CLI.hpp>
#include <filesystem>
#include <string>

namespace solix::cli {

struct PackResult {
    bool success{false};
    std::filesystem::path output_package;
    std::string error_message;
    size_t file_count{0};
    uintmax_t package_size_bytes{0};
};

/**
 * @brief Packages a Solix project into a single .slxpac archive.
 * @param manifest_or_dir Path to manifest.json, solix.json, or project directory.
 * @param profile_name Build profile name (default: "release").
 * @param explicit_output_path Optional user-specified output path for the .slxpac file.
 */
PackResult pack_project(const std::filesystem::path& manifest_or_dir,
                        const std::string& profile_name = "release",
                        const std::filesystem::path& explicit_output_path = "");

void setup_pack_command(CLI::App &app);

} // namespace solix::cli
