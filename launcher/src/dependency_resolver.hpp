#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "solix/compilation.hpp"
#include "package_manager.hpp"
#include "semver.hpp"

namespace solix::cli {

struct ProjectCandidate {
    std::string name;
    SemVer version;
    std::filesystem::path manifest_path;
    std::filesystem::path project_root;
    nlohmann::json manifest;
};

/**
 * @brief Extensible interface for resolving different project dependency types.
 */
class IDependencyResolver {
public:
    virtual ~IDependencyResolver() = default;
    virtual bool can_resolve(const std::string& type) const = 0;
    virtual bool resolve(const nlohmann::json& dep_node,
                         const std::filesystem::path& project_root,
                         CompilationOptions& opts) = 0;
};

/**
 * @brief Resolves local source file dependencies ("type": "source").
 */
class SourceDependencyResolver : public IDependencyResolver {
public:
    bool can_resolve(const std::string& type) const override {
        return type == "source";
    }

    bool resolve(const nlohmann::json& dep_node,
                 const std::filesystem::path& project_root,
                 CompilationOptions& opts) override {
        if (!dep_node.contains("path") || !dep_node["path"].is_string()) {
            std::cerr << "Error: 'source' dependency missing valid 'path' string." << std::endl;
            return false;
        }

        std::string raw_path = dep_node["path"].get<std::string>();
        std::filesystem::path file_path(raw_path);
        if (file_path.is_relative()) {
            file_path = project_root / file_path;
        }
        file_path = file_path.lexically_normal();

        if (!std::filesystem::exists(file_path)) {
            std::cerr << "Error: Dependency source file does not exist: " << file_path.string() << std::endl;
            return false;
        }

        std::ifstream t(file_path);
        if (!t.is_open()) {
            std::cerr << "Error: Could not open source dependency file: " << file_path.string() << std::endl;
            return false;
        }

        std::stringstream buffer;
        buffer << t.rdbuf();
        opts.sources[file_path.string()] = buffer.str();
        return true;
    }
};

/**
 * @brief Coordinates two-phase dependency resolution, transitive graph walking,
 * circular reference handling, and SemVer conflict resolution.
 */
class DependencyManager {
public:
    DependencyManager() {
        register_resolver(std::make_unique<SourceDependencyResolver>());
    }

    void register_resolver(std::unique_ptr<IDependencyResolver> resolver) {
        resolvers_.push_back(std::move(resolver));
    }

