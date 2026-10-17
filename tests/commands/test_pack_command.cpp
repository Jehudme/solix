#include <catch2/catch_all.hpp>
#include "cli_test_helper.hpp"
#include "solix/zip_utils.hpp"
#include <nlohmann/json.hpp>

using namespace solix::test;

static std::string create_sample_manifest(const std::string& name = "sample_app") {
    nlohmann::json manifest;
    manifest["name"] = name;
    manifest["version"] = "1.2.0";
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

TEST_CASE("CLI Command - pack", "[command][pack]") {
    TempDir sandbox;

    SECTION("Positive - Case 20.1: Default Project Packaging to .slxpac") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        sandbox.create_file("manifest.json", create_sample_manifest("my_app"));
        sandbox.create_file("src/main.slx", "class Main { public static int32 main() { return 0; } }\n");
        sandbox.create_file("resources/config.json", "{\"title\": \"Test App\"}\n");

        auto res = run_cli({"pack"});
        std::filesystem::current_path(orig_cwd);

        CHECK(res.exit_code == 0);

        // Default pack output is in build/release/my_app.slxpac
        auto expected_pac = sandbox.path() / "build" / "release" / "my_app.slxpac";
        CHECK(std::filesystem::exists(expected_pac));
        CHECK(solix::is_zip_archive(expected_pac));

        // Verify internal contents using ZipArchiveReader
        solix::ZipArchiveReader reader(expected_pac);
        CHECK(reader.is_valid());
        CHECK(reader.has_entry("bytecode.slxbin"));
        CHECK(reader.has_entry("manifest.json"));
        CHECK(reader.has_entry("resources/config.json"));

        auto config_str = reader.read_entry_as_string("resources/config.json");
        REQUIRE(config_str.has_value());
        CHECK(config_str->find("Test App") != std::string::npos);

        auto manifest_str = reader.read_entry_as_string("manifest.json");
        REQUIRE(manifest_str.has_value());
        auto bundle_json = nlohmann::json::parse(*manifest_str);
        CHECK(bundle_json["name"] == "my_app");
        CHECK(bundle_json["version"] == "1.2.0");
        CHECK(bundle_json["entry"] == "bytecode.slxbin");
    }

    SECTION("Positive - Case 20.2: Custom Output Path and Profile") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        sandbox.create_file("manifest.json", create_sample_manifest("custom_app"));
        sandbox.create_file("src/main.slx", "class Main { public static int32 main() { return 42; } }\n");

        auto res = run_cli({"pack", "-p", "debug", "-o", "dist/app_bundle.slxpac"});
        std::filesystem::current_path(orig_cwd);

        CHECK(res.exit_code == 0);

        auto custom_pac = sandbox.path() / "dist" / "app_bundle.slxpac";
        CHECK(std::filesystem::exists(custom_pac));
        CHECK(solix::is_zip_archive(custom_pac));

        solix::ZipArchiveReader reader(custom_pac);
        CHECK(reader.is_valid());
        CHECK(reader.has_entry("bytecode.slxbin"));
        CHECK(reader.has_entry("manifest.json"));
    }

    SECTION("Positive - Case 20.3: Packaging Project with Native Libraries") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        sandbox.create_file("manifest.json", create_sample_manifest("native_app"));
        sandbox.create_file("src/main.slx", "class Main { public static int32 main() { return 0; } }\n");
        sandbox.create_file("lib/mock_plugin.so", "MOCK_SHARED_LIB_DATA");
        sandbox.create_file("resources/asset.txt", "hello asset\n");

        auto res = run_cli({"pack"});
        std::filesystem::current_path(orig_cwd);

        CHECK(res.exit_code == 0);

        auto expected_pac = sandbox.path() / "build" / "release" / "native_app.slxpac";
        CHECK(std::filesystem::exists(expected_pac));

        solix::ZipArchiveReader reader(expected_pac);
        CHECK(reader.is_valid());
        CHECK(reader.has_entry("bytecode.slxbin"));
        CHECK(reader.has_entry("lib/mock_plugin.so"));
        CHECK(reader.has_entry("resources/asset.txt"));

        auto lib_content = reader.read_entry_as_string("lib/mock_plugin.so");
        REQUIRE(lib_content.has_value());
        CHECK(*lib_content == "MOCK_SHARED_LIB_DATA");
    }

    SECTION("Negative - Case 20.4: Packaging Non-Existent Project or Missing Manifest") {
        auto non_existent = sandbox.path() / "does_not_exist";
        auto res = run_cli({"pack", non_existent.string()});
        CHECK(res.exit_code != 0);
    }

    SECTION("Negative - Case 20.5: Packaging Project with Compilation Errors") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        sandbox.create_file("manifest.json", create_sample_manifest("broken_app"));
        sandbox.create_file("src/main.slx", "class Main { broken syntax error here! }\n");

        auto res = run_cli({"pack", "-o", "broken.slxpac"});
        std::filesystem::current_path(orig_cwd);

        CHECK(res.exit_code != 0);
        CHECK(!sandbox.exists("broken.slxpac"));
    }

    SECTION("Positive - Case 20.11: Multi-Depth Dependency Staging in Build") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        // Setup multi-depth dependency tree: app -> lib_a -> lib_b
        sandbox.create_file("lib_b/manifest.json", R"({
            "name": "lib_b",
            "project": "lib_b",
            "version": "1.0.0",
            "type": "library",
            "dependencies": [
                { "type": "source", "path": "src/b.slx" }
            ]
        })");
        sandbox.create_file("lib_b/src/b.slx", "class HelperB { public static int32 get_b() { return 10; } }\n");
        sandbox.create_file("lib_b/lib/lib_b_native.so", "NATIVE_LIB_B_BYTES");
        sandbox.create_file("lib_b/resources/b_data.txt", "from lib_b\n");
        sandbox.create_file("lib_b/resources/shared.txt", "from lib_b shared\n");

        sandbox.create_file("lib_a/manifest.json", R"({
            "name": "lib_a",
            "project": "lib_a",
            "version": "1.0.0",
            "type": "library",
            "dependencies": [
                { "type": "source", "path": "src/a.slx" },
                { "type": "project", "path": "../lib_b" }
            ]
        })");
        sandbox.create_file("lib_a/src/a.slx", "class HelperA { public static int32 get_a() { return HelperB.get_b() + 5; } }\n");
        sandbox.create_file("lib_a/resources/a_data.txt", "from lib_a\n");

        sandbox.create_file("app/manifest.json", R"({
            "name": "app",
            "project": "app",
            "version": "1.0.0",
            "dependencies": [
                { "type": "source", "path": "src/main.slx" },
                { "type": "project", "path": "../lib_a" }
            ],
            "profiles": {
                "debug": {
                    "output_directory": "build/debug",
                    "exe_filename": "out.slxbin",
                    "compilation": {
                        "entry_point": "main",
                        "multithreaded": false
                    }
                }
            }
        })");
        sandbox.create_file("app/src/main.slx", "class Main { public static int32 main() { return HelperA.get_a() == 15 ? 0 : 1; } }\n");
        sandbox.create_file("app/resources/app_data.txt", "from app\n");
        sandbox.create_file("app/resources/shared.txt", "overridden by app\n");

        // Run solix build in app
        std::filesystem::current_path(sandbox.path() / "app");
        auto build_res = run_cli({"build"});
        std::filesystem::current_path(orig_cwd);

        CHECK(build_res.exit_code == 0);

        auto app_build_dir = sandbox.path() / "app" / "build" / "debug";
        CHECK(std::filesystem::exists(app_build_dir / "out.slxbin"));

        // Verify native libraries staged from transitive dependency lib_b
        CHECK(std::filesystem::exists(app_build_dir / "lib" / "lib_b_native.so"));

        // Verify resources staged from all dependency depths
        CHECK(std::filesystem::exists(app_build_dir / "resources" / "b_data.txt"));
        CHECK(std::filesystem::exists(app_build_dir / "resources" / "a_data.txt"));
        CHECK(std::filesystem::exists(app_build_dir / "resources" / "app_data.txt"));
        CHECK(std::filesystem::exists(app_build_dir / "resources" / "shared.txt"));

        // Verify collision override: app overrides lib_b
        std::ifstream shared_f(app_build_dir / "resources" / "shared.txt");
        std::string shared_content((std::istreambuf_iterator<char>(shared_f)), std::istreambuf_iterator<char>());
        CHECK(shared_content.find("overridden by app") != std::string::npos);
    }

    SECTION("Positive - Case 20.12: Multi-Depth Dependency Packaging and Execution") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        std::filesystem::path plugin_src(SOLIX_TEST_PLUGIN_PATH);
        std::string lib_filename = plugin_src.filename().string();

        // Setup multi-depth dependency tree: multi_app -> multi_lib_a -> multi_lib_b
        sandbox.create_file("multi_b/manifest.json", R"({
            "name": "multi_b",
            "project": "multi_b",
            "version": "1.0.0",
            "type": "library",
            "dependencies": [
                { "type": "source", "path": "src/b.slx" }
            ]
        })");
        sandbox.create_file("multi_b/src/b.slx", R"(
            public class NativeMath {
                public static native int32 add(int32 a, int32 b);
            }
            class DepB {
                public static int32 val() { return NativeMath.add(10, 10); }
            }
        )");

        if (std::filesystem::exists(plugin_src)) {
            std::filesystem::create_directories(sandbox.path() / "multi_b" / "lib");
            std::filesystem::copy_file(plugin_src, sandbox.path() / "multi_b" / "lib" / lib_filename,
                                       std::filesystem::copy_options::overwrite_existing);
        }

        sandbox.create_file("multi_b/resources/b_res.txt", "resource from b\n");

        sandbox.create_file("multi_a/manifest.json", R"({
            "name": "multi_a",
            "project": "multi_a",
            "version": "1.0.0",
            "type": "library",
            "dependencies": [
                { "type": "source", "path": "src/a.slx" },
                { "type": "project", "path": "../multi_b" }
            ]
        })");
        sandbox.create_file("multi_a/src/a.slx", "class DepA { public static int32 val() { return DepB.val() + 10; } }\n");
        sandbox.create_file("multi_a/resources/a_res.txt", "resource from a\n");

        sandbox.create_file("multi_app/manifest.json", R"({
            "name": "multi_app",
            "project": "multi_app",
            "version": "1.0.0",
            "dependencies": [
                { "type": "source", "path": "src/main.slx" },
                { "type": "project", "path": "../multi_a" }
            ],
            "profiles": {
                "debug": {
                    "output_directory": "build/debug",
                    "exe_filename": "out.slxbin",
                    "compilation": {
                        "entry_point": "main",
                        "multithreaded": false
                    }
                },
                "release": {
                    "output_directory": "build/release",
                    "exe_filename": "out.slxbin",
                    "compilation": {
                        "entry_point": "main",
                        "multithreaded": false
                    }
                }
            }
        })");
        sandbox.create_file("multi_app/src/main.slx", "class Main { public static int32 main() { return DepA.val() == 30 ? 0 : 1; } }\n");
        sandbox.create_file("multi_app/resources/app_res.txt", "resource from app\n");

        // Run solix pack in multi_app
        std::filesystem::current_path(sandbox.path() / "multi_app");
        auto pack_res = run_cli({"pack", "-o", "multi.slxpac"});
        CHECK(pack_res.exit_code == 0);

        auto pac_path = sandbox.path() / "multi_app" / "multi.slxpac";
        CHECK(std::filesystem::exists(pac_path));

        // Verify package contents contains transitive native libs and resources
        solix::ZipArchiveReader reader(pac_path);
        CHECK(reader.is_valid());
        CHECK(reader.has_entry("bytecode.slxbin"));
        CHECK(reader.has_entry("manifest.json"));
        if (std::filesystem::exists(plugin_src)) {
            CHECK(reader.has_entry("lib/" + lib_filename));
        }
        CHECK(reader.has_entry("resources/b_res.txt"));
        CHECK(reader.has_entry("resources/a_res.txt"));
        CHECK(reader.has_entry("resources/app_res.txt"));

        // Execute bundle with solix run
        auto run_res = run_cli({"run", pac_path.string()});
        std::filesystem::current_path(orig_cwd);

        CHECK(run_res.exit_code == 0);
    }

    SECTION("Positive - Case 21.6: Pre-Compile Command Generating Native Library Bundled by solix pack") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        std::filesystem::path plugin_src(SOLIX_TEST_PLUGIN_PATH);
        std::string lib_filename = plugin_src.filename().string();

        nlohmann::json manifest;
        manifest["name"] = "precompile_pack_app";
        manifest["project"] = "precompile_pack_app";
        manifest["version"] = "1.0.0";
        manifest["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/main.slx"}}
        });

        // Pre-compile command creates lib/ and copies the plugin library into lib/
        std::string copy_cmd;
