#include "package_manager.hpp"
#include "commands/build.hpp"
#include "dependency_resolver.hpp"
#include "solix/path_utils.hpp"
#include "solix/shared_library.hpp"
#include "solix/zip_utils.hpp"
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <set>
#include <algorithm>

namespace solix::cli {

namespace {

inline bool is_shared_library(const std::filesystem::path& p) {
    return solix::is_any_shared_library(p);
}

void collect_native_libs(const std::filesystem::path& dir,
                         std::vector<std::filesystem::path>& out,
                         std::set<std::string>& seen) {
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec)) return;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (entry.is_regular_file(ec) && is_shared_library(entry.path())) {
            std::string filename = entry.path().filename().string();
            if (seen.find(filename) == seen.end()) {
                seen.insert(filename);
                out.push_back(entry.path().lexically_normal());
            }
        }
    }
}

void copy_dir_recursive(const std::filesystem::path& from, const std::filesystem::path& to, bool exclude_main_file = false) {
    std::error_code ec;
    if (!std::filesystem::exists(from, ec)) return;
    std::filesystem::create_directories(to, ec);
    for (const auto& entry : std::filesystem::recursive_directory_iterator(from, ec)) {
        if (exclude_main_file && entry.path().filename() == "main.slx") {
            continue;
        }
        auto rel = std::filesystem::relative(entry.path(), from, ec);
        auto target = to / rel;
        if (entry.is_directory(ec)) {
            std::filesystem::create_directories(target, ec);
        } else if (entry.is_regular_file(ec)) {
            if (target.has_parent_path()) {
                std::filesystem::create_directories(target.parent_path(), ec);
            }
            std::filesystem::copy_file(entry.path(), target, std::filesystem::copy_options::overwrite_existing, ec);
        }
    }
}

} // namespace

PackageManager::PackageManager() {
    load_registry();
}

std::filesystem::path PackageManager::get_solix_home() {
    const char* custom_home = std::getenv("SOLIX_HOME");
    if (custom_home && *custom_home != '\0') {
        return std::filesystem::path(custom_home);
    }

#if defined(_WIN32)
    const char* appdata = std::getenv("LOCALAPPDATA");
    if (appdata && *appdata != '\0') {
        return std::filesystem::path(appdata) / "solix";
    }
    const char* userprofile = std::getenv("USERPROFILE");
    if (userprofile && *userprofile != '\0') {
        return std::filesystem::path(userprofile) / ".solix";
    }
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    if (home && *home != '\0') {
        return std::filesystem::path(home) / "Library" / "Application Support" / "solix";
    }
#else
    const char* xdg_data = std::getenv("XDG_DATA_HOME");
    if (xdg_data && *xdg_data != '\0') {
        return std::filesystem::path(xdg_data) / "solix";
    }
    const char* home = std::getenv("HOME");
    if (home && *home != '\0') {
        return std::filesystem::path(home) / ".solix";
    }
#endif

    return std::filesystem::current_path() / ".solix";
}

std::filesystem::path PackageManager::get_installed_dir() {
    return get_solix_home() / "installed";
}

std::filesystem::path PackageManager::get_registry_path() {
    return get_solix_home() / "installed.json";
}

std::string PackageManager::compute_project_id(const std::string& name, const std::string& version) {
    std::string key = name + "@" + version;
    uint64_t hash = 14695981039346656037ULL; // 64-bit FNV offset basis
    for (char c : key) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 1099511628211ULL;            // 64-bit FNV prime
    }
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hash;
    return ss.str();
}

void PackageManager::load_registry() {
    std::filesystem::path reg_path = get_registry_path();
    if (!std::filesystem::exists(reg_path)) {
        registry_ = {
            {"version", 1},
            {"projects", nlohmann::json::object()}
        };
        return;
    }

    try {
        std::ifstream file(reg_path);
        file >> registry_;
        if (!registry_.contains("projects") || !registry_["projects"].is_object()) {
            registry_["projects"] = nlohmann::json::object();
        }
    } catch (...) {
        registry_ = {
            {"version", 1},
            {"projects", nlohmann::json::object()}
        };
    }
}

