#pragma once
#include <CLI/CLI.hpp>
#include <filesystem>
#include <string>
#include <nlohmann/json.hpp>

namespace solix::cli {

struct ProjectBuildResult {
    bool success{false};
    std::filesystem::path output_binary;
    std::filesystem::path project_root;
    nlohmann::json manifest;
    std::string profile_name;
};

ProjectBuildResult build_project(const std::filesystem::path& manifest_or_dir,
                                 const std::string& profile_name);

void setup_build_command(CLI::App &app);

} // namespace solix::cli
