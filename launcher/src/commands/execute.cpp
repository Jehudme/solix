#include "execute.hpp"
#include "build.hpp"
#include "../cli_utils.hpp"
#include "../package_manager.hpp"
#include "solix/compilation.hpp"
#include "solix/runtime.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

namespace solix::cli {

void setup_execute_command(CLI::App &app) {
  auto *execute_cmd = app.add_subcommand("run", "Run compiled bytecode or Solix project");

  auto opts = std::make_shared<RuntimeOptions>();
  auto input_target = std::make_shared<std::string>();
  auto profile_str = std::make_shared<std::string>("debug");
  auto package_name = std::make_shared<std::string>();
  auto version_str = std::make_shared<std::string>();
  auto project_path_str = std::make_shared<std::string>();

  execute_cmd->add_option("target", *input_target, "Solix bytecode file (.slxbin), project directory, or package@version");

  execute_cmd->add_option("-P,--profile", *profile_str, "Build profile to run (default: debug)");
  execute_cmd->add_option("-n,--package", *package_name, "Installed package name in $SOLIX_HOME");
  execute_cmd->add_option("-v,--version", *version_str, "Installed package version");
  execute_cmd->add_option("--project", *project_path_str, "Path to project directory or solix.json");

  auto *stack_opt = execute_cmd->add_option("-s,--stack", opts->stack_capacity,
                                            "Stack capacity in words (default: 1048576)");

  auto *heap_opt = execute_cmd->add_option("-p,--heap", opts->heap_capacity,
                                           "Heap capacity in words (default: 16777216)");

  execute_cmd->add_option("args", opts->program_args,
                          "Arguments passed to the Solix program");

  execute_cmd->callback([opts, input_target, profile_str, package_name, version_str, project_path_str, stack_opt, heap_opt]() {
    std::filesystem::path bytecode_path;
    std::filesystem::path project_dir;
    bool is_project = false;

    // 1. Resolve Target Mode
    if (!package_name->empty()) {
      if (version_str->empty()) {
        std::cerr << "Error: --version is required when specifying --package" << std::endl;
        cli_exit(1);
      }
      PackageManager pkg_mgr;
      auto installed = pkg_mgr.get_project_by_name_and_version(*package_name, *version_str);
      if (!installed) {
        std::cerr << "Error: Installed project '" << *package_name << "' with version '" << *version_str << "' not found" << std::endl;
        cli_exit(1);
      }
      project_dir = installed->path;
      is_project = true;
    } else if (!project_path_str->empty()) {
      project_dir = *project_path_str;
      is_project = true;
    } else if (!input_target->empty()) {
      std::string t = *input_target;
      auto at_pos = t.find('@');
      if (at_pos != std::string::npos && !std::filesystem::exists(t)) {
        std::string pkg = t.substr(0, at_pos);
        std::string ver = t.substr(at_pos + 1);
        PackageManager pkg_mgr;
        auto installed = pkg_mgr.get_project_by_name_and_version(pkg, ver);
        if (!installed) {
          std::cerr << "Error: Installed project '" << pkg << "' with version '" << ver << "' not found" << std::endl;
          cli_exit(1);
        }
        project_dir = installed->path;
        is_project = true;
      } else if (std::filesystem::is_directory(t) || std::filesystem::path(t).filename() == "solix.json") {
        project_dir = t;
        is_project = true;
      } else if (std::filesystem::exists(t)) {
        bytecode_path = t;
        is_project = false;
      } else {
        std::cerr << "Error: File does not exist: " << t << std::endl;
        cli_exit(1);
      }
    } else {
      // Neither positional target nor flags specified
      if (std::filesystem::exists("solix.json")) {
        project_dir = ".";
        is_project = true;
      } else {
        std::cerr << "Error: Target file or project is required (file is required)." << std::endl;
        cli_exit(1);
      }
    }

    // 2. Handle Project Mode
    if (is_project) {
      auto build_res = build_project(project_dir, *profile_str);
      if (!build_res.success) {
        cli_exit(1);
      }
      bytecode_path = build_res.output_binary;

      // Ingest runtime settings from manifest profile
      if (build_res.manifest.contains("profiles") &&
          build_res.manifest["profiles"].contains(*profile_str) &&
          build_res.manifest["profiles"][*profile_str].contains("runtime") &&
          build_res.manifest["profiles"][*profile_str]["runtime"].is_object()) {
        const auto& rt = build_res.manifest["profiles"][*profile_str]["runtime"];

        if (stack_opt->count() == 0 && rt.contains("stack_size") && !rt["stack_size"].is_null()) {
          opts->stack_capacity = rt["stack_size"].get<uint64_t>();
        }
        if (heap_opt->count() == 0 && rt.contains("heap_size") && !rt["heap_size"].is_null()) {
          opts->heap_capacity = rt["heap_size"].get<uint64_t>();
        }
        if (opts->program_args.empty() && rt.contains("arguments") && rt["arguments"].is_array()) {
          for (const auto& arg : rt["arguments"]) {
            if (arg.is_string()) {
              opts->program_args.push_back(arg.get<std::string>());
            }
          }
        }
      }
    }

    // 3. Validate bytecode file
    if (!std::filesystem::exists(bytecode_path)) {
      std::cerr << "Error: File does not exist: " << bytecode_path.string() << std::endl;
      cli_exit(1);
    }

    if (std::filesystem::file_size(bytecode_path) == 0) {
      std::cerr << "Error: Bytecode file is empty: " << bytecode_path.string() << std::endl;
      cli_exit(1);
    }

    opts->bytecode_source = bytecode_path;

    int32_t exit_code = 0;
    try {
      exit_code = run(*opts);
    } catch (const std::exception &e) {
      std::cerr << "Runtime error: " << e.what() << std::endl;
      cli_exit(1);
    }

    if (exit_code != 0) {
      cli_exit(exit_code);
    }
  });
}
} // namespace solix::cli
