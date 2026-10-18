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
        std::string win_plugin_path = plugin_src.string();
        std::replace(win_plugin_path.begin(), win_plugin_path.end(), '/', '\\');
        copy_cmd = "cmd /c if not exist lib mkdir lib && copy /Y \"" + win_plugin_path + "\" \"lib\\" + lib_filename + "\"";
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

    SECTION("Positive - Case 23.1: End-to-End Native Compilation, Transitive Staging, Packaging, and Execution") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::path plugin_src(SOLIX_TEST_PLUGIN_PATH);
        std::string lib_filename = plugin_src.filename().string();

        auto log_file_path = (sandbox.path() / "lifecycle.log").lexically_normal();

        // 1. Setup lib_a: has "release" profile that copies the native plugin into lib_a/lib/
        // and appends LIB_A_PRE to lifecycle.log
        auto lib_a_dir = sandbox.path() / "lib_a";
        std::filesystem::create_directories(lib_a_dir / "src");
        std::filesystem::create_directories(lib_a_dir / "resources");

        nlohmann::json lib_a_manifest;
        lib_a_manifest["project"] = "lib_a";
        lib_a_manifest["version"] = "1.0.0";
        lib_a_manifest["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/math.slx"}}
        });

        std::string lib_a_pre_copy;
        std::string lib_a_pre_log;
#if defined(_WIN32)
        std::string win_plugin_path = plugin_src.string();
        std::replace(win_plugin_path.begin(), win_plugin_path.end(), '/', '\\');
        lib_a_pre_copy = "cmd /c if not exist lib mkdir lib && copy /Y \"" + win_plugin_path + "\" lib\\";
        lib_a_pre_log = "cmd /c echo LIB_A_PRE>> \"" + log_file_path.string() + "\"";
#else
        lib_a_pre_copy = "sh -c 'mkdir -p lib && cp \"" + plugin_src.string() + "\" lib/'";
        lib_a_pre_log = "sh -c 'echo LIB_A_PRE >> \"" + log_file_path.string() + "\"'";
#endif

        lib_a_manifest["profiles"] = {
            {"debug", {
                {"output_directory", "build/debug"},
                {"compilation", {{"entry_point", "dummy"}}}
            }},
            {"release", {
                {"output_directory", "build/release"},
                {"pre_compile", nlohmann::json::array({lib_a_pre_copy, lib_a_pre_log})},
                {"compilation", {{"entry_point", "dummy"}}}
            }}
        };

        sandbox.create_file("lib_a/manifest.json", lib_a_manifest.dump(2));
        sandbox.create_file("lib_a/resources/lib_data.txt", "lib_a_resource_content\n");
        sandbox.create_file("lib_a/src/math.slx", R"(
            public class NativeMath {
                public static native int32 add(int32 a, int32 b);
            }
            public class LibMath {
                public static int32 compute(int32 x) {
                    return NativeMath.add(x, 10);
                }
            }
        )");

        // 2. Setup app: depends on lib_a with profile "release"
        auto app_dir = sandbox.path() / "app";
        std::filesystem::create_directories(app_dir / "src");
        std::filesystem::create_directories(app_dir / "resources");

        std::string app_root_pre_cmd;
        std::string app_prof_pre_cmd;
        std::string app_prof_post_cmd;
        std::string app_root_post_cmd;
#if defined(_WIN32)
        app_root_pre_cmd = "cmd /c echo APP_ROOT_PRE>> \"" + log_file_path.string() + "\"";
        app_prof_pre_cmd = "cmd /c echo APP_PROF_PRE>> \"" + log_file_path.string() + "\"";
        app_prof_post_cmd = "cmd /c echo APP_PROF_POST>> \"" + log_file_path.string() + "\"";
        app_root_post_cmd = "cmd /c echo APP_ROOT_POST>> \"" + log_file_path.string() + "\"";
