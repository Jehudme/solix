#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "solix/compilation.hpp"

namespace solix::cli {

/**
 * @brief Extensible interface for resolving different project dependency types.
 */
class IDependencyResolver {
public:
    virtual ~IDependencyResolver() = default;

    /**
     * @brief Checks if this resolver handles the specified dependency type.
     */
    virtual bool can_resolve(const std::string& type) const = 0;

    /**
     * @brief Resolves the dependency, loading necessary source code or modules into CompilationOptions.
     * @param dep_node The JSON object describing the dependency entry.
     * @param project_root The root directory of the project containing solix.json.
     * @param opts The compilation options being populated for the build.
     * @return true on success, false on error.
     */
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
            std::cerr << "Error: Source dependency file does not exist: " << file_path.string() << std::endl;
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
 * @brief Coordinates dependency resolution across all registered resolvers.
 */
class DependencyManager {
public:
    DependencyManager() {
        // Register default resolvers
        register_resolver(std::make_unique<SourceDependencyResolver>());
        // Future resolvers (e.g. LibraryDependencyResolver, PhysicalDependencyResolver)
        // can be registered here or injected via register_resolver().
    }

    void register_resolver(std::unique_ptr<IDependencyResolver> resolver) {
        resolvers_.push_back(std::move(resolver));
    }

    bool resolve_dependency(const nlohmann::json& dep_node,
                            const std::filesystem::path& project_root,
                            CompilationOptions& opts) {
        if (!dep_node.is_object() || !dep_node.contains("type") || !dep_node["type"].is_string()) {
            std::cerr << "Error: Dependency entry must be an object with a 'type' string field." << std::endl;
            return false;
        }

        std::string type = dep_node["type"].get<std::string>();
        for (const auto& resolver : resolvers_) {
            if (resolver->can_resolve(type)) {
                return resolver->resolve(dep_node, project_root, opts);
            }
        }

        std::cerr << "Error: Unsupported dependency type '" << type << "'." << std::endl;
        return false;
    }

private:
    std::vector<std::unique_ptr<IDependencyResolver>> resolvers_;
};

} // namespace solix::cli
