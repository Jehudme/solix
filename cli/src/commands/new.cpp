#include "new.hpp"
#include "../cli_utils.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace solix::cli {

void setup_new_command(CLI::App &app) {
    auto *new_cmd = app.add_subcommand("new", "Create a new Solix project");

    auto path_str = std::make_shared<std::string>();
    auto name_str = std::make_shared<std::string>();
    auto version_str = std::make_shared<std::string>("0.1.0");
    auto author_str = std::make_shared<std::string>();
    auto description_str = std::make_shared<std::string>();
    auto license_str = std::make_shared<std::string>();
    auto tags = std::make_shared<std::vector<std::string>>();
    auto entry_str = std::make_shared<std::string>("main");
    auto template_str = std::make_shared<std::string>();
    auto force_flag = std::make_shared<bool>(false);

    new_cmd->add_option("path", *path_str, "Destination path for the new project")->required();
    new_cmd->add_option("-n,--name", *name_str, "Project name (default: derived from path)");
    new_cmd->add_option("-v,--version", *version_str, "Initial semantic version (default: 0.1.0)");
    new_cmd->add_option("-a,--author", *author_str, "Author name or email");
    new_cmd->add_option("-d,--description", *description_str, "Project description");
    new_cmd->add_option("-l,--license", *license_str, "License identifier (e.g. MIT, Apache-2.0)");
    new_cmd->add_option("-t,--tag", *tags, "Project tags/keywords (can be specified multiple times)");
    new_cmd->add_option("-e,--entry", *entry_str, "Default entry point method name (default: main)");
    new_cmd->add_option("--template", *template_str, "Path to a custom template folder on disk");
    new_cmd->add_flag("-f,--force", *force_flag, "Overwrite destination if directory is not empty");

    new_cmd->callback([path_str, name_str, version_str, author_str, description_str,
                       license_str, tags, entry_str, template_str, force_flag]() {
        std::filesystem::path target_dir(*path_str);
        target_dir = std::filesystem::absolute(target_dir).lexically_normal();

        // 1. Collision & existence check
        if (std::filesystem::exists(target_dir)) {
            if (!std::filesystem::is_directory(target_dir)) {
                std::cerr << "Error: Destination path exists and is not a directory: " 
                          << target_dir.string() << std::endl;
                cli_exit(1);
            }
            if (!std::filesystem::is_empty(target_dir) && !(*force_flag)) {
                std::cerr << "Error: Destination directory '" << target_dir.string() 
                          << "' already exists and is not empty. Use --force to proceed anyway." << std::endl;
                cli_exit(1);
            }
        }

        // Determine project name
        std::string project_name = *name_str;
        if (project_name.empty()) {
            project_name = target_dir.filename().string();
            if (project_name.empty() || project_name == ".") {
                project_name = "unnamed_project";
            }
        }

        // 2. Hybrid Scaffolding: On-disk template copy (if requested)
        if (!template_str->empty()) {
            std::filesystem::path template_path(*template_str);
            if (!std::filesystem::exists(template_path) || !std::filesystem::is_directory(template_path)) {
                std::cerr << "Error: Template directory does not exist: " << template_path.string() << std::endl;
                cli_exit(1);
            }

            try {
                std::filesystem::create_directories(target_dir);
                std::filesystem::copy(template_path, target_dir, 
                                     std::filesystem::copy_options::recursive | 
                                     std::filesystem::copy_options::overwrite_existing);

                // Update solix.json if present in the copied template
                std::filesystem::path manifest_path = target_dir / "solix.json";
                if (std::filesystem::exists(manifest_path)) {
                    std::ifstream in_file(manifest_path);
                    nlohmann::json manifest;
                    in_file >> manifest;
                    in_file.close();

                    manifest["project"] = project_name;
                    manifest["version"] = *version_str;
                    if (!author_str->empty()) manifest["author"] = *author_str;
                    if (!description_str->empty()) manifest["description"] = *description_str;
                    if (!license_str->empty()) manifest["license"] = *license_str;
                    if (!tags->empty()) manifest["tags"] = *tags;

                    if (manifest.contains("profiles") && manifest["profiles"].is_object()) {
                        for (auto& [_, prof] : manifest["profiles"].items()) {
                            if (prof.contains("compilation") && prof["compilation"].is_object()) {
                                prof["compilation"]["entry_point"] = *entry_str;
                            }
                        }
                    }

                    std::ofstream out_file(manifest_path);
                    out_file << manifest.dump(2) << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: Failed to copy template: " << e.what() << std::endl;
                cli_exit(1);
            }
        } else {
            // 3. Default In-Memory Scaffolding (100% self-contained)
            try {
                std::filesystem::create_directories(target_dir / "src");

                // Generate src/main.slx
                std::filesystem::path main_file = target_dir / "src" / "main.slx";
                std::ofstream slx_out(main_file);
                if (!slx_out) {
                    std::cerr << "Error: Could not create file " << main_file.string() << std::endl;
                    cli_exit(1);
                }
                slx_out << "int32 " << *entry_str << "() {\n";
                slx_out << "    return 0;\n";
                slx_out << "}\n";
                slx_out.close();

                // Generate solix.json
                nlohmann::json manifest;
                manifest["project"] = project_name;
                manifest["version"] = *version_str;
                manifest["description"] = description_str->empty() ? nullptr : nlohmann::json(*description_str);
                manifest["tags"] = *tags;
                manifest["author"] = author_str->empty() ? nullptr : nlohmann::json(*author_str);
                manifest["license"] = license_str->empty() ? nullptr : nlohmann::json(*license_str);

                manifest["dependencies"] = nlohmann::json::array({
                    {
                        {"type", "source"},
                        {"path", "src/main.slx"}
                    }
                });

                auto make_profile = [&](const std::string& profile_name, const std::string& log_level,
                                        const std::string& flush_level, const std::string& log_file,
                                        bool multithreaded) {
                    nlohmann::json p;
                    p["output_directory"] = "build/" + profile_name;
                    p["relative_paths"] = true;
                    p["exe_filename"] = "out.slxbin";
                    p["asm_filename"] = "out.slxasm";

                    p["compilation"] = {
                        {"entry_point", *entry_str},
                        {"multithreaded", multithreaded},
                        {"additional_dependencies", nlohmann::json::array()},
                        {"logs", {
                            {"level", log_level},
                            {"flush_level", flush_level},
                            {"filename", log_file},
                            {"sink", "STDOUT"},
                            {"pattern", "[%^%-8l%$] [%-12n] %v"}
                        }}
                    };

                    p["runtime"] = {
                        {"heap_size", nullptr},
                        {"stack_size", nullptr},
                        {"arguments", nlohmann::json::array()}
                    };

                    return p;
                };

                manifest["profiles"]["release"] = make_profile("release", "warning", "warning", "logs/release.log", true);
                manifest["profiles"]["debug"] = make_profile("debug", "debug", "debug", "logs/debug.log", false);
                manifest["profiles"]["test"] = make_profile("test", "debug", "debug", "logs/test.log", false);

                std::filesystem::path manifest_path = target_dir / "solix.json";
                std::ofstream manifest_out(manifest_path);
                if (!manifest_out) {
                    std::cerr << "Error: Could not create file " << manifest_path.string() << std::endl;
                    cli_exit(1);
                }
                manifest_out << manifest.dump(2) << std::endl;
                manifest_out.close();

            } catch (const std::exception& e) {
                std::cerr << "Error: Failed to create project: " << e.what() << std::endl;
                cli_exit(1);
            }
        }

        std::cout << "Created Solix project '" << project_name << "' at " << target_dir.string() << std::endl;
        std::cout << "To build and run:\n";
        std::cout << "  cd " << target_dir.string() << "\n";
        std::cout << "  solix build\n";
        std::cout << "  solix run build/debug/out.slxbin\n";
    });
}

} // namespace solix::cli
