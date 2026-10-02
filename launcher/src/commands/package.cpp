#include "package.hpp"
#include "../cli_utils.hpp"
#include "../package_manager.hpp"
#include <iostream>
#include <iomanip>
#include <memory>

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
    auto *install_cmd = app.add_subcommand("install", "Install a Solix project locally");
    auto install_path = std::make_shared<std::string>(".");
    auto force_flag = std::make_shared<bool>(false);

    install_cmd->add_option("path", *install_path, "Path to the project directory (default: current directory)");
    install_cmd->add_flag("-f,--force", *force_flag, "Reinstall / overwrite if already installed");

    install_cmd->callback([install_path, force_flag]() {
        PackageManager pm;
        std::string id;
        if (!pm.install_project(std::filesystem::path(*install_path), *force_flag, id)) {
            cli_exit(1);
        }
        auto installed_dir = PackageManager::get_installed_dir() / id;
        std::cout << "Successfully installed project (ID: " << id << ")" << std::endl;
        std::cout << "  Location: " << installed_dir.string() << std::endl;
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
    auto *list_cmd = app.add_subcommand("list", "List all installed Solix projects");

    list_cmd->callback([]() {
        PackageManager pm;
        auto projects = pm.list_installed_projects();
        if (projects.empty()) {
            std::cout << "No Solix projects are currently installed." << std::endl;
            return;
        }

        std::cout << "Installed Solix Projects (" << projects.size() << "):\n\n";
        std::cout << std::left 
                  << std::setw(24) << "NAME" 
                  << std::setw(12) << "VERSION" 
                  << std::setw(18) << "ID" 
                  << "INSTALLED PATH\n";
        std::cout << std::string(80, '-') << "\n";

        for (const auto& p : projects) {
            std::cout << std::left 
                      << std::setw(24) << p.name
                      << std::setw(12) << p.version
                      << std::setw(18) << p.id
                      << p.path.string() << "\n";
        }
    });

    // 4. details
    auto *details_cmd = app.add_subcommand("details", "Display extensive details about an installed project");
    auto details_name = std::make_shared<std::string>();
    auto details_version = std::make_shared<std::string>();

    details_cmd->add_option("name", *details_name, "Name of the installed project")->required();
    details_cmd->add_option("version", *details_version, "Version of the project (optional if unique)");

    details_cmd->callback([details_name, details_version]() {
        PackageManager pm;
        std::optional<InstalledProject> target;

        if (!details_version->empty()) {
            target = pm.get_project_by_name_and_version(*details_name, *details_version);
            if (!target) {
                std::cerr << "Error: No installed project found for '" << *details_name 
                          << "' version '" << *details_version << "'." << std::endl;
                cli_exit(1);
            }
        } else {
            auto matches = pm.get_projects_by_name(*details_name);
            if (matches.empty()) {
                std::cerr << "Error: No installed project found with name '" << *details_name << "'." << std::endl;
                cli_exit(1);
            }
            if (matches.size() > 1) {
                std::cout << "Multiple versions installed for '" << *details_name << "'. Please specify a version:\n";
                for (const auto& m : matches) {
                    std::cout << "  - " << m.version << " (ID: " << m.id << ")\n";
                }
                return;
            }
            target = matches[0];
        }

        const auto& p = *target;
        uintmax_t size_bytes = PackageManager::calculate_directory_size(p.path);

        std::cout << "=================================================================\n";
        std::cout << "Project:      " << p.name << " v" << p.version << "\n";
        std::cout << "ID:           " << p.id << "\n";
        std::cout << "Installed At: " << p.installed_at << "\n";
        std::cout << "Path:         " << p.path.string() << "\n";
        std::cout << "Original:     " << p.original_path.string() << "\n";
        std::cout << "Disk Size:    " << format_size(size_bytes) << "\n";

        auto manifest_opt = PackageManager::load_installed_manifest(p);
        if (manifest_opt) {
            const auto& m = *manifest_opt;
            if (m.contains("description") && !m["description"].is_null()) {
                std::cout << "Description:  " << m["description"].get<std::string>() << "\n";
            }
            if (m.contains("author") && !m["author"].is_null()) {
                std::cout << "Author:       " << m["author"].get<std::string>() << "\n";
            }
            if (m.contains("license") && !m["license"].is_null()) {
                std::cout << "License:      " << m["license"].get<std::string>() << "\n";
            }
            if (m.contains("tags") && m["tags"].is_array() && !m["tags"].empty()) {
                std::cout << "Tags:         ";
                for (size_t i = 0; i < m["tags"].size(); ++i) {
                    std::cout << m["tags"][i].get<std::string>() << (i + 1 < m["tags"].size() ? ", " : "\n");
                }
            }

            if (m.contains("dependencies") && m["dependencies"].is_array()) {
                std::cout << "\nDependencies (" << m["dependencies"].size() << "):\n";
                for (const auto& dep : m["dependencies"]) {
                    std::cout << "  - Type: " << dep.value("type", "unknown") 
                              << ", Path: " << dep.value("path", "") << "\n";
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