void PackageManager::save_registry() {
    std::filesystem::path reg_path = get_registry_path();
    std::filesystem::create_directories(reg_path.parent_path());
    std::ofstream file(reg_path);
    if (file) {
        file << registry_.dump(2) << std::endl;
    }
}

bool PackageManager::install_project(const std::filesystem::path& project_path,
                                    const std::string& profile_name,
                                    bool force,
                                    bool as_is,
                                    const std::filesystem::path& custom_output_dir,
                                    std::string& out_id,
                                    const std::string& alias_name) {
    std::filesystem::path orig_source_path = std::filesystem::absolute(project_path).lexically_normal();
    std::filesystem::path input_path = orig_source_path;
    std::filesystem::path temp_bundle_dir;
    bool is_from_bundle = false;

    if (std::filesystem::is_regular_file(input_path) &&
        (input_path.extension() == ".slxpac" || solix::is_zip_archive(input_path))) {
        auto temp_base = std::filesystem::temp_directory_path() / "solix_install_bundle";
        std::error_code tec;
        std::filesystem::create_directories(temp_base, tec);
        auto t_now = std::chrono::steady_clock::now().time_since_epoch().count();
        temp_bundle_dir = temp_base / ("bundle_" + std::to_string(t_now));
        std::filesystem::create_directories(temp_bundle_dir, tec);

        solix::ZipArchiveReader reader(input_path);
        if (!reader.is_valid() || !reader.extract_all(temp_bundle_dir)) {
            std::cerr << "Error: Failed to extract package bundle: " << input_path.string() << std::endl;
            return false;
        }
        input_path = temp_bundle_dir;
        is_from_bundle = true;
        as_is = true;
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
    } temp_guard{temp_bundle_dir, is_from_bundle};

    std::filesystem::path src_dir = input_path;
    if (std::filesystem::is_regular_file(src_dir) &&
        (src_dir.filename() == "manifest.json" || src_dir.filename() == "solix.json")) {
        src_dir = src_dir.parent_path();
    }
    src_dir = src_dir.lexically_normal();

    std::filesystem::path manifest_path = solix::find_project_manifest(src_dir);
    if (!std::filesystem::exists(manifest_path)) {
        std::cerr << "Error: manifest.json not found (solix.json not found) in " << src_dir.string() << std::endl;
        return false;
    }

    nlohmann::json manifest;
    try {
        std::ifstream file(manifest_path);
        file >> manifest;
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to parse " << manifest_path.string() << ": " << e.what() << std::endl;
        return false;
    }

    std::string name = manifest.value("project", "");
    if (name.empty()) {
        name = manifest.value("name", "");
    }
    if (!alias_name.empty()) {
        name = alias_name;
    }
    std::string version = manifest.value("version", "");

    if (name.empty()) {
        std::cerr << "Error: Project manifest must define a 'project' name." << std::endl;
        return false;
    }
    if (version.empty()) {
        std::cerr << "Error: Project manifest must define a 'version'." << std::endl;
        return false;
    }

    bool has_explicit_type = manifest.contains("type") && !manifest["type"].is_null();
    std::string proj_type = manifest.value("type", "app");
    std::transform(proj_type.begin(), proj_type.end(), proj_type.begin(), [](unsigned char c) { return std::tolower(c); });
    if (proj_type == "lib") proj_type = "library";

    std::string id = compute_project_id(name, version);
    out_id = id;

    bool is_custom = !custom_output_dir.empty();
    if (!is_custom && registry_["projects"].contains(id) && !force) {
        std::cerr << "Error: Project '" << name << "' version '" << version 
                  << "' is already installed (ID: " << id << "). Use --force to reinstall." << std::endl;
        return false;
    }

    bool is_already_prepared = as_is || manifest.value("installed", false);
    ProjectBuildResult build_res;

    if (!is_already_prepared) {
        // 1. Inspect dependencies with "path" - check if already installed, warn if so, otherwise install first
        if (!is_custom && manifest.contains("dependencies") && manifest["dependencies"].is_array()) {
            for (const auto& dep : manifest["dependencies"]) {
                if (dep.is_object() && dep.contains("path") && dep["path"].is_string()) {
                    if (dep.value("type", "") == "source") continue;
                    std::filesystem::path dep_path = dep["path"].get<std::string>();
                    if (dep_path.is_relative()) dep_path = src_dir / dep_path;
                    dep_path = dep_path.lexically_normal();
                    auto dep_mf_path = solix::find_project_manifest(dep_path);
                    if (std::filesystem::exists(dep_mf_path)) {
                        try {
                            std::ifstream f(dep_mf_path);
                            nlohmann::json dep_mf;
                            f >> dep_mf;
                            std::string dep_name = dep_mf.value("project", "");
                            if (dep_name.empty()) dep_name = dep_mf.value("name", "");
                            std::string dep_ver = dep_mf.value("version", "0.1.0");
                            if (!dep_name.empty()) {
                                std::string dep_id = compute_project_id(dep_name, dep_ver);
                                if (registry_["projects"].contains(dep_id)) {
                                    std::cout << "[warning] Dependency '" << dep_name << "' version '" << dep_ver 
                                              << "' is already installed (ID: " << dep_id << "), skipping reinstall." << std::endl;
                                } else {
                                    std::string child_id;
                                    if (!install_project(dep_path, profile_name, false, false, {}, child_id)) {
                                        std::cerr << "Error: Failed to install dependency '" << dep_name << "' at " << dep_path.string() << std::endl;
                                        return false;
                                    }
                                    load_registry();
                                }
                            }
                        } catch (...) {}
                    }
                }
            }
        }

        bool has_profiles = manifest.contains("profiles") && manifest["profiles"].is_object() && !manifest["profiles"].empty();
        if (has_profiles) {
            std::string effective_profile = profile_name;
            if (!manifest["profiles"].contains(effective_profile)) {
                if (manifest["profiles"].contains("debug")) {
                    effective_profile = "debug";
                } else {
                    effective_profile = manifest["profiles"].begin().key();
                }
            }

            // 2. Build the project using the requested profile
            build_res = build_project(src_dir, effective_profile);
            if (!build_res.success) {
                std::cerr << "Error: Failed to build project before installation." << std::endl;
                return false;
            }
        } else {
            if (proj_type == "library" || proj_type == "plugin") {
                build_res.success = true;
                DependencyManager dep_mgr;
                std::vector<ProjectCandidate> candidates;
                if (dep_mgr.resolve_graph(manifest, src_dir, nlohmann::json::object(), profile_name, candidates)) {
                    for (const auto& proj : candidates) {
                        if (proj.project_root != src_dir) {
                            build_res.dependency_roots.push_back(proj.project_root);
                        }
                    }
                }
            } else {
                std::cerr << "Error: 'profiles' section missing or invalid in " << manifest_path.filename().string() << std::endl;
                return false;
            }
        }
    }

    // Determine target destination
    std::filesystem::path dest_dir;
    bool should_register = true;
    if (is_custom) {
        dest_dir = std::filesystem::absolute(custom_output_dir).lexically_normal();
        should_register = false;
    } else {
        dest_dir = get_installed_dir() / id;
        should_register = true;
    }

    std::error_code ec;
    if (std::filesystem::exists(dest_dir)) {
        std::filesystem::remove_all(dest_dir, ec);
    }
    std::filesystem::create_directories(dest_dir, ec);

    if (is_already_prepared) {
        std::cout << "Installing pre-packaged distribution as-is..." << std::endl;
        // Already prepared: copy as-is (excluding transient build/git/vscode artifacts)
        for (const auto& entry : std::filesystem::directory_iterator(src_dir, ec)) {
            std::string filename = entry.path().filename().string();
            if (filename == "build" || filename == ".git" || filename == ".vscode") {
                continue;
            }
            std::filesystem::copy(entry.path(), dest_dir / filename,
                                 std::filesystem::copy_options::recursive |
                                 std::filesystem::copy_options::overwrite_existing,
                                 ec);
        }
        if (!alias_name.empty()) {
            std::filesystem::path staged_mf = solix::find_project_manifest(dest_dir);
            if (std::filesystem::exists(staged_mf)) {
                try {
                    std::ifstream f(staged_mf);
                    nlohmann::json mj;
                    f >> mj;
                    f.close();
                    mj["project"] = alias_name;
                    mj["name"] = alias_name;
                    std::ofstream of(staged_mf);
                    of << mj.dump(2) << std::endl;
                } catch (...) {}
            }
        }
    } else if (!has_explicit_type && custom_output_dir.empty()) {
        // Legacy untyped project: preserve full original directory contents (excluding build/.git/.vscode)
        for (const auto& entry : std::filesystem::directory_iterator(src_dir, ec)) {
            std::string filename = entry.path().filename().string();
            if (filename == "build" || filename == ".git" || filename == ".vscode") {
                continue;
            }
            std::filesystem::copy(entry.path(), dest_dir / filename,
                                 std::filesystem::copy_options::recursive |
                                 std::filesystem::copy_options::overwrite_existing,
                                 ec);
        }
        if (std::filesystem::exists(build_res.output_binary)) {
            std::filesystem::copy_file(build_res.output_binary, dest_dir / "bytecode.slxbin", std::filesystem::copy_options::overwrite_existing, ec);
        }
    } else if (proj_type == "library") {
        // Clean library distribution:
        // A. Copy its own src/
        if (std::filesystem::exists(src_dir / "src", ec)) {
            copy_dir_recursive(src_dir / "src", dest_dir / "src", true);
        }
        if (name == "solixlib" && !build_res.output_binary.empty() && std::filesystem::exists(build_res.output_binary)) {
            std::filesystem::copy_file(build_res.output_binary, dest_dir / "bytecode.slxbin", std::filesystem::copy_options::overwrite_existing, ec);
        }

        // B. Stage native shared libraries
        std::vector<std::filesystem::path> native_libs;
        std::set<std::string> seen_libs;
        collect_native_libs(src_dir / "lib", native_libs, seen_libs);
        for (const auto& droot : build_res.dependency_roots) {
            collect_native_libs(droot / "lib", native_libs, seen_libs);
        }
        collect_native_libs(build_res.output_binary.parent_path() / "lib", native_libs, seen_libs);
        if (!native_libs.empty()) {
            std::filesystem::create_directories(dest_dir / "lib", ec);
            for (const auto& lib : native_libs) {
                std::filesystem::copy_file(lib, dest_dir / "lib" / lib.filename(), std::filesystem::copy_options::overwrite_existing, ec);
            }
        }

        // C. Stage resources
        for (const auto& droot : build_res.dependency_roots) {
            copy_dir_recursive(droot / "resources", dest_dir / "resources");
        }
        copy_dir_recursive(src_dir / "resources", dest_dir / "resources");
        copy_dir_recursive(build_res.output_binary.parent_path() / "resources", dest_dir / "resources");

        // D. Reduced manifest.json
        nlohmann::json reduced;
        reduced["project"] = name;
        reduced["name"] = name;
        reduced["version"] = version;
        reduced["type"] = "library";
        reduced["installed"] = true;
        if (manifest.contains("description") && !manifest["description"].is_null()) reduced["description"] = manifest["description"];
        if (manifest.contains("author") && !manifest["author"].is_null()) reduced["author"] = manifest["author"];
        if (manifest.contains("license") && !manifest["license"].is_null()) reduced["license"] = manifest["license"];
        if (manifest.contains("tags") && manifest["tags"].is_array()) reduced["tags"] = manifest["tags"];

        // Keep normalized package dependencies (strip individual source entries)
        nlohmann::json reduced_deps = nlohmann::json::array();
        if (manifest.contains("dependencies") && manifest["dependencies"].is_array()) {
            for (const auto& d : manifest["dependencies"]) {
                if (!d.is_object()) continue;
                std::string dtype = d.value("type", "");
                if (dtype == "source") continue;
                if (dtype == "project" || dtype == "library") {
                    nlohmann::json norm_dep;
                    norm_dep["type"] = "library";
                    if (d.contains("name")) norm_dep["name"] = d["name"];
                    else if (d.contains("package")) norm_dep["name"] = d["package"];
                    if (d.contains("version")) norm_dep["version"] = d["version"];

                    if (d.contains("path") && d["path"].is_string()) {
                        std::filesystem::path dp = d["path"].get<std::string>();
                        if (dp.is_relative()) dp = src_dir / dp;
                        auto dep_mf_p = solix::find_project_manifest(dp);
                        if (std::filesystem::exists(dep_mf_p)) {
                            try {
                                std::ifstream f(dep_mf_p);
                                nlohmann::json j;
                                f >> j;
                                if (!norm_dep.contains("name") || norm_dep["name"].empty()) norm_dep["name"] = j.value("project", "");
                                if (!norm_dep.contains("version") || norm_dep["version"].empty()) norm_dep["version"] = j.value("version", "0.1.0");
                            } catch (...) {}
                        }
                    }
                    if (norm_dep.contains("name") && !norm_dep["name"].empty()) {
                        reduced_deps.push_back(norm_dep);
                    }
                }
            }
        }
        reduced["dependencies"] = reduced_deps;

        std::ofstream out_mf(dest_dir / manifest_path.filename());
        out_mf << reduced.dump(2) << std::endl;
        if (manifest_path.filename() != "manifest.json") {
            std::ofstream out_alt(dest_dir / "manifest.json");
            out_alt << reduced.dump(2) << std::endl;
        }

    } else if (proj_type == "plugin") {
        // Plugin distribution:
        // A. Copy src/ if exists
        if (std::filesystem::exists(src_dir / "src", ec)) {
            copy_dir_recursive(src_dir / "src", dest_dir / "src", true);
        }
        if (!build_res.output_binary.empty() && std::filesystem::exists(build_res.output_binary)) {
            std::filesystem::copy_file(build_res.output_binary, dest_dir / "bytecode.slxbin", std::filesystem::copy_options::overwrite_existing, ec);
        }

        // B. Stage native shared libraries
        std::vector<std::filesystem::path> native_libs;
        std::set<std::string> seen_libs;
        collect_native_libs(src_dir / "lib", native_libs, seen_libs);
        for (const auto& droot : build_res.dependency_roots) {
            collect_native_libs(droot / "lib", native_libs, seen_libs);
        }
        collect_native_libs(build_res.output_binary.parent_path() / "lib", native_libs, seen_libs);
        if (!native_libs.empty()) {
            std::filesystem::create_directories(dest_dir / "lib", ec);
            for (const auto& lib : native_libs) {
                std::filesystem::copy_file(lib, dest_dir / "lib" / lib.filename(), std::filesystem::copy_options::overwrite_existing, ec);
            }
        }

        // C. Stage resources
        for (const auto& droot : build_res.dependency_roots) {
            copy_dir_recursive(droot / "resources", dest_dir / "resources");
        }
        copy_dir_recursive(src_dir / "resources", dest_dir / "resources");
        copy_dir_recursive(build_res.output_binary.parent_path() / "resources", dest_dir / "resources");

        // D. Reduced manifest.json
        nlohmann::json reduced;
        reduced["project"] = name;
        reduced["name"] = name;
        reduced["version"] = version;
        reduced["type"] = "plugin";
        reduced["installed"] = true;
        if (manifest.contains("description") && !manifest["description"].is_null()) reduced["description"] = manifest["description"];
        if (manifest.contains("author") && !manifest["author"].is_null()) reduced["author"] = manifest["author"];
        if (manifest.contains("license") && !manifest["license"].is_null()) reduced["license"] = manifest["license"];
        if (manifest.contains("tags") && manifest["tags"].is_array()) reduced["tags"] = manifest["tags"];

        std::ofstream out_mf(dest_dir / manifest_path.filename());
        out_mf << reduced.dump(2) << std::endl;
        if (manifest_path.filename() != "manifest.json") {
            std::ofstream out_alt(dest_dir / "manifest.json");
            out_alt << reduced.dump(2) << std::endl;
        }

    } else {
        // App distribution:
        // A. Copy compiled bytecode
        if (std::filesystem::exists(build_res.output_binary)) {
            std::filesystem::copy_file(build_res.output_binary, dest_dir / "bytecode.slxbin", std::filesystem::copy_options::overwrite_existing, ec);
        }

        // B. Stage native shared libraries
        std::vector<std::filesystem::path> native_libs;
        std::set<std::string> seen_libs;
        collect_native_libs(src_dir / "lib", native_libs, seen_libs);
        for (const auto& droot : build_res.dependency_roots) {
            collect_native_libs(droot / "lib", native_libs, seen_libs);
        }
        collect_native_libs(build_res.output_binary.parent_path() / "lib", native_libs, seen_libs);
        if (!native_libs.empty()) {
            std::filesystem::create_directories(dest_dir / "lib", ec);
            for (const auto& lib : native_libs) {
                std::filesystem::copy_file(lib, dest_dir / "lib" / lib.filename(), std::filesystem::copy_options::overwrite_existing, ec);
            }
        }

        // C. Stage resources
        for (const auto& droot : build_res.dependency_roots) {
            copy_dir_recursive(droot / "resources", dest_dir / "resources");
        }
        copy_dir_recursive(src_dir / "resources", dest_dir / "resources");
        copy_dir_recursive(build_res.output_binary.parent_path() / "resources", dest_dir / "resources");

        // D. Reduced manifest.json
        nlohmann::json reduced;
        reduced["project"] = name;
        reduced["name"] = name;
        reduced["version"] = version;
        reduced["type"] = "app";
        reduced["installed"] = true;
        reduced["entry"] = "bytecode.slxbin";
        if (manifest.contains("description") && !manifest["description"].is_null()) reduced["description"] = manifest["description"];
        if (manifest.contains("author") && !manifest["author"].is_null()) reduced["author"] = manifest["author"];
        if (manifest.contains("license") && !manifest["license"].is_null()) reduced["license"] = manifest["license"];
        if (manifest.contains("tags") && manifest["tags"].is_array()) reduced["tags"] = manifest["tags"];

        std::ofstream out_mf(dest_dir / manifest_path.filename());
        out_mf << reduced.dump(2) << std::endl;
        if (manifest_path.filename() != "manifest.json") {
            std::ofstream out_alt(dest_dir / "manifest.json");
            out_alt << reduced.dump(2) << std::endl;
        }
    }

    if (should_register) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now_c));
        std::string installed_at(buf);

        std::string description = "";
        if (manifest.contains("description") && manifest["description"].is_string()) {
            description = manifest["description"].get<std::string>();
        }

        registry_["projects"][id] = {
            {"id", id},
            {"name", name},
            {"version", version},
            {"type", proj_type},
            {"path", dest_dir.string()},
            {"original_path", orig_source_path.string()},
            {"installed_at", installed_at},
            {"description", description}
        };

        save_registry();
    }

    return true;
}