#else
        app_root_pre_cmd = "sh -c 'echo APP_ROOT_PRE >> \"" + log_file_path.string() + "\"'";
        app_prof_pre_cmd = "sh -c 'echo APP_PROF_PRE >> \"" + log_file_path.string() + "\"'";
        app_prof_post_cmd = "sh -c 'echo APP_PROF_POST >> \"" + log_file_path.string() + "\"'";
        app_root_post_cmd = "sh -c 'echo APP_ROOT_POST >> \"" + log_file_path.string() + "\"'";
#endif

        nlohmann::json app_manifest;
        app_manifest["project"] = "app";
        app_manifest["name"] = "e2e_app";
        app_manifest["version"] = "1.0.0";
        app_manifest["pre_compile"] = nlohmann::json::array({app_root_pre_cmd});
        app_manifest["post_compile"] = nlohmann::json::array({app_root_post_cmd});
        app_manifest["dependencies"] = nlohmann::json::array({
            {
                {"type", "project"},
                {"path", "../lib_a"},
                {"profile", "release"}
            },
            {
                {"type", "source"},
                {"path", "src/main.slx"}
            }
        });
        app_manifest["profiles"] = {
            {"release", {
                {"output_directory", "build/release"},
                {"exe_filename", "out.slxbin"},
                {"pre_compile", nlohmann::json::array({app_prof_pre_cmd})},
                {"post_compile", nlohmann::json::array({app_prof_post_cmd})},
                {"compilation", {
                    {"entry_point", "main"},
                    {"multithreaded", false}
                }}
            }}
        };

        sandbox.create_file("app/manifest.json", app_manifest.dump(2));
        sandbox.create_file("app/resources/app_data.txt", "app_resource_content\n");
        sandbox.create_file("app/src/main.slx", R"(
            public class Main {
                public static int32 main() {
                    int32 res = LibMath.compute(32);
                    return res == 42 ? 0 : 1;
                }
            }
        )");

        // 3. Run solix pack in app with profile "release"
        std::filesystem::current_path(app_dir);
        auto pack_res = run_cli({"pack", "-p", "release", "-o", "bundle.slxpac"});
        CHECK(pack_res.exit_code == 0);

        auto pac_path = app_dir / "bundle.slxpac";
        CHECK(std::filesystem::exists(pac_path));

        // 4. Verify bundle contents
        solix::ZipArchiveReader reader(pac_path);
        CHECK(reader.is_valid());
        CHECK(reader.has_entry("bytecode.slxbin"));
        CHECK(reader.has_entry("manifest.json"));
        CHECK(reader.has_entry("resources/app_data.txt"));
        CHECK(reader.has_entry("resources/lib_data.txt"));
        if (std::filesystem::exists(plugin_src)) {
            CHECK(reader.has_entry("lib/" + lib_filename));
        }

        // 5. Verify lifecycle execution order
        CHECK(std::filesystem::exists(log_file_path));
        std::ifstream lf(log_file_path);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(lf, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (!line.empty()) {
                lines.push_back(line);
            }
        }

        REQUIRE(lines.size() >= 5);
        CHECK(lines[0] == "LIB_A_PRE");
        CHECK(lines[1] == "APP_ROOT_PRE");
        CHECK(lines[2] == "APP_PROF_PRE");
        CHECK(lines[3] == "APP_PROF_POST");
        CHECK(lines[4] == "APP_ROOT_POST");

        // 6. Execute packaged bundle and verify native call and exit code
        auto run_res = run_cli({"run", pac_path.string()});
        std::filesystem::current_path(orig_cwd);

        CHECK(run_res.exit_code == 0);
    }
}

