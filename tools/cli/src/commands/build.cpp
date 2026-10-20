#include "build.hpp"
#include "compile.hpp"
#include "../cli_utils.hpp"
#include "../command_runner.hpp"
#include "../dependency_resolver.hpp"
#include "../package_manager.hpp"
#include "solix/compilation.hpp"
#include "solix/path_utils.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <algorithm>
#include "solix/shared_library.hpp"
#include <nlohmann/json.hpp>

namespace solix::cli {

namespace {

inline bool is_shared_library(const std::filesystem::path& p) {
    return solix::is_any_shared_library(p);
}

void stage_native_libraries(const std::filesystem::path& project_root,
                            const std::vector<std::filesystem::path>& dependency_roots,
                            const std::filesystem::path& out_dir) {
    std::map<std::string, std::filesystem::path> libs_to_copy;
    std::error_code ec;

    // 1. Collect from dependencies (transitive)
    for (const auto& dep_root : dependency_roots) {
        std::filesystem::path dep_lib = dep_root / "lib";
        if (std::filesystem::exists(dep_lib, ec) && std::filesystem::is_directory(dep_lib, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(dep_lib, ec)) {
                if (entry.is_regular_file(ec) && is_shared_library(entry.path())) {
                    libs_to_copy[entry.path().filename().string()] = entry.path();
                }
            }
        }
    }

    // 2. Collect from root project (overrides dependencies on collision)
    std::filesystem::path root_lib = project_root / "lib";
    if (std::filesystem::exists(root_lib, ec) && std::filesystem::is_directory(root_lib, ec)) {
        for (const auto& entry : std::filesystem::directory_iterator(root_lib, ec)) {
            if (entry.is_regular_file(ec) && is_shared_library(entry.path())) {
                libs_to_copy[entry.path().filename().string()] = entry.path();
            }
        }
    }

    // 3. Stage into out_dir / "lib"
    if (!libs_to_copy.empty()) {
        std::filesystem::path target_lib_dir = out_dir / "lib";
        std::filesystem::create_directories(target_lib_dir, ec);
        for (const auto& [filename, src_path] : libs_to_copy) {
            std::filesystem::path dst_path = target_lib_dir / filename;
            if (src_path != dst_path) {
                std::filesystem::copy_file(src_path, dst_path, std::filesystem::copy_options::overwrite_existing, ec);
            }
        }
    }
}

void collect_resources_from_dir(const std::filesystem::path& base_dir,
                                std::map<std::string, std::filesystem::path>& resource_map) {
    std::error_code ec;
    if (!std::filesystem::exists(base_dir, ec) || !std::filesystem::is_directory(base_dir, ec)) {
        return;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(base_dir, ec)) {
        if (entry.is_regular_file(ec)) {
            std::filesystem::path rel = std::filesystem::relative(entry.path(), base_dir, ec);
            resource_map[rel.generic_string()] = entry.path();
        }
    }
}

void stage_resources(const std::filesystem::path& project_root,
                     const nlohmann::json& manifest,
                     const std::vector<std::filesystem::path>& dependency_roots,
                     const std::filesystem::path& out_dir) {
    std::map<std::string, std::filesystem::path> resources_to_copy;
    std::error_code ec;

    // 1. Collect from dependencies (transitive)
    for (const auto& dep_root : dependency_roots) {
        collect_resources_from_dir(dep_root / "resources", resources_to_copy);
    }

    // 2. Collect from root project (overrides dependencies on collision)
    collect_resources_from_dir(project_root / "resources", resources_to_copy);

    if (manifest.contains("resources")) {
        const auto& res_item = manifest["resources"];
        if (res_item.is_string()) {
            std::filesystem::path custom_res = project_root / res_item.get<std::string>();
            collect_resources_from_dir(custom_res, resources_to_copy);
        } else if (res_item.is_array()) {
            for (const auto& elem : res_item) {
                if (elem.is_string()) {
                    std::filesystem::path custom_res = project_root / elem.get<std::string>();
                    collect_resources_from_dir(custom_res, resources_to_copy);
                }
            }
        }
    }

    // 3. Stage into out_dir / "resources"
    if (!resources_to_copy.empty()) {
        std::filesystem::path target_res_dir = out_dir / "resources";
        std::filesystem::create_directories(target_res_dir, ec);
        for (const auto& [rel_path_str, src_path] : resources_to_copy) {
            std::filesystem::path dst_path = target_res_dir / rel_path_str;
            if (dst_path.has_parent_path()) {
                std::filesystem::create_directories(dst_path.parent_path(), ec);
            }
            if (src_path != dst_path) {
                std::filesystem::copy_file(src_path, dst_path, std::filesystem::copy_options::overwrite_existing, ec);
            }
        }
    }
}

} // namespace

ProjectBuildResult build_project(const std::filesystem::path& manifest_or_dir,
                                 const std::string& profile_name) {
    ProjectBuildResult result;
    result.profile_name = profile_name;

    std::string target_str = manifest_or_dir.string();

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
        manifest_path = solix::find_project_manifest(installed->path);
    } else {
        std::filesystem::path p = target_str.empty() ? std::filesystem::current_path() : std::filesystem::path(target_str);
        manifest_path = solix::find_project_manifest(p);
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

    // 3. Resolve Dependency Graph & Selected Profiles
    DependencyManager dep_mgr;
    std::vector<ProjectCandidate> candidates;
    if (!dep_mgr.resolve_graph(root, project_root, profile, profile_name, candidates)) {
        return result;
    }

    for (const auto& proj : candidates) {
        if (proj.project_root != project_root) {
            result.dependency_roots.push_back(proj.project_root);
        }
    }

    // 4. Topological Dependency Pre-Compile Hooks (Deepest dependencies first, in dependency roots)
    for (const auto& dep : candidates) {
        if (dep.project_root == project_root) {
            continue;
        }
        auto dep_root_pre = CommandRunner::extract_commands(dep.manifest, "pre_compile", "precompile");
        if (!CommandRunner::run_commands(dep_root_pre, dep.project_root, "pre-compile")) {
            return result;
        }
        if (!dep.selected_profile.empty() && dep.manifest.contains("profiles") &&
            dep.manifest["profiles"].is_object() && dep.manifest["profiles"].contains(dep.selected_profile)) {
            const auto& dep_prof = dep.manifest["profiles"][dep.selected_profile];
            auto dep_prof_pre = CommandRunner::extract_commands(dep_prof, "pre_compile", "precompile");
            if (!CommandRunner::run_commands(dep_prof_pre, dep.project_root, "pre-compile")) {
                return result;
            }
        }
    }

    // 5. Root Project Pre-Compile Lifecycle Commands
    auto root_pre_cmds = CommandRunner::extract_commands(root, "pre_compile", "precompile");
    if (!CommandRunner::run_commands(root_pre_cmds, project_root, "pre-compile")) {
        return result;
    }

    // 6. Active Profile Pre-Compile Lifecycle Commands
    auto profile_pre_cmds = CommandRunner::extract_commands(profile, "pre_compile", "precompile");
    if (!CommandRunner::run_commands(profile_pre_cmds, project_root, "pre-compile")) {
        return result;
    }

    // 7. Source Ingestion across all candidates
    if (!dep_mgr.ingest_sources(candidates, *opts, profile, project_root)) {
        return result;
    }

    if (opts->sources.empty()) {
        std::cerr << "Error: No source files found to compile in profile '" << profile_name << "'." << std::endl;
        return result;
    }

    // 5. Execute compilation and write binary
    int exit_code = execute_compilation_and_write(*opts, out_exe);
    if (exit_code != 0) {
        return result;
    }

    // 6. Stage transitive native libraries and resources into output directory
    stage_native_libraries(project_root, result.dependency_roots, out_dir);
    stage_resources(project_root, root, result.dependency_roots, out_dir);

    // 7. Post-Compile Lifecycle Commands (Active Profile -> Root)
    auto profile_post_cmds = CommandRunner::extract_commands(profile, "post_compile", "postcompile");
    if (!CommandRunner::run_commands(profile_post_cmds, project_root, "post-compile")) {
        return result;
    }

    auto root_post_cmds = CommandRunner::extract_commands(root, "post_compile", "postcompile");
    if (!CommandRunner::run_commands(root_post_cmds, project_root, "post-compile")) {
        return result;
    }

    result.output_binary = out_exe;
    result.success = true;
    return result;
}

void setup_build_command(CLI::App &app) {
    auto *build_cmd = app.add_subcommand("build", "Build the Solix project using manifest.json (or solix.json)");

    auto profile_str = std::make_shared<std::string>("debug");
    auto target_str = std::make_shared<std::string>();

    build_cmd->add_option("-p,--profile", *profile_str, "Build profile (default: debug)");
    build_cmd->add_option("target", *target_str, "Path to project directory, manifest.json, or package@version (default: manifest.json)");

    build_cmd->callback([profile_str, target_str]() {
        auto build_res = build_project(*target_str, *profile_str);
        if (!build_res.success) {
            cli_exit(1);
        }
    });
}

} // namespace solix::cli