bool PackageManager::install_project(const std::filesystem::path& project_path, bool force, std::string& out_id) {
    return install_project(project_path, "release", force, false, {}, out_id);
}

bool PackageManager::uninstall_project(const std::string& name, const std::string& version) {
    std::string id = compute_project_id(name, version);

    if (!registry_["projects"].contains(id)) {
        std::cerr << "Error: No installed project found matching '" << name 
                  << "' version '" << version << "' (ID: " << id << ")." << std::endl;
        return false;
    }

    std::filesystem::path dest_dir = get_installed_dir() / id;
    std::error_code ec;
    if (std::filesystem::exists(dest_dir)) {
        std::filesystem::remove_all(dest_dir, ec);
    }

    registry_["projects"].erase(id);
    save_registry();
    return true;
}

void PackageManager::reload_registry() {
    load_registry();
}

std::vector<InstalledProject> PackageManager::list_installed_projects() {
    load_registry();
    std::vector<InstalledProject> list;
    if (!registry_.contains("projects") || !registry_["projects"].is_object()) {
        return list;
    }

    for (const auto& [_, item] : registry_["projects"].items()) {
        InstalledProject p;
        p.id = item.value("id", "");
        p.name = item.value("name", "");
        p.version = item.value("version", "");
        p.type = item.value("type", "");
        p.path = std::filesystem::path(item.value("path", ""));
        p.original_path = std::filesystem::path(item.value("original_path", ""));
        p.installed_at = item.value("installed_at", "");
        p.description = item.value("description", "");

        if (p.type.empty()) {
            std::filesystem::path mf = solix::find_project_manifest(p.path);
            if (std::filesystem::exists(mf)) {
                try {
                    std::ifstream f(mf);
                    nlohmann::json j;
                    f >> j;
                    p.type = j.value("type", "app");
                } catch (...) {
                    p.type = "app";
                }
            } else {
                p.type = "app";
            }
        }

        list.push_back(p);
    }
    return list;
}