TEST_CASE("Universal Multi-Platform Cross-Compilation & Packaging (Part XXV)", "[pack][zig][cross_compile][phase16]") {
    TempDir sandbox;

    SECTION("Case 25.1: Multi-Platform Zig Cross-Compilation of solixlib_native") {
        auto out_dir = sandbox.path() / "native_libs";
        auto script_path = std::filesystem::path(SOLIX_PROJECT_ROOT) / "libs" / "solixlib" / "scripts" / "build_native.py";
        if (!std::filesystem::exists(script_path)) {
            script_path = std::filesystem::path(SOLIX_PROJECT_ROOT) / "solixlib" / "scripts" / "build_native.py";
        }
        REQUIRE(std::filesystem::exists(script_path));

#ifdef _WIN32
        const char* zig_check_cmd = "zig version > nul 2>&1";
#else
        const char* zig_check_cmd = "zig version > /dev/null 2>&1";
#endif
        bool has_zig = (std::system(zig_check_cmd) == 0);

        int py_res = std::system(("python3 " + script_path.string() + " --output-dir " + out_dir.string()).c_str());
        if (py_res != 0) {
            py_res = std::system(("python " + script_path.string() + " --output-dir " + out_dir.string()).c_str());
        }
        REQUIRE(py_res == 0);

#ifdef _WIN32
        CHECK(std::filesystem::exists(out_dir / "solixlib_native.dll"));
#elif defined(__APPLE__)
        CHECK(std::filesystem::exists(out_dir / "libsolixlib_native.dylib"));
#else
        CHECK(std::filesystem::exists(out_dir / "libsolixlib_native.so"));
#endif
        if (has_zig) {
            CHECK(std::filesystem::exists(out_dir / "solixlib_native.dll"));
            CHECK(std::filesystem::exists(out_dir / "libsolixlib_native.dylib"));
            CHECK(std::filesystem::exists(out_dir / "libsolixlib_native.so"));
        }
    }

    SECTION("Case 25.2: Release Packaging of Canonical solixlib into Universal Multi-Platform .slxpac") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::path solixlib_dir = std::filesystem::path(SOLIX_PROJECT_ROOT) / "libs" / "solixlib";
        if (!std::filesystem::exists(solixlib_dir)) solixlib_dir = std::filesystem::path(SOLIX_PROJECT_ROOT) / "solixlib";
        std::filesystem::current_path(solixlib_dir);

        auto out_pac = sandbox.path() / "solixlib.slxpac";
        auto pack_res = run_cli({"pack", "-p", "release", "-o", out_pac.string()});
        std::filesystem::current_path(orig_cwd);

        REQUIRE(pack_res.exit_code == 0);
        REQUIRE(std::filesystem::exists(out_pac));

        solix::ZipArchiveReader reader(out_pac);
        CHECK(reader.is_valid());
        CHECK(reader.has_entry("bytecode.slxbin"));
        CHECK(reader.has_entry("manifest.json"));
#ifdef _WIN32
        const char* zig_check_cmd = "zig version > nul 2>&1";
        CHECK(reader.has_entry("lib/solixlib_native.dll"));
#elif defined(__APPLE__)
        const char* zig_check_cmd = "zig version > /dev/null 2>&1";
        CHECK(reader.has_entry("lib/libsolixlib_native.dylib"));
#else
        const char* zig_check_cmd = "zig version > /dev/null 2>&1";
        CHECK(reader.has_entry("lib/libsolixlib_native.so"));
#endif
        if (std::system(zig_check_cmd) == 0) {
            CHECK(reader.has_entry("lib/solixlib_native.dll"));
            CHECK(reader.has_entry("lib/libsolixlib_native.dylib"));
            CHECK(reader.has_entry("lib/libsolixlib_native.so"));
        }
    }

    SECTION("Case 25.3: Consumer Application Release Packaging and Cross-Platform Native Bundling") {
        auto orig_cwd = std::filesystem::current_path();
        auto app_dir = sandbox.path() / "universal_consumer";
        std::filesystem::create_directories(app_dir / "src");

        std::filesystem::path solixlib_dir = std::filesystem::path(SOLIX_PROJECT_ROOT) / "libs" / "solixlib";
        if (!std::filesystem::exists(solixlib_dir)) solixlib_dir = std::filesystem::path(SOLIX_PROJECT_ROOT) / "solixlib";

        nlohmann::json manifest;
        manifest["project"] = "universal_consumer";
        manifest["version"] = "1.0.0";
        manifest["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/main.slx"}},
            {{"type", "project"}, {"path", solixlib_dir.string()}, {"profile", "release"}}
        });
        manifest["profiles"]["release"]["output_directory"] = "build/release";
        manifest["profiles"]["release"]["exe_filename"] = "app.slxbin";
        manifest["profiles"]["release"]["compilation"]["entry_point"] = "main";
        manifest["profiles"]["release"]["compilation"]["multithreaded"] = false;

        std::ofstream mf(app_dir / "manifest.json");
        mf << manifest.dump(2);
        mf.close();

        std::ofstream sf(app_dir / "src/main.slx");
        sf << "import solix.core.Internal;\n"
           << "int32 main() {\n"
           << "    return Internal.version() == 1 ? 0 : 1;\n"
           << "}\n";
        sf.close();

        std::filesystem::current_path(app_dir);
        auto out_pac = app_dir / "consumer.slxpac";
        auto pack_res = run_cli({"pack", "-p", "release", "-o", out_pac.string()});
        REQUIRE(pack_res.exit_code == 0);
        REQUIRE(std::filesystem::exists(out_pac));

        solix::ZipArchiveReader reader(out_pac);
        CHECK(reader.is_valid());
        CHECK(reader.has_entry("bytecode.slxbin"));
