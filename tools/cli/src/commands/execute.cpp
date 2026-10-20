#include "execute.hpp"
#include "build.hpp"
#include "../cli_utils.hpp"
#include "../package_manager.hpp"
#include "solix/compilation.hpp"
#include "solix/runtime.hpp"
#include "solix/zip_utils.hpp"
#include "solix/shared_library.hpp"
#include "solix/path_utils.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace solix::cli {

namespace {

inline void scan_directory_for_native_libs(const std::filesystem::path &dir, std::vector<std::filesystem::path> &out) {
  std::error_code ec;
  if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec)) {
    return;
  }
  for (const auto &entry : std::filesystem::directory_iterator(dir, ec)) {
    if (entry.is_regular_file(ec) && solix::is_platform_native_library(entry.path())) {
      out.push_back(entry.path().lexically_normal());
    }
  }
}

inline std::filesystem::path resolve_and_unpack_bundle(const std::filesystem::path& pac_path) {
  std::error_code ec;
  uintmax_t fsize = std::filesystem::file_size(pac_path, ec);
  auto mtime = std::filesystem::last_write_time(pac_path, ec).time_since_epoch().count();

  std::string pac_stem = pac_path.stem().string();
  uint64_t hash = 14695981039346656037ULL;
  for (char c : pac_path.filename().string()) {
    hash = (hash ^ static_cast<uint8_t>(c)) * 1099511628211ULL;
  }
  hash = (hash ^ fsize) * 1099511628211ULL;
  hash = (hash ^ static_cast<uint64_t>(mtime)) * 1099511628211ULL;

  std::stringstream ss;
  ss << std::hex << hash;
  std::string hash_str = ss.str();

  std::filesystem::path cache_dir = std::filesystem::temp_directory_path() / "solix_pac" / (pac_stem + "_" + hash_str);

  // If cache directory exists and contains bytecode.slxbin, reuse it immediately
  if (std::filesystem::exists(cache_dir / "bytecode.slxbin", ec)) {
    return cache_dir;
  }

  std::filesystem::create_directories(cache_dir, ec);

  ZipArchiveReader reader(pac_path);
  if (!reader.is_valid()) {
    throw std::runtime_error("Invalid or corrupted Solix package: " + pac_path.string() + " (" + reader.get_error() + ")");
  }

  if (!reader.extract_all(cache_dir)) {
    throw std::runtime_error("Failed to unpack Solix package: " + reader.get_error());
  }

  return cache_dir;
}

} // namespace

