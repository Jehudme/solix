#include "pack.hpp"
#include "build.hpp"
#include "../cli_utils.hpp"
#include "solix/zip_utils.hpp"
#include "solix/path_utils.hpp"
#include <iostream>
#include <iomanip>
#include <set>
#include <algorithm>

namespace solix::cli {

namespace {

inline bool is_shared_library(const std::filesystem::path& p) {
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    return ext == ".so" || ext == ".dll" || ext == ".dylib";
}

inline void collect_native_libs_from_dir(const std::filesystem::path& dir,
                                        std::vector<std::filesystem::path>& out,
                                        std::set<std::string>& seen_filenames) {
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec)) {
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (entry.is_regular_file(ec) && is_shared_library(entry.path())) {
            std::string filename = entry.path().filename().string();
            if (seen_filenames.find(filename) == seen_filenames.end()) {
                seen_filenames.insert(filename);
                out.push_back(entry.path().lexically_normal());
            }
        }
    }
}

static std::string format_size(uintmax_t bytes) {
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << (static_cast<double>(bytes) / 1024.0) << " KB";
        return ss.str();
    }
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << (static_cast<double>(bytes) / (1024.0 * 1024.0)) << " MB";
    return ss.str();
}

} // namespace

PackResult pack_project(const std::filesystem::path& manifest_or_dir,
                        const std::string& profile_name,
                        const std::filesystem::path& explicit_output_path) {
    PackResult result;

    // 1. Build the project using the specified profile
    ProjectBuildResult build_res = build_project(manifest_or_dir, profile_name);
    if (!build_res.success) {
        result.error_message = "Failed to build project before packaging.";
        return result;
    }

    // 2. Extract project metadata from manifest
    std::string project_name = "app";
    std::string version = "1.0.0";
    if (build_res.manifest.contains("name") && build_res.manifest["name"].is_string()) {
        project_name = build_res.manifest["name"].get<std::string>();
    }
    if (build_res.manifest.contains("version") && build_res.manifest["version"].is_string()) {
        version = build_res.manifest["version"].get<std::string>();
    }

    // 3. Determine output .slxpac file path
    std::filesystem::path out_pac;
    if (!explicit_output_path.empty()) {
        out_pac = explicit_output_path;
        if (out_pac.is_relative()) {
            out_pac = std::filesystem::current_path() / out_pac;
        }
    } else {
        out_pac = build_res.output_binary.parent_path() / (project_name + ".slxpac");
    }
    out_pac = out_pac.lexically_normal();
    result.output_package = out_pac;

    // Ensure parent directory exists
    std::error_code ec;
    if (out_pac.has_parent_path()) {
        std::filesystem::create_directories(out_pac.parent_path(), ec);
    }

    // Remove existing file if any
    std::filesystem::remove(out_pac, ec);

    // 4. Initialize ZipArchiveWriter
    ZipArchiveWriter writer(out_pac);
    if (!writer.is_valid()) {
        result.error_message = "Failed to open output package archive: " + writer.get_error();
        return result;
    }

    size_t total_entries = 0;

    // 5. Add bytecode as bytecode.slxbin
    if (!writer.add_file("bytecode.slxbin", build_res.output_binary)) {
        result.error_message = "Failed to add bytecode to package: " + writer.get_error();
        return result;
    }
    total_entries++;

    // 6. Collect and add native libraries into lib/
    std::vector<std::filesystem::path> native_libs;
    std::set<std::string> seen_lib_filenames;

    // A. Root project lib/
    collect_native_libs_from_dir(build_res.project_root / "lib", native_libs, seen_lib_filenames);

    // B. Dependency projects lib/
    for (const auto& dep_root : build_res.dependency_roots) {
        collect_native_libs_from_dir(dep_root / "lib", native_libs, seen_lib_filenames);
    }

    // C. Output binary parent directory (and parent/lib)
    collect_native_libs_from_dir(build_res.output_binary.parent_path(), native_libs, seen_lib_filenames);
    collect_native_libs_from_dir(build_res.output_binary.parent_path() / "lib", native_libs, seen_lib_filenames);

    for (const auto& lib_path : native_libs) {
        std::string entry_name = "lib/" + lib_path.filename().string();
        if (writer.add_file(entry_name, lib_path)) {
            total_entries++;
        }
    }

    // 7. Collect and add resources into resources/
    std::filesystem::path default_res_dir = build_res.project_root / "resources";
    if (std::filesystem::exists(default_res_dir, ec) && std::filesystem::is_directory(default_res_dir, ec)) {
        writer.add_directory_recursive(default_res_dir, "resources");
    }

    // Check custom manifest resources definition if present
    if (build_res.manifest.contains("resources")) {
        const auto& res_item = build_res.manifest["resources"];
        if (res_item.is_string()) {
            std::filesystem::path custom_res = build_res.project_root / res_item.get<std::string>();
            if (std::filesystem::exists(custom_res, ec) && std::filesystem::is_directory(custom_res, ec) && custom_res != default_res_dir) {
                writer.add_directory_recursive(custom_res, "resources");
            }
        } else if (res_item.is_array()) {
            for (const auto& elem : res_item) {
                if (elem.is_string()) {
                    std::filesystem::path custom_res = build_res.project_root / elem.get<std::string>();
                    if (std::filesystem::exists(custom_res, ec) && std::filesystem::is_directory(custom_res, ec)) {
                        writer.add_directory_recursive(custom_res, "resources");
                    }
                }
            }
        }
    }

    // 8. Generate and add internal bundle manifest.json
    nlohmann::json bundle_manifest;
    bundle_manifest["name"] = project_name;
    bundle_manifest["version"] = version;
    bundle_manifest["format_version"] = 1;
    bundle_manifest["entry"] = "bytecode.slxbin";
    bundle_manifest["resources_dir"] = "resources";
    bundle_manifest["native_lib_dir"] = "lib";
    bundle_manifest["profile"] = profile_name;
    bundle_manifest["created_by"] = "solix pack";

    if (!writer.add_string("manifest.json", bundle_manifest.dump(2))) {
        result.error_message = "Failed to add manifest.json to package: " + writer.get_error();
        return result;
    }
    total_entries++;

    // 9. Finalize package archive
    if (!writer.finalize()) {
        result.error_message = "Failed to finalize package archive: " + writer.get_error();
        return result;
    }

    result.file_count = total_entries;
    result.package_size_bytes = std::filesystem::file_size(out_pac, ec);
    result.success = true;
    return result;
}