#ifdef _WIN32
        const char* zig_check_cmd = "zig version > nul 2>&1";
        CHECK(reader.has_entry("lib/solixlib_native.dll"));
#elif defined(__APPLE__)
        const char* zig_check_cmd = "zig version > /dev/null 2>&1";
        CHECK(reader.has_entry("lib/libsolixlib_native.dylib"));
#else
        const char* zig_check_cmd = "zig version > /dev/null 2>&1";
        CHECK(reader.has_entry("lib/libsolixlib_native.so"));
#endif
        if (std::system(zig_check_cmd) == 0) {
            CHECK(reader.has_entry("lib/solixlib_native.dll"));
            CHECK(reader.has_entry("lib/libsolixlib_native.dylib"));
            CHECK(reader.has_entry("lib/libsolixlib_native.so"));
        }

        // Run the packaged .slxpac
        auto run_res = run_cli({"run", out_pac.string()});
        std::filesystem::current_path(orig_cwd);
        CHECK(run_res.exit_code == 0);
    }

    SECTION("Case 25.4: solix package Alias and End-to-End Scaffolding Packaging") {
        auto proj_dir = sandbox.path() / "scaffold_app";

        // 1. solix new scaffold_app
        auto new_res = run_cli({"new", proj_dir.string()});
        REQUIRE(new_res.exit_code == 0);
        REQUIRE(std::filesystem::exists(proj_dir / "manifest.json"));
        REQUIRE(std::filesystem::exists(proj_dir / "src" / "main.slx"));

        // 2. solix package scaffold_app -o app.slxpac (using 'package' alias)
        auto out_pac = sandbox.path() / "app.slxpac";
        auto pack_res = run_cli({"package", proj_dir.string(), "-o", out_pac.string()});
        REQUIRE(pack_res.exit_code == 0);
        REQUIRE(std::filesystem::exists(out_pac));

        solix::ZipArchiveReader reader(out_pac);
        CHECK(reader.is_valid());
        CHECK(reader.has_entry("bytecode.slxbin"));

        // 3. solix run app.slxpac
        auto run_res = run_cli({"run", out_pac.string()});
        CHECK(run_res.exit_code == 0);
#ifndef _WIN32
        CHECK(run_res.out.find("Hello, World!") != std::string::npos);
#endif
    }
}

