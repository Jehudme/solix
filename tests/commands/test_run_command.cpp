#include <catch2/catch_all.hpp>
#include "cli_test_helper.hpp"
#include "solix/native_registry.hpp"
#include <nlohmann/json.hpp>

using namespace solix::test;

static std::filesystem::path create_runnable_project(
    TempDir& sandbox,
    const std::string& folder_name,
    const std::string& project_name = "sample_app",
    const std::string& version = "1.0.0",
    const std::string& source_code = "int32 main() { return 0; }\n",
    const nlohmann::json& custom_runtime = nlohmann::json::object()) {

    auto proj_dir = sandbox.path() / folder_name;
    std::filesystem::create_directories(proj_dir / "src");

    nlohmann::json manifest;
    manifest["project"] = project_name;
    manifest["version"] = version;
    manifest["dependencies"] = nlohmann::json::array({
        {{"type", "source"}, {"path", "src/main.slx"}}
    });

    manifest["profiles"]["debug"]["output_directory"] = "build/debug";
    manifest["profiles"]["debug"]["exe_filename"] = "out.slxbin";
    manifest["profiles"]["debug"]["compilation"]["entry_point"] = "main";
    manifest["profiles"]["debug"]["compilation"]["multithreaded"] = false;
    if (!custom_runtime.empty()) {
        manifest["profiles"]["debug"]["runtime"] = custom_runtime;
    }

    manifest["profiles"]["release"]["output_directory"] = "build/release";
    manifest["profiles"]["release"]["exe_filename"] = "release_out.slxbin";
    manifest["profiles"]["release"]["compilation"]["entry_point"] = "main";
    manifest["profiles"]["release"]["compilation"]["multithreaded"] = false;

    std::ofstream mf(proj_dir / "solix.json");
    mf << manifest.dump(2);
    mf.close();

    std::ofstream sf(proj_dir / "src/main.slx");
    sf << source_code;
    sf.close();

    return proj_dir;
}

