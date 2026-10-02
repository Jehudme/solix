#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace solix::cli {

struct InstalledProject {
    std::string id;
    std::string name;
    std::string version;
    std::filesystem::path path;
    std::filesystem::path original_path;
    std::string installed_at;
    std::string description;
};

class PackageManager {
public:
    PackageManager();

    // Cross-platform paths
    static std::filesystem::path get_solix_home();
    static std::filesystem::path get_installed_dir();
    static std::filesystem::path get_registry_path();

    // Deterministic 16-hex hash ID from name and version
    static std::string compute_project_id(const std::string& name, const std::string& version);

    // Lifecycle operations
    bool install_project(const std::filesystem::path& project_path, bool force, std::string& out_id);
    bool uninstall_project(const std::string& name, const std::string& version);
    std::vector<InstalledProject> list_installed_projects();
    std::optional<InstalledProject> get_project_by_name_and_version(const std::string& name, const std::string& version);
    std::vector<InstalledProject> get_projects_by_name(const std::string& name);

    // Load full solix.json manifest of an installed project
    static std::optional<nlohmann::json> load_installed_manifest(const InstalledProject& proj);
    static uintmax_t calculate_directory_size(const std::filesystem::path& dir);

private:
    void load_registry();
    void save_registry();

    nlohmann::json registry_;
};

} // namespace solix::cli
