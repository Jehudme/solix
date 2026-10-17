#include <catch2/catch_all.hpp>
#include "cli_test_helper.hpp"
#include "solix/zip_utils.hpp"
#include <nlohmann/json.hpp>

using namespace solix::test;

#ifndef SOLIX_TEST_PLUGIN_PATH
#define SOLIX_TEST_PLUGIN_PATH ""
#endif

static std::string create_bundle_manifest(const std::string& name = "bundle_app") {
    nlohmann::json manifest;
    manifest["name"] = name;
    manifest["version"] = "1.0.0";
    manifest["dependencies"] = nlohmann::json::array({
        {
            {"type", "source"},
            {"path", "src/main.slx"}
        }
    });

    auto make_profile = [](const std::string& prof_name) {
        nlohmann::json p;
        p["output_directory"] = "build/" + prof_name;
        p["exe_filename"] = "out.slxbin";
        p["compilation"] = {
            {"entry_point", "main"},
            {"multithreaded", false}
        };
        return p;
    };

    manifest["profiles"]["debug"] = make_profile("debug");
    manifest["profiles"]["release"] = make_profile("release");
    return manifest.dump(2);
}

TEST_CASE("CLI Command - run bundle (.slxpac)", "[command][run][bundle]") {
    TempDir sandbox;

    SECTION("Positive - Case 20.6: Direct .slxpac Bundle Execution") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        sandbox.create_file("manifest.json", create_bundle_manifest("app_20_6"));
        sandbox.create_file("src/main.slx", "class Main { public static int32 main() { return 0; } }\n");

        auto pack_res = run_cli({"pack", "-o", "app.slxpac"});
        REQUIRE(pack_res.exit_code == 0);
        REQUIRE(sandbox.exists("app.slxpac"));

        // Execute package directly
        auto run_res = run_cli({"run", "app.slxpac"});
        std::filesystem::current_path(orig_cwd);

        CHECK(run_res.exit_code == 0);
    }

    SECTION("Positive - Case 20.7: Bundle Execution with Arguments") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        sandbox.create_file("manifest.json", create_bundle_manifest("args_app"));
        sandbox.create_file("src/main.slx", R"(
            class Main {
                public static int32 main(char[][] args) {
                    if (args != null && args.length == 2) {
                        return 0;
                    }
                    return 1;
                }
            }
        )");

        auto pack_res = run_cli({"pack", "-o", "args_app.slxpac"});
        REQUIRE(pack_res.exit_code == 0);
        REQUIRE(sandbox.exists("args_app.slxpac"));

        // Run with two arguments
        auto run_res = run_cli({"run", "args_app.slxpac", "arg1", "arg2"});
        std::filesystem::current_path(orig_cwd);

        CHECK(run_res.exit_code == 0);
    }

    SECTION("Positive - Case 20.8: Bundle Execution with Native Shared Libraries") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        std::filesystem::path plugin_src(SOLIX_TEST_PLUGIN_PATH);
        if (std::filesystem::exists(plugin_src)) {
            sandbox.create_file("manifest.json", create_bundle_manifest("native_bundle_app"));
            sandbox.create_file("src/main.slx", R"(
                public class NativeMath {
                    public static native int32 add(int32 a, int32 b);
                }

                public class Main {
                    public static int32 main() {
                        return NativeMath.add(15, 27) == 42 ? 0 : 1;
                    }
                }
            )");

            // Copy native plugin into lib/
            auto lib_dir = sandbox.path() / "lib";
            std::filesystem::create_directories(lib_dir);
            std::filesystem::copy_file(plugin_src, lib_dir / plugin_src.filename(),
                                       std::filesystem::copy_options::overwrite_existing);

            auto pack_res = run_cli({"pack", "-o", "native_app.slxpac"});
            REQUIRE(pack_res.exit_code == 0);

            auto run_res = run_cli({"run", "native_app.slxpac"});
            CHECK(run_res.exit_code == 0);
        }
        std::filesystem::current_path(orig_cwd);
    }

    SECTION("Negative - Case 20.9: Corrupted or Truncated .slxpac Archive") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        sandbox.create_file("corrupt.slxpac", "THIS_IS_NOT_A_VALID_ZIP_FILE");

        auto run_res = run_cli({"run", "corrupt.slxpac"});
        std::filesystem::current_path(orig_cwd);

        CHECK(run_res.exit_code != 0);
    }

    SECTION("Negative - Case 20.10: Bundle Missing bytecode.slxbin Entry Point") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        // Create zip archive with manifest but missing bytecode
        solix::ZipArchiveWriter writer(sandbox.path() / "no_bytecode.slxpac");
        writer.add_string("manifest.json", "{\"name\": \"empty\"}");
        writer.finalize();

        auto run_res = run_cli({"run", "no_bytecode.slxpac"});
        std::filesystem::current_path(orig_cwd);

        CHECK(run_res.exit_code != 0);
    }
}