TEST_CASE("CLI Command - run", "[command][run]") {
    TempDir sandbox;

    // Helper to generate a valid bytecode file in the sandbox
    auto generate_bytecode = [&](const std::string& name, const std::string& source) {
        auto src = sandbox.create_file(name + ".slx", source);
        auto bc = sandbox.path() / (name + ".slxb");
        auto comp_res = run_cli({"compile", src.string(), "-o", bc.string()});
        REQUIRE(comp_res.exit_code == 0);
        REQUIRE(std::filesystem::exists(bc));
        return bc;
    };

    SECTION("Positive - Case 2.1: Execute Valid Compiled Bytecode") {
        auto bc = generate_bytecode("app_zero", "int32 main() { return 0; }\n");
        auto res = run_cli({"run", bc.string()});

        CHECK(res.exit_code == 0);
    }

    SECTION("Positive - Case 2.2: Pass Program Arguments to Executing Bytecode") {
        auto bc = generate_bytecode("app_args", "int32 main() { return 0; }\n");
        auto res = run_cli({"run", bc.string(), "arg1", "arg2", "foo"});

        CHECK(res.exit_code == 0);
    }

    SECTION("Positive - Case 2.3: Custom Stack and Heap Capacities") {
        auto bc = generate_bytecode("app_limits", "int32 main() { return 0; }\n");
        auto res = run_cli({"run", bc.string(), "-s", "2048", "-p", "4096"});

        CHECK(res.exit_code == 0);
    }

    SECTION("Positive - Case 2.9: Run Uninstalled Project from Directory (Default Profile)") {
        auto proj = create_runnable_project(sandbox, "uninstalled_default", "uninstalled_default");
        auto res = run_cli({"run", proj.string()});

        CHECK(res.exit_code == 0);
        CHECK(std::filesystem::exists(proj / "build/debug/out.slxbin"));
    }

    SECTION("Positive - Case 2.10: Run Uninstalled Project with Specific Profile") {
        auto proj = create_runnable_project(sandbox, "uninstalled_release", "uninstalled_release");
        auto res = run_cli({"run", proj.string(), "-P", "release"});

        CHECK(res.exit_code == 0);
        CHECK(std::filesystem::exists(proj / "build/release/release_out.slxbin"));
    }

    SECTION("Positive - Case 2.11: Run Uninstalled Project with Manifest Runtime Configuration") {
        nlohmann::json rt;
        rt["stack_size"] = 2048;
        rt["heap_size"] = 4096;
        rt["arguments"] = nlohmann::json::array({"def_arg1", "def_arg2"});

        auto proj = create_runnable_project(sandbox, "uninstalled_rt", "uninstalled_rt", "1.0.0",
                                            "int32 main() { return 0; }\n", rt);
        auto res = run_cli({"run", proj.string()});

        CHECK(res.exit_code == 0);
    }

    SECTION("Positive - Case 2.12: Run Uninstalled Project with CLI Arguments Overriding Runtime Config") {
        nlohmann::json rt;
        rt["arguments"] = nlohmann::json::array({"def_arg1", "def_arg2"});

        auto proj = create_runnable_project(sandbox, "uninstalled_args", "uninstalled_args", "1.0.0",
                                            "int32 main() { return 0; }\n", rt);
        auto res = run_cli({"run", proj.string(), "override_arg1", "override_arg2"});

        CHECK(res.exit_code == 0);
    }

    SECTION("Positive - Case 2.13: Run Installed Project by Name and Version") {
        TempDir solix_home;
        auto proj = create_runnable_project(sandbox, "installed_proj_1", "inst_pkg", "1.0.0");
        auto install_res = run_cli({"install", proj.string()}, solix_home.path());
        REQUIRE(install_res.exit_code == 0);

        auto res = run_cli({"run", "-n", "inst_pkg", "-v", "1.0.0"}, solix_home.path());
        CHECK(res.exit_code == 0);
    }

    SECTION("Positive - Case 2.14: Run Installed Project by Positional Specifier") {
        TempDir solix_home;
        auto proj = create_runnable_project(sandbox, "installed_proj_2", "pos_pkg", "2.1.0");
        auto install_res = run_cli({"install", proj.string()}, solix_home.path());
        REQUIRE(install_res.exit_code == 0);

        auto res = run_cli({"run", "pos_pkg@2.1.0"}, solix_home.path());
        CHECK(res.exit_code == 0);
    }

    SECTION("Positive - Case 2.15: Run Installed Project with Specific Profile") {
        TempDir solix_home;
        auto proj = create_runnable_project(sandbox, "installed_proj_3", "prof_pkg", "1.5.0");
        auto install_res = run_cli({"install", proj.string()}, solix_home.path());
        REQUIRE(install_res.exit_code == 0);

        auto res = run_cli({"run", "prof_pkg@1.5.0", "-P", "release"}, solix_home.path());
        CHECK(res.exit_code == 0);
    }

    SECTION("Negative - Case 2.4: Missing Bytecode File Argument") {
        auto res = run_cli({"run"});

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("file is required") != std::string::npos);
    }

    SECTION("Negative - Case 2.5: Non-Existent Bytecode File") {
        auto missing = sandbox.path() / "non_existent.slxb";
        auto res = run_cli({"run", missing.string()});

        CHECK(res.exit_code != 0);
        CHECK((res.err.find("non_existent.slxb") != std::string::npos || res.err.find("does not exist") != std::string::npos));
    }

    SECTION("Negative - Case 2.6: Empty Bytecode File") {
        auto empty_file = sandbox.create_file("empty.slxb", "");
        auto res = run_cli({"run", empty_file.string()});

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("empty") != std::string::npos);
    }

    SECTION("Negative - Case 2.7: Runtime Fault / Unhandled Exception") {
        std::string fault_source = R"(
class Exception {}
int32 main() {
    Exception e = null;
    throw e;
    return 0;
}
)";
        auto bc = generate_bytecode("app_fault", fault_source);
        auto res = run_cli({"run", bc.string()});

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("Runtime error") != std::string::npos);
    }

    SECTION("Negative - Case 2.8: Non-Zero Exit Code Propagation") {
        auto bc = generate_bytecode("app_exit7", "int32 main() { return 7; }\n");
        auto res = run_cli({"run", bc.string()});

        CHECK(res.exit_code == 7);
    }

    SECTION("Negative - Case 2.16: Run Project with Non-Existent Directory Path") {
        auto missing_dir = sandbox.path() / "non_existent_proj";
        auto res = run_cli({"run", missing_dir.string()});

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("does not exist") != std::string::npos);
    }

    SECTION("Negative - Case 2.17: Run Project in Directory Lacking solix.json") {
        auto empty_dir = sandbox.path() / "empty_proj_dir";
        std::filesystem::create_directories(empty_dir);

        auto res = run_cli({"run", empty_dir.string()});

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("Manifest file does not exist") != std::string::npos);
    }

    SECTION("Negative - Case 2.18: Run Project Requesting Non-Existent Profile") {
        auto proj = create_runnable_project(sandbox, "no_custom_prof", "no_custom_prof");
        auto res = run_cli({"run", proj.string(), "-P", "missing_profile"});

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("Profile 'missing_profile' not defined") != std::string::npos);
    }

    SECTION("Negative - Case 2.19: Run Installed Project Not Found in Registry") {
        TempDir solix_home;
        auto res = run_cli({"run", "-n", "ghost_pkg", "-v", "9.9.9"}, solix_home.path());

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("not found") != std::string::npos);
    }

    SECTION("Negative - Case 2.20: Run Installed Project with --package but Missing --version") {
        auto res = run_cli({"run", "-n", "some_pkg"});

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("--version is required") != std::string::npos);
    }

    SECTION("Negative - Case 2.21: Run Project with Compilation Error") {
        auto proj = create_runnable_project(sandbox, "broken_syntax_proj", "broken_syntax_proj",
                                            "1.0.0", "invalid syntax code !!!;\n");
        auto res = run_cli({"run", proj.string()});

        CHECK(res.exit_code != 0);
    }

    SECTION("Case 2.22: Run Project with Native Auto-Discovery from lib/ Directory") {
        const std::string native_code = R"(
            public class NativeMath {
                public static native int32 add(int32 a, int32 b);
            }
            int32 main() {
                return NativeMath.add(10, 20) == 30 ? 0 : 1;
            }
        )";
        auto proj = create_runnable_project(sandbox, "proj_autodiscover_lib", "proj_autodiscover_lib",
                                            "1.0.0", native_code);
        auto lib_dir = proj / "lib";
        std::filesystem::create_directories(lib_dir);

        std::filesystem::path plugin_src(SOLIX_TEST_PLUGIN_PATH);
        REQUIRE(std::filesystem::exists(plugin_src));
        std::filesystem::copy_file(plugin_src, lib_dir / plugin_src.filename(),
                                   std::filesystem::copy_options::overwrite_existing);

        auto res = run_cli({"run", proj.string()});
        CHECK(res.exit_code == 0);
    }

    SECTION("Case 2.23: Run Bytecode Binary with Co-located Native Library Auto-Discovery") {
        const std::string native_code = R"(
            public class NativeMath {
                public static native int32 add(int32 a, int32 b);
            }
            int32 main() {
                return NativeMath.add(15, 27) == 42 ? 0 : 1;
            }
        )";
        auto sub_dir = sandbox.path() / "colocated_test";
        std::filesystem::create_directories(sub_dir);
        auto src = sub_dir / "app.slx";
        {
            std::ofstream sf(src);
            sf << native_code;
        }
        auto bc = sub_dir / "app.slxbin";
        auto comp_res = run_cli({"compile", src.string(), "-o", bc.string()});
        REQUIRE(comp_res.exit_code == 0);

        std::filesystem::path plugin_src(SOLIX_TEST_PLUGIN_PATH);
        REQUIRE(std::filesystem::exists(plugin_src));
        std::filesystem::copy_file(plugin_src, sub_dir / plugin_src.filename(),
                                   std::filesystem::copy_options::overwrite_existing);

        auto res = run_cli({"run", bc.string()});
        CHECK(res.exit_code == 0);
    }

    SECTION("Case 2.24: Run Project with Explicit native_libraries in Manifest") {
        const std::string native_code = R"(
            public class NativeMath {
                public static native int32 add(int32 a, int32 b);
            }
            int32 main() {
                return NativeMath.add(100, 200) == 300 ? 0 : 1;
            }
        )";
        auto proj = create_runnable_project(sandbox, "proj_manifest_native", "proj_manifest_native",
                                            "1.0.0", native_code);
        // Update manifest to add root native_libraries
        auto mf_path = proj / "solix.json";
        nlohmann::json manifest;
        {
            std::ifstream in(mf_path);
            in >> manifest;
        }
        manifest["native_libraries"] = nlohmann::json::array({SOLIX_TEST_PLUGIN_PATH});
        {
            std::ofstream out(mf_path);
            out << manifest.dump(2);
        }

        auto res = run_cli({"run", proj.string()});
        CHECK(res.exit_code == 0);
    }

    SECTION("Case 2.25: Run Bytecode with Explicit CLI --native-lib / -L Flag") {
        const std::string native_code = R"(
            public class NativeMath {
                public static native int32 add(int32 a, int32 b);
            }
            int32 main() {
                return NativeMath.add(7, 8) == 15 ? 0 : 1;
            }
        )";
        auto isolated_dir = sandbox.path() / "isolated_bin";
        std::filesystem::create_directories(isolated_dir);
        auto src = isolated_dir / "app.slx";
        {
            std::ofstream sf(src);
            sf << native_code;
        }
        auto bc = isolated_dir / "app.slxbin";
        auto comp_res = run_cli({"compile", src.string(), "-o", bc.string()});
        REQUIRE(comp_res.exit_code == 0);

        auto res = run_cli({"run", bc.string(), "-L", SOLIX_TEST_PLUGIN_PATH});
        CHECK(res.exit_code == 0);
    }

    SECTION("Negative - Case 2.26: Run Binary Requiring Native Library Without Providing It") {
        solix::NativeRegistry::global().clear();
        const std::string native_code = R"(
            public class MissingNative {
                public static native int32 non_existent(int32 a, int32 b);
            }
            int32 main() {
                return MissingNative.non_existent(1, 2);
            }
        )";
        auto isolated_dir = sandbox.path() / "isolated_no_native";
        std::filesystem::create_directories(isolated_dir);
        auto src = isolated_dir / "app.slx";
        {
            std::ofstream sf(src);
            sf << native_code;
        }
        auto bc = isolated_dir / "app.slxbin";
        auto comp_res = run_cli({"compile", src.string(), "-o", bc.string()});
        REQUIRE(comp_res.exit_code == 0);

        // Run without -L and without co-located library
        auto res = run_cli({"run", bc.string()});
        CHECK(res.exit_code != 0);
        CHECK(res.err.find("Call to unknown native function") != std::string::npos);
    }
}