std::optional<InstalledProject> PackageManager::get_project_by_name_and_version(const std::string& name, const std::string& version) {
    load_registry();
    std::string id = compute_project_id(name, version);
    if (registry_["projects"].contains(id)) {
        const auto& item = registry_["projects"][id];
        InstalledProject p;
        p.id = item.value("id", "");
        p.name = item.value("name", "");
        p.version = item.value("version", "");
        p.type = item.value("type", "");
        p.path = std::filesystem::path(item.value("path", ""));
        p.original_path = std::filesystem::path(item.value("original_path", ""));
        p.installed_at = item.value("installed_at", "");
        p.description = item.value("description", "");

        if (p.type.empty()) {
            std::filesystem::path mf = solix::find_project_manifest(p.path);
            if (std::filesystem::exists(mf)) {
                try {
                    std::ifstream f(mf);
                    nlohmann::json j;
                    f >> j;
                    p.type = j.value("type", "app");
                } catch (...) {
                    p.type = "app";
                }
            } else {
                p.type = "app";
            }
        }

        return p;
    }
    return std::nullopt;
}

std::vector<InstalledProject> PackageManager::get_projects_by_name(const std::string& name) {
    std::vector<InstalledProject> matches;
    for (const auto& p : list_installed_projects()) {
        if (p.name == name) {
            matches.push_back(p);
        }
    }
    return matches;
}

