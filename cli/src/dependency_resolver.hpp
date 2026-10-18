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
#include "solix/path_utils.hpp"
#include "package_manager.hpp"
#include "semver.hpp"

namespace solix::cli {

struct ProjectCandidate {
    std::string name;
    SemVer version;
    std::filesystem::path manifest_path;
    std::filesystem::path project_root;
    nlohmann::json manifest;
    std::string selected_profile;
    bool explicit_profile{false};
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
     * @brief Resolves the complete dependency graph and candidate profiles in topological order.
     */
    bool resolve_graph(const nlohmann::json& root_manifest,
                       const std::filesystem::path& root_project_root,
                       const nlohmann::json& active_profile_json,
                       const std::string& active_profile_name,
                       std::vector<ProjectCandidate>& out_candidates) {
        std::set<std::string> visited_manifests;
        std::vector<ProjectCandidate> discovered_projects;
        std::vector<std::string> discovery_order;

        // Register root project
        std::string root_name = root_manifest.value("project", "root_project");
        std::string root_version_str = root_manifest.value("version", "0.1.0");
        auto root_ver = SemVer::parse(root_version_str).value_or(SemVer{0, 1, 0, root_version_str});

        std::filesystem::path root_manifest_path = solix::find_project_manifest(root_project_root).lexically_normal();
        visited_manifests.insert(root_manifest_path.string());

        ProjectCandidate root_cand;
        root_cand.name = root_name;
        root_cand.version = root_ver;
        root_cand.manifest_path = root_manifest_path;
        root_cand.project_root = root_project_root;
        root_cand.manifest = root_manifest;
        root_cand.selected_profile = active_profile_name;
        root_cand.explicit_profile = true;

        // 1. Discover all project-type dependencies recursively
        if (!discover_dependencies(root_manifest, root_project_root, active_profile_name, visited_manifests, discovered_projects, discovery_order)) {
            return false;
        }

        // Also discover any project dependencies in active profile's additional_dependencies
        const nlohmann::json* comp_ptr = nullptr;
        if (active_profile_json.contains("compilation") && active_profile_json["compilation"].is_object()) {
            comp_ptr = &active_profile_json["compilation"];
        } else if (root_manifest.contains("profiles") && root_manifest["profiles"].is_object() &&
                   root_manifest["profiles"].contains(active_profile_name) &&
                   root_manifest["profiles"][active_profile_name].contains("compilation") &&
                   root_manifest["profiles"][active_profile_name]["compilation"].is_object()) {
            comp_ptr = &root_manifest["profiles"][active_profile_name]["compilation"];
        }

        if (comp_ptr && comp_ptr->contains("additional_dependencies") && (*comp_ptr)["additional_dependencies"].is_array()) {
            for (const auto& dep : (*comp_ptr)["additional_dependencies"]) {
                if (dep.is_object()) {
                    std::string type = dep.value("type", "");
                    if (type == "project" || type == "library") {
                        if (!discover_single_project_dependency(dep, root_project_root, active_profile_name, visited_manifests, discovered_projects, discovery_order)) {
                            return false;
                        }
                    }
                }
            }
        }

        // Add root project candidate after all dependencies
        discovered_projects.push_back(root_cand);
        if (std::find(discovery_order.begin(), discovery_order.end(), root_name) == discovery_order.end()) {
            discovery_order.push_back(root_name);
        }

        // 2. Group discovered projects by name and apply SemVer conflict resolution
        std::map<std::string, std::vector<ProjectCandidate>> projects_by_name;
        for (const auto& p : discovered_projects) {
            projects_by_name[p.name].push_back(p);
        }

        std::vector<ProjectCandidate> final_selected_projects;

        for (const auto& name : discovery_order) {
            auto it = projects_by_name.find(name);
            if (it == projects_by_name.end() || it->second.empty()) continue;
            auto& candidates = it->second;

            if (candidates.size() == 1) {
                final_selected_projects.push_back(candidates[0]);
                continue;
            }

            // Multiple references to the same project name: check version compatibility
            std::sort(candidates.begin(), candidates.end(), [](const ProjectCandidate& a, const ProjectCandidate& b) {
                return a.version < b.version;
            });

            const auto& lowest = candidates.front();
            auto highest = candidates.back();

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

            // If highest candidate didn't have explicit profile, adopt explicit profile from any candidate if available
            if (!highest.explicit_profile) {
                for (const auto& c : candidates) {
                    if (c.explicit_profile) {
                        highest.selected_profile = c.selected_profile;
                        highest.explicit_profile = true;
                        break;
                    }
                }
            }

            final_selected_projects.push_back(highest);
        }

        out_candidates = std::move(final_selected_projects);
        return true;
    }

