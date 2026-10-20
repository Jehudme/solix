#include "pack.hpp"
#include "build.hpp"
#include "../cli_utils.hpp"
#include "../package_manager.hpp"
#include "solix/zip_utils.hpp"
#include "solix/path_utils.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <cctype>

namespace solix::cli {

namespace {

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
    PackageManager pm;

    std::filesystem::path staging_dir;
    std::filesystem::path temp_staging_dir;
    bool is_temp = false;
    std::string project_name = "app";
    std::string version = "1.0.0";
    std::filesystem::path base_project_dir;

    std::string target_str = manifest_or_dir.string();
    auto at_pos = target_str.find('@');

    // Check if target is an installed package (<name>@<version> or name in pm)
    if (at_pos != std::string::npos && !std::filesystem::exists(manifest_or_dir)) {
        std::string req_name = target_str.substr(0, at_pos);
        std::string req_ver = target_str.substr(at_pos + 1);
        auto installed = pm.get_project_by_name_and_version(req_name, req_ver);
        if (!installed) {
            result.error_message = "Installed project '" + req_name + "' version '" + req_ver + "' not found.";
            return result;
        }
        staging_dir = installed->path;
        project_name = installed->name;
        version = installed->version;
    } else if (!std::filesystem::exists(manifest_or_dir) && !target_str.empty()) {
        auto matches = pm.get_projects_by_name(target_str);
        if (!matches.empty()) {
            const auto& installed = matches[0];
            staging_dir = installed.path;
            project_name = installed.name;
            version = installed.version;
        } else {
            result.error_message = "Target directory, manifest, or installed package not found: " + target_str;
            return result;
        }
    } else {
        // Project directory or manifest file
        std::filesystem::path proj_root = std::filesystem::absolute(manifest_or_dir);
        if (std::filesystem::is_regular_file(proj_root)) {
            proj_root = proj_root.parent_path();
        }
        proj_root = proj_root.lexically_normal();
        base_project_dir = proj_root;

        std::filesystem::path mf_path = solix::find_project_manifest(proj_root);
        if (!std::filesystem::exists(mf_path)) {
            result.error_message = "manifest.json (or solix.json) not found in " + proj_root.string();
            return result;
        }

        nlohmann::json manifest;
        try {
            std::ifstream f(mf_path);
            f >> manifest;
        } catch (const std::exception& e) {
            result.error_message = "Failed to parse manifest: " + std::string(e.what());
            return result;
        }

        project_name = manifest.value("project", manifest.value("name", proj_root.filename().string()));
        version = manifest.value("version", "0.1.0");
        std::string proj_type = manifest.value("type", "app");
        std::transform(proj_type.begin(), proj_type.end(), proj_type.begin(), [](unsigned char c) { return std::tolower(c); });
        if (proj_type == "lib") proj_type = "library";


        // Create temporary staging directory
        auto temp_base = std::filesystem::temp_directory_path() / "solix_pack_staging";
        std::error_code tec;
        std::filesystem::create_directories(temp_base, tec);
        auto t_now = std::chrono::steady_clock::now().time_since_epoch().count();
        temp_staging_dir = temp_base / ("stage_" + std::to_string(t_now));
        std::filesystem::create_directories(temp_staging_dir, tec);

        std::string out_id;
        if (!pm.install_project(proj_root, profile_name, true, false, temp_staging_dir, out_id)) {
            std::filesystem::remove_all(temp_staging_dir, tec);
            result.error_message = "Failed to build and stage project before packaging.";
            return result;
        }
        staging_dir = temp_staging_dir;
        is_temp = true;
    }

    struct TempDirGuard {
        std::filesystem::path dir;
        bool active;
        ~TempDirGuard() {
            if (active && !dir.empty()) {
                std::error_code tec;
                std::filesystem::remove_all(dir, tec);
            }
        }
    } guard{temp_staging_dir, is_temp};

    // Determine output package path
    std::filesystem::path out_pac;
    if (!explicit_output_path.empty()) {
        out_pac = explicit_output_path;
        if (out_pac.is_relative()) {
            out_pac = std::filesystem::current_path() / out_pac;
        }
    } else if (!base_project_dir.empty()) {
        out_pac = base_project_dir / "build" / profile_name / (project_name + ".slxpac");
    } else {
        out_pac = std::filesystem::current_path() / (project_name + "-" + version + ".slxpac");
    }
    out_pac = out_pac.lexically_normal();
    result.output_package = out_pac;

    std::error_code ec;
    if (out_pac.has_parent_path()) {
        std::filesystem::create_directories(out_pac.parent_path(), ec);
    }
    std::filesystem::remove(out_pac, ec);

    // Initialize ZipArchiveWriter and pack
    ZipArchiveWriter writer(out_pac);
    if (!writer.is_valid()) {
        result.error_message = "Failed to open output package archive: " + writer.get_error();
        return result;
    }

    if (!writer.add_directory_recursive(staging_dir, "")) {
        result.error_message = "Failed to add files to package archive: " + writer.get_error();
        return result;
    }

    if (!writer.finalize()) {
        result.error_message = "Failed to finalize package archive: " + writer.get_error();
        return result;
    }

    ZipArchiveReader reader(out_pac);
    result.file_count = reader.get_file_count();
    result.package_size_bytes = std::filesystem::file_size(out_pac, ec);
    result.success = true;
    return result;
}

void setup_pack_command(CLI::App &app) {
    auto *pack_cmd = app.add_subcommand("pack", "Package a Solix project into a single .slxpac archive");
    pack_cmd->alias("package");

    auto target_str = std::make_shared<std::string>();
    auto profile_str = std::make_shared<std::string>("release");
    auto output_str = std::make_shared<std::string>();
    auto package_opt = std::make_shared<std::string>();
    auto path_opt = std::make_shared<std::string>();

    pack_cmd->add_option("target", *target_str, "Path to project directory, manifest.json, or package@version (default: current directory)");
    pack_cmd->add_option("-p,--profile", *profile_str, "Build profile to use (default: release)");
    pack_cmd->add_option("-o,--output", *output_str, "Output .slxpac package file path");
    pack_cmd->add_option("-P,--package", *package_opt, "Installed package name in $SOLIX_HOME");
    pack_cmd->add_option("-i,--input,--path", *path_opt, "Path to project directory or manifest.json");

    pack_cmd->callback([target_str, profile_str, output_str, package_opt, path_opt]() {
        std::string raw_target = ".";
        if (!package_opt->empty()) {
            raw_target = *package_opt;
        } else if (!path_opt->empty()) {
            raw_target = *path_opt;
        } else if (!target_str->empty()) {
            raw_target = *target_str;
        }

        std::filesystem::path target_path(raw_target);
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