std::optional<nlohmann::json> PackageManager::load_installed_manifest(const InstalledProject& proj) {
    std::filesystem::path manifest_path = solix::find_project_manifest(proj.path);
    if (!std::filesystem::exists(manifest_path)) {
        return std::nullopt;
    }
    try {
        std::ifstream file(manifest_path);
        nlohmann::json root;
        file >> root;
        return root;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<nlohmann::json> PackageManager::load_manifest_from_bundle(const std::filesystem::path& pac_path) {
    std::error_code ec;
    if (!std::filesystem::exists(pac_path, ec)) return std::nullopt;
    solix::ZipArchiveReader reader(pac_path);
    if (!reader.is_valid()) return std::nullopt;
    auto content = reader.read_entry_as_string("manifest.json");
    if (!content) {
        content = reader.read_entry_as_string("solix.json");
    }
    if (!content) return std::nullopt;
    try {
        return nlohmann::json::parse(*content);
    } catch (...) {
        return std::nullopt;
    }
}

uintmax_t PackageManager::calculate_directory_size(const std::filesystem::path& dir) {
    uintmax_t total = 0;
    std::error_code ec;
    if (!std::filesystem::exists(dir)) return 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, ec)) {
        if (std::filesystem::is_regular_file(entry, ec)) {
            total += std::filesystem::file_size(entry, ec);
        }
    }
    return total;
}

