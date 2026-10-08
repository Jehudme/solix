#include <catch2/catch_test_macros.hpp>
#include "package_manager.hpp"
#include "dependency_resolver.hpp"
#include "cli_test_helper.hpp"
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <string>

using namespace solix::cli;
using namespace solix::test;

TEST_CASE("Suite 18: Toolchain solixlib Auto-Discovery & IDE Integration Command", "[ide][autodiscovery]") {
    SECTION("Case 18.1: Bundled solixlib Relative Path Auto-Discovery") {
        auto bundled_opt = PackageManager::discover_bundled_solixlib();
        REQUIRE(bundled_opt.has_value());
        REQUIRE(std::filesystem::exists(bundled_opt.value() / "solix.json"));
        REQUIRE(std::filesystem::exists(bundled_opt.value() / "src"));
    }

    SECTION("Case 18.2: Automatic Standard Library Seeding on Missing Dependency") {
        std::filesystem::path temp_home = std::filesystem::temp_directory_path() / ("solix_home_auto_seed_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::error_code ec;
        std::filesystem::create_directories(temp_home, ec);

#if defined(_WIN32)
        _putenv_s("SOLIX_HOME", temp_home.string().c_str());
#else
        setenv("SOLIX_HOME", temp_home.string().c_str(), 1);
#endif

        PackageManager pm;
        REQUIRE(pm.list_installed_projects().empty());

        // Resolve dependency requesting solixlib
        DependencyManager dm;
        nlohmann::json manifest = {
            {"project", "test_app"},
            {"version", "0.1.0"},
            {"dependencies", {
                {{"type", "project"}, {"name", "solixlib"}}
            }}
        };

        solix::CompilationOptions opts;
        opts.log_level = solix::CompilationOptions::LogLevel::OFF;
        opts.flush_level = solix::CompilationOptions::LogLevel::OFF;

        bool resolved = dm.resolve_all(manifest, temp_home, nlohmann::json::object(), opts);
        REQUIRE(resolved);

        // Registry should now contain installed solixlib
        auto installed = pm.get_projects_by_name("solixlib");
        REQUIRE_FALSE(installed.empty());

#if defined(_WIN32)
        _putenv_s("SOLIX_HOME", "");
#else
        unsetenv("SOLIX_HOME");
#endif
        std::filesystem::remove_all(temp_home, ec);
    }

    SECTION("Case 18.3: solix ide install Execution and Help Inspection") {
        auto res = run_cli({"ide", "--help"});
        REQUIRE(res.exit_code == 0);
        REQUIRE(res.out.find("install") != std::string::npos);

        auto res2 = run_cli({"ide", "install", "--help"});
        REQUIRE(res2.exit_code == 0);
        REQUIRE(res2.out.find("--editor") != std::string::npos);
        REQUIRE(res2.out.find("cursor") != std::string::npos);
    }

    SECTION("Case 18.4: solix ide install with Unknown or Missing Editor") {
        auto res = run_cli({"ide", "install", "--editor", "non_existent_editor_binary_xyz"});
        // Should exit with non-zero code due to missing editor binary
        REQUIRE(res.exit_code != 0);
        REQUIRE(res.err.find("non_existent_editor_binary_xyz") != std::string::npos);
    }
}
