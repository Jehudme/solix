#include "package.hpp"
#include "../cli_utils.hpp"
#include "../package_manager.hpp"
#include "../semver.hpp"
#include "solix/zip_utils.hpp"
#include <iostream>
#include <iomanip>
#include <memory>
#include <algorithm>
#include <cctype>

namespace solix::cli {

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

void setup_package_commands(CLI::App &app) {
    // 1. install
    auto *install_cmd = app.add_subcommand("install", "Install a Solix project or package bundle locally");
    auto install_path = std::make_shared<std::string>(".");
    auto profile_str = std::make_shared<std::string>("release");
    auto force_flag = std::make_shared<bool>(false);
    auto archive_flag = std::make_shared<bool>(false);
    auto output_str = std::make_shared<std::string>();
    auto alias_str = std::make_shared<std::string>();

    install_cmd->add_option("path", *install_path, "Path to the project directory, manifest, or package bundle (.slxpac)");
    install_cmd->add_option("-p,--profile", *profile_str, "Build profile to use (default: release)");
    install_cmd->add_flag("-f,--force", *force_flag, "Reinstall / overwrite if already installed");
    install_cmd->add_flag("--archive,--as-is", *archive_flag, "Install an already-prepared package directory or bundle without rebuilding");
    install_cmd->add_option("-o,--output", *output_str, "Output destination directory (stages without registering in registry)");
    install_cmd->add_option("--alias,--as", *alias_str, "Install or stage under an alternative project name");

    install_cmd->callback([install_path, profile_str, force_flag, archive_flag, output_str, alias_str]() {
        PackageManager pm;
        std::string id;
        std::filesystem::path out_dir = output_str->empty() ? std::filesystem::path() : std::filesystem::path(*output_str);
        if (!pm.install_project(std::filesystem::path(*install_path), *profile_str, *force_flag, *archive_flag, out_dir, id, *alias_str)) {
            cli_exit(1);
        }
        if (out_dir.empty()) {
            auto installed_dir = PackageManager::get_installed_dir() / id;
            std::cout << "Successfully installed project (ID: " << id << ")" << std::endl;
            std::cout << "  Location: " << installed_dir.string() << std::endl;
        } else {
            std::cout << "Successfully prepared and staged project (ID: " << id << ")" << std::endl;
            std::cout << "  Location: " << std::filesystem::absolute(out_dir).lexically_normal().string() << std::endl;
        }
    });

    // 1b. archive (alias for install -o)
    auto *archive_cmd = app.add_subcommand("archive", "Build and export a clean, prepared project distribution directory");
    auto archive_path = std::make_shared<std::string>(".");
    auto archive_profile = std::make_shared<std::string>("release");
    auto archive_output = std::make_shared<std::string>();
    auto archive_force = std::make_shared<bool>(false);
    auto archive_alias = std::make_shared<std::string>();

    archive_cmd->add_option("path", *archive_path, "Path to the project directory (default: current directory)");
    archive_cmd->add_option("-p,--profile", *archive_profile, "Build profile to use (default: release)");
    archive_cmd->add_option("-o,--output", *archive_output, "Output destination directory")->required();
    archive_cmd->add_flag("-f,--force", *archive_force, "Overwrite output destination if it exists");
    archive_cmd->add_option("--alias,--as", *archive_alias, "Export staged distribution under an alternative project name");

    archive_cmd->callback([archive_path, archive_profile, archive_output, archive_force, archive_alias]() {
        PackageManager pm;
        std::string id;
        std::filesystem::path out_dir(*archive_output);
        if (!pm.install_project(std::filesystem::path(*archive_path), *archive_profile, *archive_force, false, out_dir, id, *archive_alias)) {
            cli_exit(1);
        }
        std::cout << "Successfully archived project (ID: " << id << ")" << std::endl;
        std::cout << "  Location: " << std::filesystem::absolute(out_dir).lexically_normal().string() << std::endl;
    });

    // 2. uninstall
    auto *uninstall_cmd = app.add_subcommand("uninstall", "Uninstall a Solix project by name and version");
    auto uninstall_name = std::make_shared<std::string>();
    auto uninstall_version = std::make_shared<std::string>();

    uninstall_cmd->add_option("name", *uninstall_name, "Name of the project to uninstall")->required();
    uninstall_cmd->add_option("version", *uninstall_version, "Version of the project to uninstall")->required();

    uninstall_cmd->callback([uninstall_name, uninstall_version]() {
        PackageManager pm;
        if (!pm.uninstall_project(*uninstall_name, *uninstall_version)) {
            cli_exit(1);
        }
        std::cout << "Successfully uninstalled '" << *uninstall_name 
                  << "' version '" << *uninstall_version << "'." << std::endl;
    });

    // 3. list
    auto *list_cmd = app.add_subcommand("list", "List installed Solix projects");
    auto filter_type = std::make_shared<std::string>();
    auto filter_query = std::make_shared<std::string>();
    auto sort_field = std::make_shared<std::string>("name");
    auto reverse_flag = std::make_shared<bool>(false);
    auto json_flag = std::make_shared<bool>(false);

    list_cmd->add_option("-t,--type", *filter_type, "Filter by project type ('app', 'library', 'plugin')");
    list_cmd->add_option("-f,--filter", *filter_query, "Case-insensitive query string to filter by name, description, or id");
    list_cmd->add_option("-s,--sort,--order", *sort_field, "Sort field: 'name', 'version', 'type', 'date' (default: 'name')");
    list_cmd->add_flag("--reverse", *reverse_flag, "Reverse the sort order");
    list_cmd->add_flag("--json", *json_flag, "Output installed projects list as raw JSON");

    list_cmd->callback([filter_type, filter_query, sort_field, reverse_flag, json_flag]() {
        PackageManager pm;
        auto projects = pm.list_installed_projects();

        // 1. Filter by type
        if (!filter_type->empty()) {
            std::string ft = *filter_type;
            std::transform(ft.begin(), ft.end(), ft.begin(), [](unsigned char c) { return std::tolower(c); });
            if (ft == "lib") ft = "library";
            std::vector<InstalledProject> filtered;
            for (const auto& p : projects) {
                std::string pt = p.type;
                std::transform(pt.begin(), pt.end(), pt.begin(), [](unsigned char c) { return std::tolower(c); });
                if (pt == ft) {
                    filtered.push_back(p);
                }
            }
            projects = std::move(filtered);
        }

        // 2. Filter by search query
        if (!filter_query->empty()) {
            std::string q = *filter_query;
            std::transform(q.begin(), q.end(), q.begin(), [](unsigned char c) { return std::tolower(c); });
            std::vector<InstalledProject> filtered;
            for (const auto& p : projects) {
                std::string n = p.name;
                std::string d = p.description;
                std::string id = p.id;
                std::transform(n.begin(), n.end(), n.begin(), [](unsigned char c) { return std::tolower(c); });
                std::transform(d.begin(), d.end(), d.begin(), [](unsigned char c) { return std::tolower(c); });
                std::transform(id.begin(), id.end(), id.begin(), [](unsigned char c) { return std::tolower(c); });
                if (n.find(q) != std::string::npos || d.find(q) != std::string::npos || id.find(q) != std::string::npos) {
                    filtered.push_back(p);
                }
            }
            projects = std::move(filtered);
        }

        // 3. Sort
        std::string sf = *sort_field;
        std::transform(sf.begin(), sf.end(), sf.begin(), [](unsigned char c) { return std::tolower(c); });
        std::sort(projects.begin(), projects.end(), [sf](const InstalledProject& a, const InstalledProject& b) {
            if (sf == "version") {
                auto va = SemVer::parse(a.version).value_or(SemVer{0, 0, 0, a.version});
                auto vb = SemVer::parse(b.version).value_or(SemVer{0, 0, 0, b.version});
                return va < vb;
            } else if (sf == "type") {
                return a.type < b.type;
            } else if (sf == "date" || sf == "installed_at") {
                return a.installed_at < b.installed_at;
            } else {
                return a.name < b.name;
            }
        });

        if (*reverse_flag) {
            std::reverse(projects.begin(), projects.end());
        }

        // 4. Output
        if (*json_flag) {
            nlohmann::json j_arr = nlohmann::json::array();
            for (const auto& p : projects) {
                j_arr.push_back({
                    {"id", p.id},
                    {"name", p.name},
                    {"version", p.version},
                    {"type", p.type},
                    {"path", p.path.string()},
                    {"original_path", p.original_path.string()},
                    {"installed_at", p.installed_at},
                    {"description", p.description}
                });
            }
            std::cout << j_arr.dump(2) << std::endl;
            return;
        }

        if (projects.empty()) {
            if (filter_type->empty() && filter_query->empty()) {
                std::cout << "No Solix projects are currently installed." << std::endl;
            } else {
                std::cout << "No Solix projects found matching the criteria." << std::endl;
            }
            return;
        }

        std::cout << "Installed Solix Projects (" << projects.size() << "):\n\n";
        std::cout << std::left 
                  << std::setw(24) << "NAME" 
                  << std::setw(12) << "VERSION" 
                  << std::setw(12) << "TYPE"
                  << std::setw(18) << "ID" 
                  << "INSTALLED PATH\n";
        std::cout << std::string(90, '-') << "\n";

        for (const auto& p : projects) {
            std::cout << std::left 
                      << std::setw(24) << p.name
                      << std::setw(12) << p.version
                      << std::setw(12) << p.type
                      << std::setw(18) << p.id
                      << p.path.string() << "\n";
        }
    });

    // 4. details
    auto *details_cmd = app.add_subcommand("details", "Display extensive details about an installed project or package bundle");
    auto target_str = std::make_shared<std::string>();
    auto version_str = std::make_shared<std::string>();
    auto explicit_package = std::make_shared<std::string>();
    auto explicit_path = std::make_shared<std::string>();

    // Scripting query flags
    auto q_name = std::make_shared<bool>(false);
    auto q_version = std::make_shared<bool>(false);
    auto q_type = std::make_shared<bool>(false);
    auto q_desc = std::make_shared<bool>(false);
    auto q_author = std::make_shared<bool>(false);
    auto q_license = std::make_shared<bool>(false);
    auto q_id = std::make_shared<bool>(false);
    auto q_path = std::make_shared<bool>(false);
    auto q_entry = std::make_shared<bool>(false);
    auto q_json = std::make_shared<bool>(false);

    details_cmd->add_option("target", *target_str, "Project name, <name>@<version>, or path to .slxpac package bundle");
    details_cmd->add_option("ver", *version_str, "Version of the project (optional if unique)");
    details_cmd->add_option("-P,--package", *explicit_package, "Explicit installed package name");
    details_cmd->add_option("-i,--input", *explicit_path, "Explicit path to project, directory, or package bundle");

    details_cmd->add_flag("-n,--name", *q_name, "Print project name");
    details_cmd->add_flag("-v,--version", *q_version, "Print project version");
    details_cmd->add_flag("-t,--type", *q_type, "Print project type");
    details_cmd->add_flag("-d,--description", *q_desc, "Print project description");
    details_cmd->add_flag("-a,--author", *q_author, "Print project author");
    details_cmd->add_flag("-l,--license", *q_license, "Print project license");
    details_cmd->add_flag("--id", *q_id, "Print project ID");
    details_cmd->add_flag("--path", *q_path, "Print project path");
    details_cmd->add_flag("--entry", *q_entry, "Print entry point");
    details_cmd->add_flag("--json", *q_json, "Output metadata as JSON");

    details_cmd->callback([target_str, version_str, explicit_package, explicit_path,
                           q_name, q_version, q_type, q_desc, q_author, q_license, q_id, q_path, q_entry, q_json]() {
        PackageManager pm;

        std::string raw_target = *target_str;
        if (!explicit_package->empty()) {
            raw_target = *explicit_package;
        } else if (!explicit_path->empty()) {
            raw_target = *explicit_path;
        }

        if (raw_target.empty()) {
            std::cerr << "Error: Target project or package bundle is required." << std::endl;
            cli_exit(1);
        }

        std::filesystem::path tp(raw_target);
        bool is_bundle = (tp.extension() == ".slxpac" || (std::filesystem::is_regular_file(tp) && solix::is_zip_archive(tp)));

        std::string proj_name;
        std::string proj_version;
        std::string proj_type;
        std::string proj_id;
        std::string proj_installed_at;
        std::string proj_path;
        std::string proj_original_path;
        std::string proj_description;
        std::string proj_author;
        std::string proj_license;
        std::string proj_entry;
        uintmax_t disk_size = 0;
        std::optional<nlohmann::json> manifest_json;

        if (is_bundle) {
            auto mf_opt = PackageManager::load_manifest_from_bundle(tp);
            if (!mf_opt) {
                std::cerr << "Error: Failed to read manifest from package bundle: " << tp.string() << std::endl;
                cli_exit(1);
            }
            manifest_json = mf_opt;
            const auto& m = *mf_opt;
            proj_name = m.value("project", m.value("name", tp.stem().string()));
            proj_version = m.value("version", "1.0.0");
            proj_type = m.value("type", "app");
            proj_id = PackageManager::compute_project_id(proj_name, proj_version);
            proj_installed_at = "bundle";
            proj_path = std::filesystem::absolute(tp).lexically_normal().string();
            proj_original_path = proj_path;
            proj_description = m.value("description", "");
            proj_author = m.value("author", "");
            proj_license = m.value("license", "");
            proj_entry = m.value("entry", "bytecode.slxbin");

            std::error_code ec;
            disk_size = std::filesystem::file_size(tp, ec);
        } else {
            std::string req_name = raw_target;
            std::string req_ver = *version_str;
            auto at_pos = req_name.find('@');
            if (at_pos != std::string::npos) {
                if (req_ver.empty()) {
                    req_ver = req_name.substr(at_pos + 1);
                }
                req_name = req_name.substr(0, at_pos);
            }

            std::optional<InstalledProject> target;
            if (!req_ver.empty()) {
                target = pm.get_project_by_name_and_version(req_name, req_ver);
                if (!target) {
                    std::cerr << "Error: No installed project found for '" << req_name 
                              << "' version '" << req_ver << "'." << std::endl;
                    cli_exit(1);
                }
            } else {
                auto matches = pm.get_projects_by_name(req_name);
                if (matches.empty()) {
                    std::cerr << "Error: No installed project found with name '" << req_name << "'." << std::endl;
                    cli_exit(1);
                }
                if (matches.size() > 1) {
                    std::cout << "Multiple versions installed for '" << req_name << "'. Please specify a version:\n";
                    for (const auto& m : matches) {
                        std::cout << "  - " << m.version << " (ID: " << m.id << ")\n";
                    }
                    return;
                }
                target = matches[0];
            }

            const auto& p = *target;
            proj_name = p.name;
            proj_version = p.version;
            proj_type = p.type;
            proj_id = p.id;
            proj_installed_at = p.installed_at;
            proj_path = p.path.string();
            proj_original_path = p.original_path.string();
            proj_description = p.description;
            disk_size = PackageManager::calculate_directory_size(p.path);

            manifest_json = PackageManager::load_installed_manifest(p);
            if (manifest_json) {
                const auto& m = *manifest_json;
                if (proj_description.empty()) proj_description = m.value("description", "");
                proj_author = m.value("author", "");
                proj_license = m.value("license", "");
                proj_entry = m.value("entry", (p.type == "app" ? "bytecode.slxbin" : ""));
            }
        }

        // Scripting queries
        bool has_query = *q_name || *q_version || *q_type || *q_desc || *q_author || *q_license || *q_id || *q_path || *q_entry || *q_json;
        if (has_query) {
            if (*q_json) {
                nlohmann::json j;
                j["id"] = proj_id;
                j["name"] = proj_name;
                j["version"] = proj_version;
                j["type"] = proj_type;
                j["path"] = proj_path;
                j["original_path"] = proj_original_path;
                j["entry"] = proj_entry;
                j["description"] = proj_description;
                j["author"] = proj_author;
                j["license"] = proj_license;
                j["size_bytes"] = disk_size;
                std::cout << j.dump(2) << std::endl;
                return;
            }
            if (*q_name) std::cout << proj_name << "\n";
            if (*q_version) std::cout << proj_version << "\n";
            if (*q_type) std::cout << proj_type << "\n";
            if (*q_desc) std::cout << proj_description << "\n";
            if (*q_author) std::cout << proj_author << "\n";
            if (*q_license) std::cout << proj_license << "\n";
            if (*q_id) std::cout << proj_id << "\n";
            if (*q_path) std::cout << proj_path << "\n";
            if (*q_entry) std::cout << proj_entry << "\n";
            return;
        }

        // Standard summary table
        std::cout << "=================================================================\n";
        std::cout << "Project:      " << proj_name << " v" << proj_version << "\n";
        std::cout << "Type:         " << proj_type << "\n";
        std::cout << "ID:           " << proj_id << "\n";
        std::cout << "Installed At: " << proj_installed_at << "\n";
        std::cout << "Path:         " << proj_path << "\n";
        std::cout << "Original:     " << proj_original_path << "\n";
        std::cout << "Disk Size:    " << format_size(disk_size) << "\n";

        if (!proj_description.empty()) {
            std::cout << "Description:  " << proj_description << "\n";
        }
        if (!proj_author.empty()) {
            std::cout << "Author:       " << proj_author << "\n";
        }
        if (!proj_license.empty()) {
            std::cout << "License:      " << proj_license << "\n";
        }
        if (!proj_entry.empty()) {
            std::cout << "Entry Point:  " << proj_entry << "\n";
        }

        if (manifest_json) {
            const auto& m = *manifest_json;
            if (m.contains("tags") && m["tags"].is_array() && !m["tags"].empty()) {
                std::cout << "Tags:         ";
                for (size_t i = 0; i < m["tags"].size(); ++i) {
                    std::cout << m["tags"][i].get<std::string>() << (i + 1 < m["tags"].size() ? ", " : "\n");
                }
            }

            if (m.contains("dependencies") && m["dependencies"].is_array() && !m["dependencies"].empty()) {
                std::cout << "\nDependencies (" << m["dependencies"].size() << "):\n";
                for (const auto& dep : m["dependencies"]) {
                    std::cout << "  - Type: " << dep.value("type", "unknown") 
                              << ", Name: " << dep.value("name", dep.value("project", ""))
                              << ", Version: " << dep.value("version", "") << "\n";
                }
            }

            if (m.contains("profiles") && m["profiles"].is_object()) {
                std::cout << "\nDefined Profiles:\n";
                for (const auto& [pname, prof] : m["profiles"].items()) {
                    std::cout << "  [" << pname << "]\n";
                    std::cout << "    Output Directory: " << prof.value("output_directory", "") << "\n";
                    std::cout << "    Exe Filename:     " << prof.value("exe_filename", "") << "\n";
                    if (prof.contains("compilation") && prof["compilation"].is_object()) {
                        std::cout << "    Entry Point:      " << prof["compilation"].value("entry_point", "main") << "\n";
                        std::cout << "    Multithreaded:    " << (prof["compilation"].value("multithreaded", false) ? "true" : "false") << "\n";
                    }
                }
            }
        }
        std::cout << "=================================================================\n";
    });
}

} // namespace solix::cli