#ifdef _WIN32
        copy_cmd = "cmd /c if not exist lib mkdir lib && copy \"" + plugin_src.string() + "\" \"lib\\" + lib_filename + "\"";
#else
        copy_cmd = "sh -c 'mkdir -p lib && cp \"" + plugin_src.string() + "\" \"lib/" + lib_filename + "\"'";
#endif
        manifest["pre_compile"] = nlohmann::json::array({copy_cmd});

        manifest["profiles"]["debug"] = {
            {"output_directory", "build/debug"},
            {"exe_filename", "out.slxbin"},
            {"compilation", {{"entry_point", "main"}, {"multithreaded", false}}}
        };
        manifest["profiles"]["release"] = {
            {"output_directory", "build/release"},
            {"exe_filename", "out.slxbin"},
            {"compilation", {{"entry_point", "main"}, {"multithreaded", false}}}
        };

        sandbox.create_file("manifest.json", manifest.dump(2));
        sandbox.create_file("src/main.slx", R"(
            public class NativeMath {
                public static native int32 add(int32 a, int32 b);
            }
            public class Main {
                public static int32 main() {
                    return NativeMath.add(10, 32) == 42 ? 0 : 1;
                }
            }
        )");

        auto pack_res = run_cli({"pack", "-o", "precompile_app.slxpac"});
        CHECK(pack_res.exit_code == 0);

        auto pac_path = sandbox.path() / "precompile_app.slxpac";
        CHECK(std::filesystem::exists(pac_path));

        solix::ZipArchiveReader reader(pac_path);
        CHECK(reader.is_valid());
        CHECK(reader.has_entry("bytecode.slxbin"));
        if (std::filesystem::exists(plugin_src)) {
            CHECK(reader.has_entry("lib/" + lib_filename));
        }

        auto run_res = run_cli({"run", pac_path.string()});
        std::filesystem::current_path(orig_cwd);

        CHECK(run_res.exit_code == 0);
    }
}