void setup_pack_command(CLI::App &app) {
    auto *pack_cmd = app.add_subcommand("pack", "Package a Solix project into a single .slxpac archive");

    auto target_str = std::make_shared<std::string>();
    auto profile_str = std::make_shared<std::string>("release");
    auto output_str = std::make_shared<std::string>();

    pack_cmd->add_option("target", *target_str, "Path to project directory or manifest.json (default: current directory)");
    pack_cmd->add_option("-p,--profile", *profile_str, "Build profile to use (default: release)");
    pack_cmd->add_option("-o,--output", *output_str, "Output .slxpac package file path");

    pack_cmd->callback([target_str, profile_str, output_str]() {
        std::filesystem::path target_path = target_str->empty() ? std::filesystem::current_path() : std::filesystem::path(*target_str);
        std::filesystem::path explicit_out = output_str->empty() ? std::filesystem::path() : std::filesystem::path(*output_str);

        auto pack_res = pack_project(target_path, *profile_str, explicit_out);
        if (!pack_res.success) {
            std::cerr << "Error: " << pack_res.error_message << std::endl;
            cli_exit(1);
        }

        std::cout << "Successfully created Solix package:" << std::endl;
        std::cout << "  Location: " << pack_res.output_package.string() << std::endl;
        std::cout << "  Size:     " << format_size(pack_res.package_size_bytes) << std::endl;
    });
}

} // namespace solix::cli
