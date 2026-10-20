#include "solix/lsp/protocol.hpp"
#include <filesystem>
#include <algorithm>

namespace solix::lsp {

std::string uri_to_path(const std::string& uri) {
    std::string prefix = "file://";
    if (uri.rfind(prefix, 0) == 0) {
        std::string path_part = uri.substr(prefix.length());
#if defined(_WIN32)
        // If URI starts with file:///C:/path, substr(7) is /C:/path -> trim leading slash
        if (path_part.size() >= 3 && path_part[0] == '/' && path_part[2] == ':') {
            path_part = path_part.substr(1);
        }
#endif
        return std::filesystem::path(path_part).lexically_normal().string();
    }
    return std::filesystem::path(uri).lexically_normal().string();
}

std::string path_to_uri(const std::string& path) {
    if (path.empty()) {
        return "";
    }
    std::string generic = std::filesystem::path(path).lexically_normal().generic_string();
    if (!generic.empty() && generic[0] != '/') {
        generic = "/" + generic;
    }
    return "file://" + generic;
}

} // namespace solix::lsp
