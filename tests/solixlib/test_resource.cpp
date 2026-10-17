#include "solixlib_test_helper.hpp"
#include <filesystem>
#include <fstream>
#include <chrono>

using namespace solix::test;

namespace {

struct ResourceEnvGuard {
    std::filesystem::path temp_dir;

    ResourceEnvGuard() {
        temp_dir = std::filesystem::temp_directory_path() / ("solix_res_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(temp_dir);
        std::filesystem::create_directories(temp_dir / "assets");
        {
            std::ofstream f(temp_dir / "config.txt", std::ios::binary);
            f << "database_url=localhost\nport=8080";
        }
        {
            std::ofstream f(temp_dir / "assets" / "banner.txt", std::ios::binary);
            f << "=== Welcome to Solix ===";
        }
#ifdef _WIN32
        _putenv_s("SOLIX_RESOURCE_ROOT", temp_dir.string().c_str());
#else
        setenv("SOLIX_RESOURCE_ROOT", temp_dir.string().c_str(), 1);
#endif
    }

    ~ResourceEnvGuard() {
#ifdef _WIN32
        _putenv_s("SOLIX_RESOURCE_ROOT", "");
#else
        unsetenv("SOLIX_RESOURCE_ROOT");
#endif
        std::error_code ec;
        std::filesystem::remove_all(temp_dir, ec);
    }
};

} // namespace

TEST_CASE("Standard Library - solix.io.filesystem.Resource", "[solixlib][io][resource]") {
    ResourceEnvGuard guard;

    SECTION("Case 16.1: Resource inspection and path resolution") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.io.filesystem.Resource;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    String root = Resource.get_resource_directory();
                    if (root.length() == 0) return 1;

                    String rel = new String("config.txt");
                    String full = Resource.get_path(rel);
                    if (!full.contains(rel)) return 2;

                    if (!Resource.exists(rel)) return 3;

                    String banner_rel = new String("assets/banner.txt");
                    if (!Resource.exists(banner_rel)) return 4;

                    String non_existent = new String("does_not_exist.txt");
                    if (Resource.exists(non_existent)) return 5;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 16.2: Reading bundled resource text and lines") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.io.filesystem.Resource;
            import solix.collections.List;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    String banner_rel = new String("assets/banner.txt");
                    String banner_text = Resource.read_all_text(banner_rel);
                    if (!banner_text.equals_chars("=== Welcome to Solix ===")) return 1;

                    String config_rel = new String("config.txt");
                    List<String> lines = Resource.read_all_lines(config_rel);
                    if (lines.size() != 2) return 2;
                    if (!lines.get(0).equals_chars("database_url=localhost")) return 3;
                    if (!lines.get(1).equals_chars("port=8080")) return 4;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 16.3: Dual-location File methods and res:// URI resolution") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.io.filesystem.File;
            import solix.io.filesystem.Path;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    String rel = new String("config.txt");

                    // 1. Dual-location parameter overload
                    if (!File.exists(rel, true)) return 1;
                    String text_from_res = File.read_all_text(rel, true);
                    if (!text_from_res.starts_with(new String("database_url="))) return 2;

                    // 2. Named resource convenience methods
                    if (!File.resource_exists(rel)) return 3;
                    String text_named = File.read_resource_text(rel);
                    if (!text_named.equals(text_from_res)) return 4;

                    // 3. Virtual URI scheme res://
                    String uri = new String("res://config.txt");
                    if (!Path.is_resource_path(uri)) return 5;

                    Path p = new Path(uri);
                    if (!p.is_resource()) return 6;

                    if (!File.exists(uri)) return 7;
                    String text_uri = File.read_all_text(uri);
                    if (!text_uri.equals(text_from_res)) return 8;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 16.4: Negative: Reading non-existent resource throws FileNotFoundException") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.io.filesystem.Resource;
            import solix.io.filesystem.File;
            import solix.core.String;
            import solix.exceptions.FileNotFoundException;

            class Main {
                public static int32 main() {
                    String missing = new String("missing_resource.txt");
                    bool caught_res = false;
                    try {
                        Resource.read_all_text(missing);
                    } catch (FileNotFoundException ex) {
                        caught_res = true;
                    }
                    if (!caught_res) return 1;

                    String missing_uri = new String("res://missing_file.bin");
                    bool caught_uri = false;
                    try {
                        File.read_all_text(missing_uri);
                    } catch (FileNotFoundException ex) {
                        caught_uri = true;
                    }
                    if (!caught_uri) return 2;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }
}
