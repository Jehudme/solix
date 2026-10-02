#include "build.hpp"
#include "compile.hpp"
#include "../cli_utils.hpp"
#include "../dependency_resolver.hpp"
#include "solix/compilation.hpp"
#include "solix/path_utils.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace solix::cli {

void setup_build_command(CLI::App &app) {
    auto *build_cmd = app.add_subcommand("build", "Build the Solix project using solix.json");

    auto profile_str = std::make_shared<std::string>("debug");
    auto manifest_path_str = std::make_shared<std::string>("solix.json");

    build_cmd->add_option("-p,--profile", *profile_str, "Build profile (default: debug)");
    build_cmd->add_option("-m,--manifest", *manifest_path_str, "Path to solix.json or project directory (default: solix.json)");

    build_cmd->callback([profile_str, manifest_path_str]() {
        std::filesystem::path manifest_path(*manifest_path_str);
        if (std::filesystem::is_directory(manifest_path)) {
            manifest_path /= "solix.json";
        }
        manifest_path = manifest_path.lexically_normal();

        if (!std::filesystem::exists(manifest_path)) {
            std::cerr << "Error: Manifest file does not exist: " << manifest_path.string() << std::endl;
            cli_exit(1);
        }

        std::filesystem::path project_root = manifest_path.parent_path();
        if (project_root.empty()) {
            project_root = std::filesystem::current_path();
        } else {
            project_root = std::filesystem::absolute(project_root);
        }

        std::ifstream manifest_file(manifest_path);
        if (!manifest_file.is_open()) {
            std::cerr << "Error: Could not open manifest file: " << manifest_path.string() << std::endl;
            cli_exit(1);
        }

        nlohmann::json root;
        try {
            manifest_file >> root;
        } catch (const nlohmann::json::parse_error& e) {
            std::cerr << "Error: Failed to parse solix.json: " << e.what() << std::endl;
            cli_exit(1);
        }

        // Validate profiles section
        if (!root.contains("profiles") || !root["profiles"].is_object()) {
            std::cerr << "Error: 'profiles' section missing or invalid in solix.json" << std::endl;
            cli_exit(1);
        }

        if (!root["profiles"].contains(*profile_str)) {
            std::cerr << "Error: Profile '" << *profile_str << "' not defined in solix.json" << std::endl;
            cli_exit(1);
        }

        const auto& profile = root["profiles"][*profile_str];
        if (!profile.is_object()) {
            std::cerr << "Error: Profile '" << *profile_str << "' must be an object." << std::endl;
            cli_exit(1);
        }

        auto opts = std::make_shared<CompilationOptions>();

        // 1. Output directory & filenames
        std::string out_dir_str = profile.value("output_directory", "build/" + *profile_str);
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

        // 3. Modular Dependency Resolution
        DependencyManager dep_mgr;

        // Resolve root dependencies
        if (root.contains("dependencies") && root["dependencies"].is_array()) {
            for (const auto& dep : root["dependencies"]) {
                if (!dep_mgr.resolve_dependency(dep, project_root, *opts)) {
                    cli_exit(1);
                }
            }
        }

        // Resolve profile-specific additional_dependencies
        if (profile.contains("compilation") && profile["compilation"].is_object()) {
            const auto& comp = profile["compilation"];
            if (comp.contains("additional_dependencies") && comp["additional_dependencies"].is_array()) {
                for (const auto& dep : comp["additional_dependencies"]) {
                    if (!dep_mgr.resolve_dependency(dep, project_root, *opts)) {
                        cli_exit(1);
                    }
                }
            }
        }

        if (opts->sources.empty()) {
            std::cerr << "Error: No source files found to compile in profile '" << *profile_str << "'." << std::endl;
            cli_exit(1);
        }

        // 4. Execute compilation and write binary
        int exit_code = execute_compilation_and_write(*opts, out_exe);
        if (exit_code != 0) {
            cli_exit(exit_code);
        }
    });
}

} // namespace solix::cli