    /**
     * @brief Resolves all dependencies for a project manifest and active profile before compilation.
     * Performs transitive dependency walking, cycle de-duplication, SemVer conflict resolution,
     * and source code ingestion.
     */
    bool resolve_all(const nlohmann::json& root_manifest,
                     const std::filesystem::path& root_project_root,
                     const nlohmann::json& active_profile,
                     CompilationOptions& opts) {
        std::set<std::string> visited_manifests;
        std::vector<ProjectCandidate> discovered_projects;

        // Register root project
        std::string root_name = root_manifest.value("project", "root_project");
        std::string root_version_str = root_manifest.value("version", "0.1.0");
        auto root_ver = SemVer::parse(root_version_str).value_or(SemVer{0, 1, 0, root_version_str});

        std::filesystem::path root_manifest_path = (root_project_root / "solix.json").lexically_normal();
        visited_manifests.insert(root_manifest_path.string());

        ProjectCandidate root_cand;
        root_cand.name = root_name;
        root_cand.version = root_ver;
        root_cand.manifest_path = root_manifest_path;
        root_cand.project_root = root_project_root;
        root_cand.manifest = root_manifest;
        discovered_projects.push_back(root_cand);

        // 1. Discover all project-type dependencies recursively
        if (!discover_dependencies(root_manifest, root_project_root, visited_manifests, discovered_projects)) {
            return false;
        }

        // Also discover any project dependencies in active profile's additional_dependencies
        if (active_profile.contains("compilation") && active_profile["compilation"].is_object()) {
            const auto& comp = active_profile["compilation"];
            if (comp.contains("additional_dependencies") && comp["additional_dependencies"].is_array()) {
                for (const auto& dep : comp["additional_dependencies"]) {
                    if (dep.is_object() && dep.value("type", "") == "project") {
                        if (!discover_single_project_dependency(dep, root_project_root, visited_manifests, discovered_projects)) {
                            return false;
                        }
                    }
                }
            }
        }

        // 2. Group discovered projects by name and apply SemVer conflict resolution
        std::map<std::string, std::vector<ProjectCandidate>> projects_by_name;
        for (const auto& p : discovered_projects) {
            projects_by_name[p.name].push_back(p);
        }

        std::vector<ProjectCandidate> final_selected_projects;

        for (auto& [name, candidates] : projects_by_name) {
            if (candidates.size() == 1) {
                final_selected_projects.push_back(candidates[0]);
                continue;
            }

            // Multiple references to the same project name: check version compatibility
            std::sort(candidates.begin(), candidates.end(), [](const ProjectCandidate& a, const ProjectCandidate& b) {
                return a.version < b.version;
            });

            const auto& lowest = candidates.front();
            const auto& highest = candidates.back();

            // Check for incompatible major versions
            for (size_t i = 0; i < candidates.size(); ++i) {
                for (size_t j = i + 1; j < candidates.size(); ++j) {
                    if (candidates[i].version.major != candidates[j].version.major) {
                        std::cerr << "Error: Incompatible major versions for dependency '" << name << "': "
                                  << candidates[i].version.raw << " vs " << candidates[j].version.raw
                                  << ". Cannot automatically resolve breaking changes." << std::endl;
                        return false;
                    }
                }
            }

            // Check for minor version differences (same major, different minor)
            if (lowest.version.minor != highest.version.minor) {
                std::cout << "Warning: Project '" << name << "' has multiple minor versions ("
                          << lowest.version.raw << " vs " << highest.version.raw
                          << "). Selecting highest version " << highest.version.raw << "." << std::endl;
            }

            // Select highest version candidate
            final_selected_projects.push_back(highest);
        }

        // 3. Pre-compilation Source Ingestion across all selected projects
        for (const auto& proj : final_selected_projects) {
            if (proj.manifest.contains("dependencies") && proj.manifest["dependencies"].is_array()) {
                for (const auto& dep : proj.manifest["dependencies"]) {
                    if (dep.is_object() && dep.value("type", "") == "source") {
                        if (!resolve_source_dependency(dep, proj.project_root, opts)) {
                            return false;
                        }
                    }
                }
            }
        }

        // Ingest profile-specific source dependencies from the root project
        if (active_profile.contains("compilation") && active_profile["compilation"].is_object()) {
            const auto& comp = active_profile["compilation"];
            if (comp.contains("additional_dependencies") && comp["additional_dependencies"].is_array()) {
                for (const auto& dep : comp["additional_dependencies"]) {
                    if (dep.is_object() && dep.value("type", "") == "source") {
                        if (!resolve_source_dependency(dep, root_project_root, opts)) {
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

private:
    bool resolve_source_dependency(const nlohmann::json& dep_node,
                                  const std::filesystem::path& project_root,
                                  CompilationOptions& opts) {
        SourceDependencyResolver resolver;
        return resolver.resolve(dep_node, project_root, opts);
    }

    bool discover_single_project_dependency(const nlohmann::json& dep_node,
                                           const std::filesystem::path& current_root,
                                           std::set<std::string>& visited_manifests,
                                           std::vector<ProjectCandidate>& discovered_projects) {
        std::filesystem::path target_manifest;

        if (dep_node.contains("path") && dep_node["path"].is_string()) {
            std::string raw_path = dep_node["path"].get<std::string>();
            std::filesystem::path p(raw_path);
            if (p.is_relative()) {
                p = current_root / p;
            }
            if (std::filesystem::is_directory(p)) {
                p /= "solix.json";
            }
            target_manifest = p.lexically_normal();
        } else if (dep_node.contains("name") && dep_node["name"].is_string()) {
            std::string name = dep_node["name"].get<std::string>();
            std::string version = dep_node.value("version", "");

            PackageManager pm;
            std::optional<InstalledProject> matched;

            if (!version.empty()) {
                matched = pm.get_project_by_name_and_version(name, version);
            } else {
                auto all_matches = pm.get_projects_by_name(name);
                if (!all_matches.empty()) {
                    // Pick the highest version installed
                    std::sort(all_matches.begin(), all_matches.end(), [](const InstalledProject& a, const InstalledProject& b) {
                        auto va = SemVer::parse(a.version).value_or(SemVer{0, 0, 0, a.version});
                        auto vb = SemVer::parse(b.version).value_or(SemVer{0, 0, 0, b.version});
                        return va < vb;
                    });
                    matched = all_matches.back();
                }
            }

            if (!matched) {
                std::cerr << "Error: Could not find installed project package '" << name
                          << (version.empty() ? "" : "' version '" + version) << "'." << std::endl;
                return false;
            }
            target_manifest = (matched->path / "solix.json").lexically_normal();
        } else {
            std::cerr << "Error: 'project' dependency must specify either 'path' or 'name'." << std::endl;
            return false;
        }

        if (!std::filesystem::exists(target_manifest)) {
            std::cerr << "Error: Project dependency manifest does not exist: " << target_manifest.string() << std::endl;
            return false;
        }

        // Circular & Diamond Dependency Guard:
        // If manifest was already visited, stop recursion cleanly without error.
        if (visited_manifests.count(target_manifest.string())) {
            return true;
        }
        visited_manifests.insert(target_manifest.string());

        std::ifstream mf(target_manifest);
        if (!mf.is_open()) {
            std::cerr << "Error: Could not open project dependency manifest: " << target_manifest.string() << std::endl;
            return false;
        }

        nlohmann::json manifest;
        try {
            mf >> manifest;
        } catch (const nlohmann::json::parse_error& e) {
            std::cerr << "Error: Failed to parse dependency manifest " << target_manifest.string() << ": " << e.what() << std::endl;
            return false;
        }

        std::string proj_name = manifest.value("project", "");
        if (proj_name.empty()) {
            std::cerr << "Error: Dependency manifest missing 'project' name: " << target_manifest.string() << std::endl;
            return false;
        }

        std::string ver_str = manifest.value("version", "0.1.0");
        auto ver = SemVer::parse(ver_str).value_or(SemVer{0, 1, 0, ver_str});

        ProjectCandidate cand;
        cand.name = proj_name;
        cand.version = ver;
        cand.manifest_path = target_manifest;
        cand.project_root = target_manifest.parent_path();
        cand.manifest = manifest;
        discovered_projects.push_back(cand);

        // Recurse into this project's dependencies (Transitive dependencies)
        return discover_dependencies(manifest, cand.project_root, visited_manifests, discovered_projects);
    }

    bool discover_dependencies(const nlohmann::json& manifest,
                               const std::filesystem::path& project_root,
                               std::set<std::string>& visited_manifests,
                               std::vector<ProjectCandidate>& discovered_projects) {
        if (!manifest.contains("dependencies") || !manifest["dependencies"].is_array()) {
            return true;
        }

        for (const auto& dep : manifest["dependencies"]) {
            if (!dep.is_object()) continue;
            std::string type = dep.value("type", "");
            if (type == "project") {
                if (!discover_single_project_dependency(dep, project_root, visited_manifests, discovered_projects)) {
                    return false;
                }
            }
        }

        return true;
    }

    std::vector<std::unique_ptr<IDependencyResolver>> resolvers_;
};

} // namespace solix::cli