    /**
     * @brief Ingests all sources across selected candidates into CompilationOptions.
     */
    bool ingest_sources(const std::vector<ProjectCandidate>& candidates,
                        CompilationOptions& opts,
                        const nlohmann::json& active_profile_json = nlohmann::json::object(),
                        const std::filesystem::path& root_project_root = {}) {
        SourceDependencyResolver resolver;
        for (const auto& proj : candidates) {
            // Determine whether to auto-discover all *.slx files under src/
            bool is_installed_library = proj.manifest.value("installed", false);
            bool is_root = (proj.project_root == root_project_root);

            bool has_explicit_source_deps = false;
            if (proj.manifest.contains("dependencies") && proj.manifest["dependencies"].is_array()) {
                for (const auto& dep : proj.manifest["dependencies"]) {
                    if (dep.is_object() && dep.value("type", "") == "source") {
                        has_explicit_source_deps = true;
                        break;
                    }
                }
            }
            if (!has_explicit_source_deps && !proj.selected_profile.empty() &&
                proj.manifest.contains("profiles") && proj.manifest["profiles"].is_object() &&
                proj.manifest["profiles"].contains(proj.selected_profile)) {
                const auto& prof = proj.manifest["profiles"][proj.selected_profile];
                if (prof.contains("compilation") && prof["compilation"].is_object()) {
                    const auto& comp = prof["compilation"];
                    if (comp.contains("additional_dependencies") && comp["additional_dependencies"].is_array()) {
                        for (const auto& dep : comp["additional_dependencies"]) {
                            if (dep.is_object() && dep.value("type", "") == "source") {
                                has_explicit_source_deps = true;
                                break;
                            }
                        }
                    }
                }
            }

            bool should_auto_discover = is_root || is_installed_library || !has_explicit_source_deps;

            // 0. Auto-discover all *.slx files recursively under src/
            if (should_auto_discover) {
                std::error_code ec;
                std::filesystem::path src_dir = proj.project_root / "src";
                if (std::filesystem::exists(src_dir, ec) && std::filesystem::is_directory(src_dir, ec)) {
                    for (const auto& entry : std::filesystem::recursive_directory_iterator(src_dir, ec)) {
                        if (entry.is_regular_file(ec) && entry.path().extension() == ".slx") {
                            if (proj.project_root != root_project_root && entry.path().filename() == "main.slx") {
                                continue;
                            }
                            std::filesystem::path file_path = entry.path().lexically_normal();
                            if (opts.sources.find(file_path.string()) == opts.sources.end()) {
                                std::ifstream t(file_path);
                                if (t.is_open()) {
                                    std::stringstream buffer;
                                    buffer << t.rdbuf();
                                    opts.sources[file_path.string()] = buffer.str();
                                }
                            }
                        }
                    }
                }
            }

            // 1. Manifest top-level source dependencies
            if (proj.manifest.contains("dependencies") && proj.manifest["dependencies"].is_array()) {
                for (const auto& dep : proj.manifest["dependencies"]) {
                    if (dep.is_object() && dep.value("type", "") == "source") {
                        if (!resolver.resolve(dep, proj.project_root, opts)) {
                            return false;
                        }
                    }
                }
            }

            // 2. Profile-specific source dependencies from selected profile
            if (!proj.selected_profile.empty() && proj.manifest.contains("profiles") &&
                proj.manifest["profiles"].is_object() && proj.manifest["profiles"].contains(proj.selected_profile)) {
                const auto& prof = proj.manifest["profiles"][proj.selected_profile];
                if (prof.contains("compilation") && prof["compilation"].is_object()) {
                    const auto& comp = prof["compilation"];
                    if (comp.contains("additional_dependencies") && comp["additional_dependencies"].is_array()) {
                        for (const auto& dep : comp["additional_dependencies"]) {
                            if (dep.is_object() && dep.value("type", "") == "source") {
                                if (!resolver.resolve(dep, proj.project_root, opts)) {
                                    return false;
                                }
                            }
                        }
                    }
                }
            }
        }

        // 3. Fallback for ad-hoc active_profile not in manifest["profiles"]
        if (!root_project_root.empty() && active_profile_json.contains("compilation") && active_profile_json["compilation"].is_object()) {
            const auto& comp = active_profile_json["compilation"];
            if (comp.contains("additional_dependencies") && comp["additional_dependencies"].is_array()) {
                for (const auto& dep : comp["additional_dependencies"]) {
                    if (dep.is_object() && dep.value("type", "") == "source") {
                        if (!resolver.resolve(dep, root_project_root, opts)) {
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

    /**
     * @brief Resolves all dependencies for a project manifest and active profile before compilation.
     * Performs transitive dependency walking, cycle de-duplication, SemVer conflict resolution,
     * and source code ingestion.
     */
    bool resolve_all(const nlohmann::json& root_manifest,
                     const std::filesystem::path& root_project_root,
                     const nlohmann::json& active_profile,
                     CompilationOptions& opts,
                     std::vector<std::filesystem::path>* out_dependency_roots = nullptr,
                     const std::string& active_profile_name = "debug",
                     std::vector<ProjectCandidate>* out_resolved_dependencies = nullptr) {
        std::vector<ProjectCandidate> candidates;
        if (!resolve_graph(root_manifest, root_project_root, active_profile, active_profile_name, candidates)) {
            return false;
        }

        if (!ingest_sources(candidates, opts, active_profile, root_project_root)) {
            return false;
        }

        if (out_dependency_roots) {
            for (const auto& proj : candidates) {
                if (proj.project_root != root_project_root) {
                    out_dependency_roots->push_back(proj.project_root);
                }
            }
        }

        if (out_resolved_dependencies) {
            for (const auto& proj : candidates) {
                if (proj.project_root != root_project_root) {
                    out_resolved_dependencies->push_back(proj);
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
                                           const std::string& parent_active_profile,
                                           std::set<std::string>& visited_manifests,
                                           std::vector<ProjectCandidate>& discovered_projects,
                                           std::vector<std::string>& discovery_order) {
        std::filesystem::path target_manifest;

        std::string raw_pkg;
        if (dep_node.contains("package") && dep_node["package"].is_string()) {
            raw_pkg = dep_node["package"].get<std::string>();
        } else if (dep_node.contains("name") && dep_node["name"].is_string()) {
            raw_pkg = dep_node["name"].get<std::string>();
        }

        if (dep_node.contains("path") && dep_node["path"].is_string()) {
            std::string raw_path = dep_node["path"].get<std::string>();
            std::filesystem::path p(raw_path);
            if (p.is_relative()) {
                p = current_root / p;
            }
            target_manifest = solix::find_project_manifest(p);
        } else if (!raw_pkg.empty()) {
            std::string name;
            std::string version;
            auto at_pos = raw_pkg.find('@');
            if (at_pos != std::string::npos) {
                name = raw_pkg.substr(0, at_pos);
                version = raw_pkg.substr(at_pos + 1);
            } else {
                name = raw_pkg;
                version = dep_node.value("version", "");
            }

            PackageManager pm;
            std::optional<InstalledProject> matched;

            // Automatically synchronize bundled library (solixlib, raylib, etc.) if available and not yet installed
            auto bundled_opt = PackageManager::discover_bundled_library(name);
            if (bundled_opt) {
                auto b_mf_path = solix::find_project_manifest(bundled_opt.value());
                if (std::filesystem::exists(b_mf_path)) {
                    try {
                        std::ifstream f(b_mf_path);
                        nlohmann::json b_mf;
                        f >> b_mf;
                        std::string b_ver = b_mf.value("version", "");
                        if (!b_ver.empty()) {
                            auto cur = pm.get_project_by_name_and_version(name, b_ver);
                            if (!cur) {
                                pm.try_auto_install_bundled_library(name);
                            }
                        }
                    } catch (...) {}
                }
            }

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
                if (pm.try_auto_install_bundled_library(name)) {
                    if (!version.empty()) {
                        matched = pm.get_project_by_name_and_version(name, version);
                    }
                }
                // Fallback: if exact requested version wasn't matched, pick latest installed
                if (!matched) {
                    auto all_matches = pm.get_projects_by_name(name);
                    if (!all_matches.empty()) {
                        std::sort(all_matches.begin(), all_matches.end(), [](const InstalledProject& a, const InstalledProject& b) {
                            auto va = SemVer::parse(a.version).value_or(SemVer{0, 0, 0, a.version});
                            auto vb = SemVer::parse(b.version).value_or(SemVer{0, 0, 0, b.version});
                            return va < vb;
                        });
                        matched = all_matches.back();
                    }
                }
            }

            if (!matched) {
                std::cerr << "Error: Could not find installed project package '" << name
                          << (version.empty() ? "" : "' version '" + version) << "'." << std::endl;
                return false;
            }
            target_manifest = solix::find_project_manifest(matched->path);
        } else {
            std::cerr << "Error: 'project' or 'library' dependency must specify 'package' (or 'name') as 'name@version', or 'path'." << std::endl;
            return false;
        }

        if (!std::filesystem::exists(target_manifest)) {
            std::cerr << "Error: Project dependency manifest does not exist: " << target_manifest.string() << std::endl;
            return false;
        }

        // Circular & Diamond Dependency Guard:
        if (visited_manifests.count(target_manifest.string())) {
            // If current reference has an explicit profile and previous didn't, adopt it
            if (dep_node.contains("profile") && dep_node["profile"].is_string() && !dep_node["profile"].get<std::string>().empty()) {
                std::string req = dep_node["profile"].get<std::string>();
                for (auto& cand : discovered_projects) {
                    if (cand.manifest_path == target_manifest && !cand.explicit_profile) {
                        cand.selected_profile = req;
                        cand.explicit_profile = true;
                    }
                }
            }
            return true;
        }

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

        if (manifest.contains("type") && !manifest["type"].is_null()) {
            std::string proj_type = manifest["type"].get<std::string>();
            std::transform(proj_type.begin(), proj_type.end(), proj_type.begin(), [](unsigned char c) { return std::tolower(c); });
            if (proj_type == "app") {
                std::cerr << "Error: Cannot include project '" << proj_name << "' as a dependency because it is an 'app' (only 'library' projects can be included)." << std::endl;
                return false;
            }
        }

        std::string ver_str = manifest.value("version", "0.1.0");
        auto ver = SemVer::parse(ver_str).value_or(SemVer{0, 1, 0, ver_str});

        // Resolve selected profile
        bool explicit_profile = false;
        std::string selected_profile;

        if (dep_node.contains("profile") && dep_node["profile"].is_string() && !dep_node["profile"].get<std::string>().empty()) {
            std::string requested = dep_node["profile"].get<std::string>();
            explicit_profile = true;
            if (!manifest.contains("profiles") || !manifest["profiles"].is_object() || !manifest["profiles"].contains(requested)) {
                std::string avail_str = "[";
                if (manifest.contains("profiles") && manifest["profiles"].is_object()) {
                    size_t idx = 0;
                    for (auto& [k, _] : manifest["profiles"].items()) {
                        if (idx > 0) avail_str += ", ";
                        avail_str += k;
                        idx++;
                    }
                }
                avail_str += "]";
                std::cerr << "Error: Dependency project '" << proj_name << "' does not define requested profile '"
                          << requested << "'. Available profiles: " << avail_str << std::endl;
                return false;
            }
            selected_profile = requested;
        } else {
            // Fallback / inheritance
            if (manifest.contains("profiles") && manifest["profiles"].is_object() && !manifest["profiles"].empty()) {
                if (!parent_active_profile.empty() && manifest["profiles"].contains(parent_active_profile)) {
                    selected_profile = parent_active_profile;
                } else if (manifest["profiles"].contains("debug")) {
                    selected_profile = "debug";
                } else {
                    selected_profile = manifest["profiles"].begin().key();
                }
            }
        }

        visited_manifests.insert(target_manifest.string());
        std::filesystem::path cand_root = target_manifest.parent_path();

        // Recurse into this project's dependencies (Transitive dependencies) in post-order:
        // 1. Top-level project dependencies
        if (!discover_dependencies(manifest, cand_root, selected_profile, visited_manifests, discovered_projects, discovery_order)) {
            return false;
        }

        // 2. Profile-specific project dependencies
        if (!selected_profile.empty() && manifest.contains("profiles") &&
            manifest["profiles"].is_object() && manifest["profiles"].contains(selected_profile)) {
            const auto& prof_node = manifest["profiles"][selected_profile];
            if (prof_node.contains("compilation") && prof_node["compilation"].is_object()) {
                const auto& comp_node = prof_node["compilation"];
                if (comp_node.contains("additional_dependencies") && comp_node["additional_dependencies"].is_array()) {
                    for (const auto& child_dep : comp_node["additional_dependencies"]) {
                        if (child_dep.is_object()) {
                            std::string type = child_dep.value("type", "");
                            if (type == "project" || type == "library") {
                                if (!discover_single_project_dependency(child_dep, cand_root, selected_profile, visited_manifests, discovered_projects, discovery_order)) {
                                    return false;
                                }
                            }
                        }
                    }
                }
            }
        }

        ProjectCandidate cand;
        cand.name = proj_name;
        cand.version = ver;
        cand.manifest_path = target_manifest;
        cand.project_root = cand_root;
        cand.manifest = manifest;
        cand.selected_profile = selected_profile;
        cand.explicit_profile = explicit_profile;

        discovered_projects.push_back(cand);
        if (std::find(discovery_order.begin(), discovery_order.end(), proj_name) == discovery_order.end()) {
            discovery_order.push_back(proj_name);
        }

        return true;
    }

    bool discover_dependencies(const nlohmann::json& manifest,
                               const std::filesystem::path& project_root,
                               const std::string& parent_active_profile,
                               std::set<std::string>& visited_manifests,
                               std::vector<ProjectCandidate>& discovered_projects,
                               std::vector<std::string>& discovery_order) {
        if (!manifest.contains("dependencies") || !manifest["dependencies"].is_array()) {
            return true;
        }

        for (const auto& dep : manifest["dependencies"]) {
            if (!dep.is_object()) continue;
            std::string type = dep.value("type", "");
            if (type == "project" || type == "library") {
                if (!discover_single_project_dependency(dep, project_root, parent_active_profile, visited_manifests, discovered_projects, discovery_order)) {
                    return false;
                }
            }
        }

        return true;
    }

    std::vector<std::unique_ptr<IDependencyResolver>> resolvers_;
};

} // namespace solix::cli