std::optional<std::filesystem::path> PackageManager::discover_bundled_library(const std::string& name) {
    std::filesystem::path exe_dir = solix::get_executable_dir();

    std::vector<std::string> search_names = { name };
    if (name == "solix-raylib") search_names.push_back("raylib");
    else if (name == "raylib") search_names.push_back("solix-raylib");

    for (const auto& n : search_names) {
        // 1. Installed package layout: <prefix>/share/solix/<name>
        std::vector<std::filesystem::path> candidates = {
            exe_dir.parent_path() / "share" / "solix" / n,
            exe_dir / ".." / "share" / "solix" / n,
            exe_dir / "share" / "solix" / n,
            exe_dir.parent_path() / "share" / "solix" / "libs" / n,
            // 2. In-tree development layout: <repo>/libs/<name> or <repo>/<name>
            exe_dir.parent_path().parent_path() / "libs" / n,
            exe_dir.parent_path() / "libs" / n,
            exe_dir.parent_path().parent_path() / n,
            exe_dir.parent_path() / n,
            exe_dir.parent_path().parent_path() / n / "project",
            exe_dir.parent_path() / n / "project"
        };

        for (const auto& cand : candidates) {
            std::filesystem::path normal_cand = cand.lexically_normal();
            std::filesystem::path mf = solix::find_project_manifest(normal_cand);
            if (std::filesystem::exists(mf) && std::filesystem::exists(normal_cand / "src")) {
                return normal_cand;
            }
        }
    }
    return std::nullopt;
}

