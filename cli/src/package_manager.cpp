#include "package_manager.hpp"
#include "solix/path_utils.hpp"
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <set>

namespace solix::cli {

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

bool PackageManager::install_project(const std::filesystem::path& project_path, bool force, std::string& out_id) {
    std::filesystem::path src_dir = std::filesystem::absolute(project_path);
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
    std::string version = manifest.value("version", "");

    if (name.empty()) {
        std::cerr << "Error: Project manifest must define a 'project' name." << std::endl;
        return false;
    }
    if (version.empty()) {
        std::cerr << "Error: Project manifest must define a 'version'." << std::endl;
        return false;
    }

    std::string id = compute_project_id(name, version);
    out_id = id;

    if (registry_["projects"].contains(id) && !force) {
        std::cerr << "Error: Project '" << name << "' version '" << version 
                  << "' is already installed (ID: " << id << "). Use --force to reinstall." << std::endl;
        return false;
    }

    std::filesystem::path dest_dir = get_installed_dir() / id;
    std::error_code ec;
    if (std::filesystem::exists(dest_dir)) {
        std::filesystem::remove_all(dest_dir, ec);
    }
    std::filesystem::create_directories(dest_dir, ec);

    // Copy project files, excluding transient build artifacts
    for (const auto& entry : std::filesystem::directory_iterator(src_dir)) {
        std::string filename = entry.path().filename().string();
        if (filename == "build" || filename == ".git" || filename == ".vscode") {
            continue;
        }
        std::filesystem::copy(entry.path(), dest_dir / filename,
                             std::filesystem::copy_options::recursive |
                             std::filesystem::copy_options::overwrite_existing,
                             ec);
    }

    // Timestamp
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
        {"path", dest_dir.string()},
        {"original_path", src_dir.string()},
        {"installed_at", installed_at},
        {"description", description}
    };

    save_registry();
    return true;
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
        p.path = std::filesystem::path(item.value("path", ""));
        p.original_path = std::filesystem::path(item.value("original_path", ""));
        p.installed_at = item.value("installed_at", "");
        p.description = item.value("description", "");
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
        p.path = std::filesystem::path(item.value("path", ""));
        p.original_path = std::filesystem::path(item.value("original_path", ""));
        p.installed_at = item.value("installed_at", "");
        p.description = item.value("description", "");
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
