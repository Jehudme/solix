#pragma once
#include <filesystem>
#include <string>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifdef ERROR
#undef ERROR
#endif
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#include <stdlib.h>
#else // Linux and other POSIX
#include <unistd.h>
#include <limits.h>
#endif

namespace solix {

/**
 * @brief Cross-platform resolution of the current executable binary path.
 * Supports Linux (/proc/self/exe), macOS (_NSGetExecutablePath), and Windows (GetModuleFileNameW).
 */
inline std::filesystem::path get_executable_path() {
#if defined(_WIN32) || defined(_WIN64)
    std::vector<wchar_t> buffer(MAX_PATH);
    while (true) {
        DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (size == 0) {
            return std::filesystem::current_path();
        }
        if (size < buffer.size()) {
            return std::filesystem::path(buffer.data());
        }
        buffer.resize(buffer.size() * 2);
    }
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size + 1, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
        char resolved[PATH_MAX];
        if (realpath(buffer.data(), resolved) != nullptr) {
            return std::filesystem::path(resolved);
        }
        return std::filesystem::path(buffer.data());
    }
    return std::filesystem::current_path();
#else // Linux / POSIX
    char buffer[PATH_MAX];
    ssize_t len = ::readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return std::filesystem::path(buffer);
    }
    return std::filesystem::current_path();
#endif
}

/**
 * @brief Returns the directory containing the current executable binary.
 */
inline std::filesystem::path get_executable_dir() {
    return get_executable_path().parent_path();
}

/**
 * @brief Finds the project manifest file in a given directory or directly validates a file path.
 * Prefers 'manifest.json' (new standard), falling back to 'solix.json' (legacy compatibility).
 */
inline std::filesystem::path find_project_manifest(const std::filesystem::path& path_or_dir) {
    if (std::filesystem::is_regular_file(path_or_dir)) {
        return path_or_dir.lexically_normal();
    }
    std::filesystem::path manifest_json = path_or_dir / "manifest.json";
    if (std::filesystem::exists(manifest_json)) {
        return manifest_json.lexically_normal();
    }
    std::filesystem::path solix_json = path_or_dir / "solix.json";
    if (std::filesystem::exists(solix_json)) {
        return solix_json.lexically_normal();
    }
    return manifest_json.lexically_normal();
}

} // namespace solix