std::optional<std::filesystem::path> PackageManager::discover_bundled_solixlib() {
    return discover_bundled_library("solixlib");
}

std::vector<std::filesystem::path> PackageManager::discover_all_bundled_libraries() {
    std::vector<std::filesystem::path> results;
    std::set<std::string> seen;

    auto check_and_add = [&](const std::filesystem::path& p) {
        std::filesystem::path norm = p.lexically_normal();
        std::filesystem::path mf = solix::find_project_manifest(norm);
        if (std::filesystem::exists(mf) && std::filesystem::exists(norm / "src")) {
            if (seen.insert(norm.string()).second) {
                results.push_back(norm);
            }
        }
    };

    std::filesystem::path exe_dir = solix::get_executable_dir();
    std::vector<std::filesystem::path> search_roots = {
        exe_dir.parent_path() / "share" / "solix",
        exe_dir / ".." / "share" / "solix",
        exe_dir / "share" / "solix",
        exe_dir.parent_path().parent_path() / "libs",
        exe_dir.parent_path() / "libs"
    };

    for (const auto& root : search_roots) {
        std::error_code ec;
        if (std::filesystem::exists(root, ec) && std::filesystem::is_directory(root, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
                if (entry.is_directory(ec)) {
                    check_and_add(entry.path());
                }
            }
        }
    }

    // Explicitly check standard library and raylib
    auto s = discover_bundled_library("solixlib");
    if (s) check_and_add(s.value());
    auto r = discover_bundled_library("raylib");
    if (r) check_and_add(r.value());

    return results;
}

bool PackageManager::try_auto_install_bundled_library(const std::string& name) {
    auto bundled_opt = discover_bundled_library(name);
    if (!bundled_opt) {
        return false;
    }
    std::string out_id;
    return install_project(bundled_opt.value(), true, out_id);
}

bool PackageManager::try_auto_install_all_bundled_libraries() {
    auto all = discover_all_bundled_libraries();
    bool any_success = false;
    for (const auto& lib_path : all) {
        std::string out_id;
        if (install_project(lib_path, true, out_id)) {
            any_success = true;
        }
    }
    return any_success;
}

bool PackageManager::try_auto_install_bundled_solixlib() {
    return try_auto_install_bundled_library("solixlib");
}

} // namespace solix::cli