void setup_execute_command(CLI::App &app) {
  auto *execute_cmd = app.add_subcommand("run", "Run compiled bytecode (.slxbin), Solix project, or package bundle (.slxpac)");

  auto opts = std::make_shared<RuntimeOptions>();
  auto input_target = std::make_shared<std::string>();
  auto profile_str = std::make_shared<std::string>("debug");
  auto package_name = std::make_shared<std::string>();
  auto version_str = std::make_shared<std::string>();
  auto project_path_str = std::make_shared<std::string>();
  auto native_libs_cli = std::make_shared<std::vector<std::string>>();

  execute_cmd->add_option("target", *input_target, "Solix bytecode file (.slxbin), package bundle (.slxpac), project directory, or package@version");

  auto *profile_opt = execute_cmd->add_option("-P,--profile", *profile_str, "Build profile to run (default: debug)");
  execute_cmd->add_option("-n,--package", *package_name, "Installed package name in $SOLIX_HOME");
  execute_cmd->add_option("-v,--version", *version_str, "Installed package version");
  execute_cmd->add_option("-i,--input,--path,--project", *project_path_str, "Path to project directory, solix.json, or package bundle");
  execute_cmd->add_option("-L,--native-lib", *native_libs_cli, "Path to native shared library (.dll, .so, .dylib)");

  auto *stack_opt = execute_cmd->add_option("-s,--stack", opts->stack_capacity,
                                            "Stack capacity in words (default: 1048576)");

  auto *heap_opt = execute_cmd->add_option("-p,--heap", opts->heap_capacity,
                                           "Heap capacity in words (default: 16777216)");

  execute_cmd->add_option("args", opts->program_args,
                          "Arguments passed to the Solix program");

  execute_cmd->callback([opts, input_target, profile_str, profile_opt, package_name, version_str, project_path_str, native_libs_cli, stack_opt, heap_opt]() {
    std::filesystem::path bytecode_path;
    std::filesystem::path project_dir;
    std::filesystem::path bundle_path;
    bool is_project = false;
    bool is_bundle = false;

    // 1. Resolve Target Mode
    if (!package_name->empty()) {
      std::string pkg = *package_name;
      std::string ver = *version_str;
      auto at_pos = pkg.find('@');
      if (at_pos != std::string::npos) {
        if (ver.empty()) {
          ver = pkg.substr(at_pos + 1);
        }
        pkg = pkg.substr(0, at_pos);
      }
      if (ver.empty()) {
        std::cerr << "Error: --version is required when specifying --package" << std::endl;
        cli_exit(1);
      }
      PackageManager pkg_mgr;
      auto installed = pkg_mgr.get_project_by_name_and_version(pkg, ver);
      if (!installed) {
        std::cerr << "Error: Installed project '" << pkg << "' with version '" << ver << "' not found" << std::endl;
        cli_exit(1);
      }
      project_dir = installed->path;
      is_project = true;
    } else if (!project_path_str->empty()) {
      std::filesystem::path p(*project_path_str);
      if (p.extension() == ".slxpac" || (std::filesystem::is_regular_file(p) && is_zip_archive(p))) {
        bundle_path = p.lexically_normal();
        is_bundle = true;
      } else {
        project_dir = p;
        is_project = true;
      }
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
      } else if (std::filesystem::is_directory(t) ||
                 std::filesystem::path(t).filename() == "manifest.json" ||
                 std::filesystem::path(t).filename() == "solix.json") {
        project_dir = t;
        is_project = true;
      } else if (std::filesystem::exists(t)) {
        std::filesystem::path tp(t);
        if (tp.extension() == ".slxpac" || is_zip_archive(tp)) {
          bundle_path = tp.lexically_normal();
          is_bundle = true;
        } else {
          bytecode_path = tp.lexically_normal();
          is_project = false;
        }
      } else {
        std::cerr << "Error: File does not exist: " << t << std::endl;
        cli_exit(1);
      }
    } else {
      // Neither positional target nor flags specified
      if (std::filesystem::exists("manifest.json") || std::filesystem::exists("solix.json")) {
        project_dir = ".";
        is_project = true;
      } else {
        std::cerr << "Error: Target file or project is required (file is required)." << std::endl;
        cli_exit(1);
      }
    }

    // 2. Handle Execution Modes
    if (is_bundle) {
      if (profile_opt->count() > 0) {
        std::cerr << "Warning: Option --profile is ignored when running a pre-built package or binary." << std::endl;
      }
      std::filesystem::path unpacked_dir;
      try {
        unpacked_dir = resolve_and_unpack_bundle(bundle_path);
      } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        cli_exit(1);
      }

      // Check bundle manifest for runtime settings and project type
      std::filesystem::path manifest_path = unpacked_dir / "manifest.json";
      if (!std::filesystem::exists(manifest_path)) {
        manifest_path = unpacked_dir / "solix.json";
      }
      if (std::filesystem::exists(manifest_path)) {
        nlohmann::json bm;
        try {
          std::ifstream mf(manifest_path);
          mf >> bm;
        } catch (...) {}

        std::string btype = bm.value("type", "app");
        std::transform(btype.begin(), btype.end(), btype.begin(), [](unsigned char c) { return std::tolower(c); });
        if (btype == "library" || btype == "lib") {
          std::cerr << "Cannot run '" << bundle_path.filename().string() << "' because it is a library (libraries do not contain an executable entry point)." << std::endl;
          cli_exit(1);
        }
        if (btype == "plugin") {
          std::cerr << "Cannot run '" << bundle_path.filename().string() << "' because it is a plugin (plugins do not contain an executable entry point)." << std::endl;
          cli_exit(1);
        }

        if (bm.contains("runtime") && bm["runtime"].is_object()) {
          const auto& rt = bm["runtime"];
          if (stack_opt->count() == 0 && rt.contains("stack_size") && !rt["stack_size"].is_null()) {
            opts->stack_capacity = rt["stack_size"].get<uint64_t>();
          }
          if (heap_opt->count() == 0 && rt.contains("heap_size") && !rt["heap_size"].is_null()) {
            opts->heap_capacity = rt["heap_size"].get<uint64_t>();
          }
        }
      }

      bytecode_path = unpacked_dir / "bytecode.slxbin";
      if (!std::filesystem::exists(bytecode_path)) {
        std::cerr << "Error: Bundle missing bytecode.slxbin entry point: " << bundle_path.string() << std::endl;
        cli_exit(1);
      }

      // Auto-discover in bundle lib/
      scan_directory_for_native_libs(unpacked_dir / "lib", opts->native_libraries);

      // Set resource directory to bundle resources/
      opts->resource_directory = (unpacked_dir / "resources").lexically_normal();

    } else if (is_project) {
      std::filesystem::path prebuilt_bin = project_dir / "bytecode.slxbin";
      std::filesystem::path mf_p = solix::find_project_manifest(project_dir);
      bool is_prebuilt_app = false;
      nlohmann::json app_mf;
      if (std::filesystem::exists(mf_p)) {
        try {
          std::ifstream f(mf_p);
          f >> app_mf;
        } catch (...) {}

        std::string p_type = app_mf.value("type", "app");
        std::transform(p_type.begin(), p_type.end(), p_type.begin(), [](unsigned char c) { return std::tolower(c); });
        if (p_type == "library" || p_type == "lib") {
          std::string name = app_mf.value("project", project_dir.filename().string());
          std::cerr << "Cannot run '" << name << "' because it is a library (libraries do not contain an executable entry point)." << std::endl;
          cli_exit(1);
        }
        if (p_type == "plugin") {
          std::string name = app_mf.value("project", project_dir.filename().string());
          std::cerr << "Cannot run '" << name << "' because it is a plugin (plugins do not contain an executable entry point)." << std::endl;
          cli_exit(1);
        }
        if (app_mf.value("installed", false) && p_type == "app") {
          is_prebuilt_app = true;
        }
      }
      if (!is_prebuilt_app && std::filesystem::exists(prebuilt_bin)) {
        is_prebuilt_app = true;
      }

      if (is_prebuilt_app && std::filesystem::exists(prebuilt_bin)) {
        if (profile_opt->count() > 0) {
          std::cerr << "Warning: Option --profile is ignored when running a pre-built package or binary." << std::endl;
        }
        bytecode_path = prebuilt_bin.lexically_normal();
        scan_directory_for_native_libs(project_dir / "lib", opts->native_libraries);
        std::error_code ec;
        if (std::filesystem::exists(project_dir / "resources", ec)) {
          opts->resource_directory = (project_dir / "resources").lexically_normal();
        }
      } else {
        auto build_res = build_project(project_dir, *profile_str);
        if (!build_res.success) {
          cli_exit(1);
        }
        bytecode_path = build_res.output_binary;

        // Ingest root native_libraries from manifest
        if (build_res.manifest.contains("native_libraries") && build_res.manifest["native_libraries"].is_array()) {
          for (const auto& item : build_res.manifest["native_libraries"]) {
            if (item.is_string()) {
              std::filesystem::path p = item.get<std::string>();
              if (p.is_relative()) p = build_res.project_root / p;
              opts->native_libraries.push_back(p.lexically_normal());
            }
          }
        }

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
          if (rt.contains("native_libraries") && rt["native_libraries"].is_array()) {
            for (const auto& item : rt["native_libraries"]) {
              if (item.is_string()) {
                std::filesystem::path p = item.get<std::string>();
                if (p.is_relative()) p = build_res.project_root / p;
                opts->native_libraries.push_back(p.lexically_normal());
              }
            }
          }
        }

        // Auto-discover in project lib/, dependency projects lib/, and output directory
        scan_directory_for_native_libs(build_res.project_root / "lib", opts->native_libraries);
        for (const auto& dep_root : build_res.dependency_roots) {
          scan_directory_for_native_libs(dep_root / "lib", opts->native_libraries);
        }
        scan_directory_for_native_libs(bytecode_path.parent_path(), opts->native_libraries);

        // Set resource directory to project resources/
        opts->resource_directory = (build_res.project_root / "resources").lexically_normal();
      }
    } else {
      if (profile_opt->count() > 0) {
        std::cerr << "Warning: Option --profile is ignored when running a pre-built package or binary." << std::endl;
      }

      // Standalone bytecode file mode: scan parent directory and parent/lib
      scan_directory_for_native_libs(bytecode_path.parent_path(), opts->native_libraries);
      scan_directory_for_native_libs(bytecode_path.parent_path() / "lib", opts->native_libraries);

      opts->resource_directory = (bytecode_path.parent_path() / "resources").lexically_normal();
    }

    // Ingest CLI flags: -L, --native-lib
    for (const auto& lib_str : *native_libs_cli) {
      std::filesystem::path p(lib_str);
      if (p.is_relative()) p = std::filesystem::current_path() / p;
      opts->native_libraries.push_back(p.lexically_normal());
    }

    // De-duplicate native libraries list
    {
      std::vector<std::filesystem::path> unique_libs;
      for (const auto& p : opts->native_libraries) {
        if (std::find(unique_libs.begin(), unique_libs.end(), p) == unique_libs.end()) {
          unique_libs.push_back(p);
        }
      }
      opts->native_libraries = std::move(unique_libs);
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

    // Export SOLIX_RESOURCE_ROOT for ambient native library / process inspection
    if (!opts->resource_directory.empty()) {
#if defined(_WIN32)
      SetEnvironmentVariableA("SOLIX_RESOURCE_ROOT", opts->resource_directory.string().c_str());
      _putenv_s("SOLIX_RESOURCE_ROOT", opts->resource_directory.string().c_str());
#else
      setenv("SOLIX_RESOURCE_ROOT", opts->resource_directory.string().c_str(), 1);
#endif
    }

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
