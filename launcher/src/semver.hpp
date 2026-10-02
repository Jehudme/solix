#pragma once

#include <string>
#include <tuple>
#include <optional>
#include <sstream>

namespace solix::cli {

/**
 * @brief Represents a semantic version (MAJOR.MINOR.PATCH) with comparison and compatibility checks.
 */
struct SemVer {
    int major{0};
    int minor{0};
    int patch{0};
    std::string raw;

    static std::optional<SemVer> parse(const std::string& str) {
        if (str.empty()) return std::nullopt;
        std::string s = str;
        if (s[0] == 'v' || s[0] == 'V') s = s.substr(1);

        std::stringstream ss(s);
        std::string part;
        int parts[3] = {0, 0, 0};
        int idx = 0;

        while (std::getline(ss, part, '.')) {
            if (idx >= 3) break;
            try {
                size_t pos = 0;
                parts[idx] = std::stoi(part, &pos);
            } catch (...) {
                return std::nullopt;
            }
            idx++;
        }

        if (idx == 0) return std::nullopt;

        SemVer v;
        v.major = parts[0];
        v.minor = parts[1];
        v.patch = parts[2];
        v.raw = str;
        return v;
    }

    bool operator<(const SemVer& other) const {
        return std::tie(major, minor, patch) < std::tie(other.major, other.minor, other.patch);
    }
    bool operator>(const SemVer& other) const {
        return other < *this;
    }
    bool operator==(const SemVer& other) const {
        return std::tie(major, minor, patch) == std::tie(other.major, other.minor, other.patch);
    }
    bool operator!=(const SemVer& other) const {
        return !(*this == other);
    }

    enum class Compatibility {
        CompatibleSamePatch,
        CompatibleMinorDifference,
        IncompatibleMajorDifference
    };

    static Compatibility assess_compatibility(const SemVer& v1, const SemVer& v2) {
        if (v1.major != v2.major) {
            return Compatibility::IncompatibleMajorDifference;
        }
        if (v1.minor != v2.minor) {
            return Compatibility::CompatibleMinorDifference;
        }
        return Compatibility::CompatibleSamePatch;
    }
};

} // namespace solix::cli
