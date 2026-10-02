#pragma once

#include "test_helper.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <string>

namespace solix::test {

/**
 * @brief Reads all .slx standard library source files from solixlib/project/src
 * to make them available in test compilation units.
 */
inline std::unordered_map<std::string, std::string> load_solixlib_sources() {
    std::unordered_map<std::string, std::string> sources;
    std::filesystem::path lib_dir = std::filesystem::path(SOLIX_PROJECT_ROOT) / "solixlib" / "project" / "src";
    if (std::filesystem::exists(lib_dir)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(lib_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".slx") {
                std::ifstream f(entry.path());
                if (f.is_open()) {
                    std::stringstream ss;
                    ss << f.rdbuf();
                    sources[entry.path().lexically_relative(lib_dir).string()] = ss.str();
                }
            }
        }
    }
    return sources;
}

} // namespace solix::test
