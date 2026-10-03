#include "build.hpp"
#include "compile.hpp"
#include "../cli_utils.hpp"
#include "../dependency_resolver.hpp"
#include "../package_manager.hpp"
#include "solix/compilation.hpp"
#include "solix/path_utils.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace solix::cli {

ProjectBuildResult build_project(const std::filesystem::path& manifest_or_dir,
                                 const std::string& profile_name) {
    ProjectBuildResult result;
    result.profile_name = profile_name;

    std::string target_str = manifest_or_dir.string();
    if (target_str.empty()) {
        target_str = "solix.json";
    }

    std::filesystem::path manifest_path;
    auto at_pos = target_str.find('@');
    if (at_pos != std::string::npos && !std::filesystem::exists(target_str)) {
        std::string pkg = target_str.substr(0, at_pos);
        std::string ver = target_str.substr(at_pos + 1);
        PackageManager pkg_mgr;
        auto installed = pkg_mgr.get_project_by_name_and_version(pkg, ver);
        if (!installed) {
            std::cerr << "Error: Installed project '" << pkg << "' with version '" << ver << "' not found" << std::endl;
            return result;
        }
        manifest_path = installed->path / "solix.json";
    } else {
        manifest_path = target_str;
        if (std::filesystem::is_directory(manifest_path)) {
            manifest_path /= "solix.json";
        }
    }
    manifest_path = manifest_path.lexically_normal();

    if (!std::filesystem::exists(manifest_path)) {
        std::cerr << "Error: Manifest file does not exist: " << manifest_path.string() << std::endl;
        return result;
    }

    std::filesystem::path project_root = manifest_path.parent_path();
    if (project_root.empty()) {
        project_root = std::filesystem::current_path();
    } else {
        project_root = std::filesystem::absolute(project_root);
    }
    result.project_root = project_root;

    std::ifstream manifest_file(manifest_path);
    if (!manifest_file.is_open()) {
        std::cerr << "Error: Could not open manifest file: " << manifest_path.string() << std::endl;
        return result;
    }

    nlohmann::json root;
    try {
        manifest_file >> root;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "Error: Failed to parse solix.json: " << e.what() << std::endl;
        return result;
    }
    result.manifest = root;

    // Validate profiles section
    if (!root.contains("profiles") || !root["profiles"].is_object()) {
        std::cerr << "Error: 'profiles' section missing or invalid in solix.json" << std::endl;
        return result;
    }

    if (!root["profiles"].contains(profile_name)) {
        std::cerr << "Error: Profile '" << profile_name << "' not defined in solix.json" << std::endl;
        return result;
    }

    const auto& profile = root["profiles"][profile_name];
    if (!profile.is_object()) {
        std::cerr << "Error: Profile '" << profile_name << "' must be an object." << std::endl;
        return result;
    }

    auto opts = std::make_shared<CompilationOptions>();

    // 1. Output directory & filenames
    std::string out_dir_str = profile.value("output_directory", "build/" + profile_name);
    std::string exe_filename_str = profile.value("exe_filename", "out.slxbin");
    std::string asm_filename_str = profile.value("asm_filename", "");

    std::filesystem::path out_dir(out_dir_str);
    if (out_dir.is_relative()) {
        out_dir = project_root / out_dir;
    }
    out_dir = out_dir.lexically_normal();

    std::filesystem::path out_exe = (out_dir / exe_filename_str).lexically_normal();
    if (!asm_filename_str.empty()) {
        opts->assembly_output_path = (out_dir / asm_filename_str).lexically_normal();
    }

    // 2. Compilation settings
    if (profile.contains("compilation") && profile["compilation"].is_object()) {
        const auto& comp = profile["compilation"];

        opts->entry_point = comp.value("entry_point", "main");
        opts->use_multithreading = comp.value("multithreaded", false);

        if (comp.contains("logs") && comp["logs"].is_object()) {
            const auto& logs = comp["logs"];
            if (logs.contains("level") && logs["level"].is_string()) {
                opts->log_level = map_log_level(logs["level"].get<std::string>());
            }
            if (logs.contains("flush_level") && logs["flush_level"].is_string()) {
                opts->flush_level = map_log_level(logs["flush_level"].get<std::string>());
            }
            if (logs.contains("sink") && logs["sink"].is_string()) {
                opts->sink_type = map_log_sink_type(logs["sink"].get<std::string>());
            }
            if (logs.contains("pattern") && logs["pattern"].is_string()) {
                opts->log_pattern = logs["pattern"].get<std::string>();
            }
            if (logs.contains("filename") && logs["filename"].is_string()) {
                std::string log_file_str = logs["filename"].get<std::string>();
                if (!log_file_str.empty()) {
                    std::filesystem::path log_path(log_file_str);
                    if (log_path.is_relative()) {
                        log_path = project_root / log_path;
                    }
                    opts->log_file_path = log_path.lexically_normal();
                }
            }
        }
    }

    // 3. Pre-Compilation Modular Dependency & Transitive Project Resolution
    DependencyManager dep_mgr;
    if (!dep_mgr.resolve_all(root, project_root, profile, *opts, &result.dependency_roots)) {
        return result;
    }


    if (opts->sources.empty()) {
        std::cerr << "Error: No source files found to compile in profile '" << profile_name << "'." << std::endl;
        return result;
    }

    // 4. Execute compilation and write binary
    int exit_code = execute_compilation_and_write(*opts, out_exe);
    if (exit_code != 0) {
        return result;
    }

    result.output_binary = out_exe;
    result.success = true;
    return result;
}

void setup_build_command(CLI::App &app) {
    auto *build_cmd = app.add_subcommand("build", "Build the Solix project using solix.json");

    auto profile_str = std::make_shared<std::string>("debug");
    auto target_str = std::make_shared<std::string>();

    build_cmd->add_option("-p,--profile", *profile_str, "Build profile (default: debug)");
    build_cmd->add_option("target", *target_str, "Path to project directory, solix.json, or package@version (default: solix.json)");

    build_cmd->callback([profile_str, target_str]() {
        auto build_res = build_project(*target_str, *profile_str);
        if (!build_res.success) {
            cli_exit(1);
        }
    });
}

} // namespace solix::cli
