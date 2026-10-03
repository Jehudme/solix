#include <catch2/catch_all.hpp>
#include "cli_test_helper.hpp"
#include <nlohmann/json.hpp>

using namespace solix::test;

static std::string create_sample_manifest(const std::string& name = "sample_proj") {
    nlohmann::json manifest;
    manifest["project"] = name;
    manifest["version"] = "0.1.0";
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
        p["asm_filename"] = "out.slxasm";
        p["compilation"] = {
            {"entry_point", "main"},
            {"multithreaded", false},
            {"additional_dependencies", nlohmann::json::array()}
        };
        return p;
    };

    manifest["profiles"]["debug"] = make_profile("debug");
    manifest["profiles"]["release"] = make_profile("release");
    return manifest.dump(2);
}

TEST_CASE("CLI Command - build", "[command][build]") {
    TempDir sandbox;

    SECTION("Positive - Case 3.1: Build Default (Debug) Profile from Working Directory") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        sandbox.create_file("solix.json", create_sample_manifest());
        sandbox.create_file("src/main.slx", "static int32 main() { return 0; }\n");

        auto res = run_cli({"build"});
        std::filesystem::current_path(orig_cwd);

        CHECK(res.exit_code == 0);
        CHECK(sandbox.exists("build/debug/out.slxbin"));
        CHECK(std::filesystem::file_size(sandbox.path() / "build/debug/out.slxbin") > 0);
    }

    SECTION("Positive - Case 3.2: Build Specific (Release) Profile") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        sandbox.create_file("solix.json", create_sample_manifest());
        sandbox.create_file("src/main.slx", "static int32 main() { return 0; }\n");

        auto res = run_cli({"build", "-p", "release"});
        std::filesystem::current_path(orig_cwd);

        CHECK(res.exit_code == 0);
        CHECK(sandbox.exists("build/release/out.slxbin"));
        CHECK(std::filesystem::file_size(sandbox.path() / "build/release/out.slxbin") > 0);
    }

    SECTION("Positive - Case 3.3: Custom Manifest Path via Positional Argument") {
        auto proj_dir = sandbox.path() / "nested_proj";
        std::filesystem::create_directories(proj_dir / "src");

        auto manifest_path = proj_dir / "solix.json";
        std::ofstream mf(manifest_path);
        mf << create_sample_manifest();
        mf.close();

        std::ofstream sf(proj_dir / "src/main.slx");
        sf << "static int32 main() { return 0; }\n";
        sf.close();

        auto res = run_cli({"build", manifest_path.string()});

        CHECK(res.exit_code == 0);
        CHECK(std::filesystem::exists(proj_dir / "build/debug/out.slxbin"));
    }

    SECTION("Positive - Case 3.4: Multi-File Project with Dependency Resolution") {
        nlohmann::json manifest = nlohmann::json::parse(create_sample_manifest("multi_proj"));
        manifest["dependencies"].push_back({
            {"type", "source"},
            {"path", "src/math.slx"}
        });

        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        sandbox.create_file("solix.json", manifest.dump(2));
        sandbox.create_file("src/math.slx", "class MathHelper { public static int32 add() { return 42; } }\n");
        sandbox.create_file("src/main.slx", "static int32 main() { return MathHelper.add() == 42 ? 0 : 1; }\n");

        auto res = run_cli({"build"});
        std::filesystem::current_path(orig_cwd);

        CHECK(res.exit_code == 0);
        CHECK(sandbox.exists("build/debug/out.slxbin"));

        // Verify the built binary actually runs!
        auto run_res = run_cli({"run", (sandbox.path() / "build/debug/out.slxbin").string()});
        CHECK(run_res.exit_code == 0);
    }

    SECTION("Negative - Case 3.5: Missing Manifest File") {
        auto missing = sandbox.path() / "non_existent_solix.json";
        auto res = run_cli({"build", missing.string()});

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("Manifest file does not exist") != std::string::npos);
    }

    SECTION("Negative - Case 3.6: Malformed Manifest JSON") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        sandbox.create_file("solix.json", "{ invalid json content... ");

        auto res = run_cli({"build"});
        std::filesystem::current_path(orig_cwd);

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("Failed to parse solix.json") != std::string::npos);
    }

    SECTION("Negative - Case 3.7: Manifest Missing Profiles Section") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        sandbox.create_file("solix.json", R"({"project": "no_profiles", "version": "1.0.0"})");

        auto res = run_cli({"build"});
        std::filesystem::current_path(orig_cwd);

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("'profiles' section missing") != std::string::npos);
    }

    SECTION("Negative - Case 3.8: Requested Profile Does Not Exist") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        sandbox.create_file("solix.json", create_sample_manifest());
        sandbox.create_file("src/main.slx", "static int32 main() { return 0; }\n");

        auto res = run_cli({"build", "-p", "non_existent_profile"});
        std::filesystem::current_path(orig_cwd);

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("Profile 'non_existent_profile' not defined") != std::string::npos);
    }

    SECTION("Negative - Case 3.9: Missing Source File Specified in Profile") {
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        sandbox.create_file("solix.json", create_sample_manifest());
        // Deliberately do NOT create src/main.slx

        auto res = run_cli({"build"});
        std::filesystem::current_path(orig_cwd);

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("does not exist") != std::string::npos);
    }

    SECTION("Positive - Case 3.10: Direct Project Dependency via Relative Path") {
        // Create library project
        auto lib_dir = sandbox.path() / "lib_math";
        std::filesystem::create_directories(lib_dir / "src");
        nlohmann::json lib_manifest;
        lib_manifest["project"] = "lib_math";
        lib_manifest["version"] = "1.0.0";
        lib_manifest["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/math.slx"}}
        });
        lib_manifest["profiles"]["debug"]["output_directory"] = "build/debug";
        std::ofstream lib_mf(lib_dir / "solix.json");
        lib_mf << lib_manifest.dump(2);
        lib_mf.close();

        std::ofstream lib_sf(lib_dir / "src/math.slx");
        lib_sf << "class MathLib { public static int32 getValue() { return 100; } }\n";
        lib_sf.close();

        // Create app project
        auto app_dir = sandbox.path() / "app";
        std::filesystem::create_directories(app_dir / "src");
        nlohmann::json app_manifest;
        app_manifest["project"] = "app";
        app_manifest["version"] = "0.1.0";
        app_manifest["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/main.slx"}},
            {{"type", "project"}, {"name", "lib_math"}, {"version", "1.0.0"}, {"path", "../lib_math"}}
        });
        app_manifest["profiles"]["debug"]["output_directory"] = "build/debug";
        app_manifest["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        app_manifest["profiles"]["debug"]["compilation"]["entry_point"] = "main";
        app_manifest["profiles"]["debug"]["compilation"]["multithreaded"] = false;
        std::ofstream app_mf(app_dir / "solix.json");
        app_mf << app_manifest.dump(2);
        app_mf.close();

        std::ofstream app_sf(app_dir / "src/main.slx");
        app_sf << "static int32 main() { return MathLib.getValue() == 100 ? 0 : 1; }\n";
        app_sf.close();

        auto res = run_cli({"build", (app_dir / "solix.json").string()});
        CHECK(res.exit_code == 0);
        CHECK(std::filesystem::exists(app_dir / "build/debug/out.slxbin"));

        auto run_res = run_cli({"run", (app_dir / "build/debug/out.slxbin").string()});
        CHECK(run_res.exit_code == 0);
    }

    SECTION("Positive - Case 3.11: Transitive Project Dependency Chain (A -> B -> C)") {
        // Create C
        auto c_dir = sandbox.path() / "lib_c";
        std::filesystem::create_directories(c_dir / "src");
        nlohmann::json c_manifest;
        c_manifest["project"] = "lib_c";
        c_manifest["version"] = "1.0.0";
        c_manifest["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/c.slx"}}
        });
        std::ofstream c_mf(c_dir / "solix.json");
        c_mf << c_manifest.dump(2);
        c_mf.close();
        std::ofstream c_sf(c_dir / "src/c.slx");
        c_sf << "class ClassC { public static int32 val() { return 5; } }\n";
        c_sf.close();

        // Create B depending on C
        auto b_dir = sandbox.path() / "lib_b";
        std::filesystem::create_directories(b_dir / "src");
        nlohmann::json b_manifest;
        b_manifest["project"] = "lib_b";
        b_manifest["version"] = "1.0.0";
        b_manifest["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/b.slx"}},
            {{"type", "project"}, {"name", "lib_c"}, {"version", "1.0.0"}, {"path", "../lib_c"}}
        });
        std::ofstream b_mf(b_dir / "solix.json");
        b_mf << b_manifest.dump(2);
        b_mf.close();
        std::ofstream b_sf(b_dir / "src/b.slx");
        b_sf << "class ClassB { public static int32 val() { return ClassC.val() * 3; } }\n";
        b_sf.close();

        // Create A depending on B
        auto a_dir = sandbox.path() / "app_transitive";
        std::filesystem::create_directories(a_dir / "src");
        nlohmann::json a_manifest;
        a_manifest["project"] = "app_transitive";
        a_manifest["version"] = "1.0.0";
        a_manifest["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/main.slx"}},
            {{"type", "project"}, {"name", "lib_b"}, {"version", "1.0.0"}, {"path", "../lib_b"}}
        });
        a_manifest["profiles"]["debug"]["output_directory"] = "build/debug";
        a_manifest["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        a_manifest["profiles"]["debug"]["compilation"]["entry_point"] = "main";
        a_manifest["profiles"]["debug"]["compilation"]["multithreaded"] = false;
        std::ofstream a_mf(a_dir / "solix.json");
        a_mf << a_manifest.dump(2);
        a_mf.close();
        std::ofstream a_sf(a_dir / "src/main.slx");
        a_sf << "static int32 main() { return ClassB.val() == 15 ? 0 : 1; }\n";
        a_sf.close();

        auto res = run_cli({"build", (a_dir / "solix.json").string()});
        CHECK(res.exit_code == 0);
        CHECK(std::filesystem::exists(a_dir / "build/debug/out.slxbin"));

        auto run_res = run_cli({"run", (a_dir / "build/debug/out.slxbin").string()});
        CHECK(run_res.exit_code == 0);
    }

    SECTION("Positive - Case 3.12: Circular / Mutual Project Dependencies (A <-> B)") {
        auto lib1_dir = sandbox.path() / "circ_lib1";
        auto lib2_dir = sandbox.path() / "circ_lib2";
        std::filesystem::create_directories(lib1_dir / "src");
        std::filesystem::create_directories(lib2_dir / "src");

        nlohmann::json m1;
        m1["project"] = "circ_lib1";
        m1["version"] = "1.0.0";
        m1["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/lib1.slx"}},
            {{"type", "project"}, {"name", "circ_lib2"}, {"version", "1.0.0"}, {"path", "../circ_lib2"}}
        });
        std::ofstream m1_file(lib1_dir / "solix.json");
        m1_file << m1.dump(2);
        m1_file.close();
        std::ofstream s1_file(lib1_dir / "src/lib1.slx");
        s1_file << "class LibOne { public static int32 val1() { return 11; } }\n";
        s1_file.close();

        nlohmann::json m2;
        m2["project"] = "circ_lib2";
        m2["version"] = "1.0.0";
        m2["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/lib2.slx"}},
            {{"type", "project"}, {"name", "circ_lib1"}, {"version", "1.0.0"}, {"path", "../circ_lib1"}}
        });
        std::ofstream m2_file(lib2_dir / "solix.json");
        m2_file << m2.dump(2);
        m2_file.close();
        std::ofstream s2_file(lib2_dir / "src/lib2.slx");
        s2_file << "class LibTwo { public static int32 val2() { return 22; } }\n";
        s2_file.close();

        // Main app depends on circ_lib1
        auto app_dir = sandbox.path() / "circ_app";
        std::filesystem::create_directories(app_dir / "src");
        nlohmann::json app_m;
        app_m["project"] = "circ_app";
        app_m["version"] = "1.0.0";
        app_m["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/main.slx"}},
            {{"type", "project"}, {"name", "circ_lib1"}, {"version", "1.0.0"}, {"path", "../circ_lib1"}}
        });
        app_m["profiles"]["debug"]["output_directory"] = "build/debug";
        app_m["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        app_m["profiles"]["debug"]["compilation"]["entry_point"] = "main";
        app_m["profiles"]["debug"]["compilation"]["multithreaded"] = false;
        std::ofstream app_file(app_dir / "solix.json");
        app_file << app_m.dump(2);
        app_file.close();
        std::ofstream app_src(app_dir / "src/main.slx");
        app_src << "static int32 main() { return (LibOne.val1() + LibTwo.val2()) == 33 ? 0 : 1; }\n";
        app_src.close();

        auto res = run_cli({"build", (app_dir / "solix.json").string()});
        CHECK(res.exit_code == 0);
        CHECK(std::filesystem::exists(app_dir / "build/debug/out.slxbin"));

        auto run_res = run_cli({"run", (app_dir / "build/debug/out.slxbin").string()});
        CHECK(run_res.exit_code == 0);
    }

    SECTION("Positive - Case 3.13: SemVer Patch Difference Silent Resolution") {
        // Create shared_lib@1.0.1
        auto v101_dir = sandbox.path() / "shared_v101";
        std::filesystem::create_directories(v101_dir / "src");
        nlohmann::json m101;
        m101["project"] = "shared_lib";
        m101["version"] = "1.0.1";
        m101["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/shared.slx"}}
        });
        std::ofstream f101_m(v101_dir / "solix.json");
        f101_m << m101.dump(2);
        f101_m.close();
        std::ofstream f101_s(v101_dir / "src/shared.slx");
        f101_s << "class SharedLib { public static int32 patchNum() { return 1; } }\n";
        f101_s.close();

        // Create shared_lib@1.0.4
        auto v104_dir = sandbox.path() / "shared_v104";
        std::filesystem::create_directories(v104_dir / "src");
        nlohmann::json m104;
        m104["project"] = "shared_lib";
        m104["version"] = "1.0.4";
        m104["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/shared.slx"}}
        });
        std::ofstream f104_m(v104_dir / "solix.json");
        f104_m << m104.dump(2);
        f104_m.close();
        std::ofstream f104_s(v104_dir / "src/shared.slx");
        f104_s << "class SharedLib { public static int32 patchNum() { return 4; } }\n";
        f104_s.close();

        // lib_left depends on 1.0.1
        auto left_dir = sandbox.path() / "lib_left";
        std::filesystem::create_directories(left_dir / "src");
        nlohmann::json ml;
        ml["project"] = "lib_left";
        ml["version"] = "1.0.0";
        ml["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/left.slx"}},
            {{"type", "project"}, {"name", "shared_lib"}, {"version", "1.0.1"}, {"path", "../shared_v101"}}
        });
        std::ofstream f_left(left_dir / "solix.json");
        f_left << ml.dump(2);
        f_left.close();
        std::ofstream s_left(left_dir / "src/left.slx");
        s_left << "class LibLeft { public static int32 left() { return 10; } }\n";
        s_left.close();

        // lib_right depends on 1.0.4
        auto right_dir = sandbox.path() / "lib_right";
        std::filesystem::create_directories(right_dir / "src");
        nlohmann::json mr;
        mr["project"] = "lib_right";
        mr["version"] = "1.0.0";
        mr["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/right.slx"}},
            {{"type", "project"}, {"name", "shared_lib"}, {"version", "1.0.4"}, {"path", "../shared_v104"}}
        });
        std::ofstream f_right(right_dir / "solix.json");
        f_right << mr.dump(2);
        f_right.close();
        std::ofstream s_right(right_dir / "src/right.slx");
        s_right << "class LibRight { public static int32 right() { return 20; } }\n";
        s_right.close();

        // Main app depends on both lib_left and lib_right
        auto app_dir = sandbox.path() / "patch_app";
        std::filesystem::create_directories(app_dir / "src");
        nlohmann::json app_m;
        app_m["project"] = "patch_app";
        app_m["version"] = "1.0.0";
        app_m["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/main.slx"}},
            {{"type", "project"}, {"name", "lib_left"}, {"version", "1.0.0"}, {"path", "../lib_left"}},
            {{"type", "project"}, {"name", "lib_right"}, {"version", "1.0.0"}, {"path", "../lib_right"}}
        });
        app_m["profiles"]["debug"]["output_directory"] = "build/debug";
        app_m["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        app_m["profiles"]["debug"]["compilation"]["entry_point"] = "main";
        app_m["profiles"]["debug"]["compilation"]["multithreaded"] = false;
        std::ofstream app_file(app_dir / "solix.json");
        app_file << app_m.dump(2);
        app_file.close();
        std::ofstream app_src(app_dir / "src/main.slx");
        // Should use version 1.0.4 which has patchNum() == 4
        app_src << "static int32 main() { return (LibLeft.left() + LibRight.right() + SharedLib.patchNum()) == 34 ? 0 : 1; }\n";
        app_src.close();

        auto res = run_cli({"build", (app_dir / "solix.json").string()});
        CHECK(res.exit_code == 0);
        CHECK(res.out.find("Warning") == std::string::npos);

        auto run_res = run_cli({"run", (app_dir / "build/debug/out.slxbin").string()});
        CHECK(run_res.exit_code == 0);
    }

    SECTION("Positive - Case 3.14: SemVer Minor Difference Warning and Resolution") {
        // Create util@1.1.0
        auto v110_dir = sandbox.path() / "util_v110";
        std::filesystem::create_directories(v110_dir / "src");
        nlohmann::json m110;
        m110["project"] = "util_lib";
        m110["version"] = "1.1.0";
        m110["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/util.slx"}}
        });
        std::ofstream f110_m(v110_dir / "solix.json");
        f110_m << m110.dump(2);
        f110_m.close();
        std::ofstream f110_s(v110_dir / "src/util.slx");
        f110_s << "class UtilLib { public static int32 minorNum() { return 1; } }\n";
        f110_s.close();

        // Create util@1.3.0
        auto v130_dir = sandbox.path() / "util_v130";
        std::filesystem::create_directories(v130_dir / "src");
        nlohmann::json m130;
        m130["project"] = "util_lib";
        m130["version"] = "1.3.0";
        m130["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/util.slx"}}
        });
        std::ofstream f130_m(v130_dir / "solix.json");
        f130_m << m130.dump(2);
        f130_m.close();
        std::ofstream f130_s(v130_dir / "src/util.slx");
        f130_s << "class UtilLib { public static int32 minorNum() { return 3; } }\n";
        f130_s.close();

        // Main app depends directly on both to test conflict resolution
        auto app_dir = sandbox.path() / "minor_app";
        std::filesystem::create_directories(app_dir / "src");
        nlohmann::json app_m;
        app_m["project"] = "minor_app";
        app_m["version"] = "1.0.0";
        app_m["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/main.slx"}},
            {{"type", "project"}, {"name", "util_lib"}, {"version", "1.1.0"}, {"path", "../util_v110"}},
            {{"type", "project"}, {"name", "util_lib"}, {"version", "1.3.0"}, {"path", "../util_v130"}}
        });
        app_m["profiles"]["debug"]["output_directory"] = "build/debug";
        app_m["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        app_m["profiles"]["debug"]["compilation"]["entry_point"] = "main";
        app_m["profiles"]["debug"]["compilation"]["multithreaded"] = false;
        std::ofstream app_file(app_dir / "solix.json");
        app_file << app_m.dump(2);
        app_file.close();
        std::ofstream app_src(app_dir / "src/main.slx");
        // Selected version should be 1.3.0 which has minorNum() == 3
        app_src << "static int32 main() { return UtilLib.minorNum() == 3 ? 0 : 1; }\n";
        app_src.close();

        auto res = run_cli({"build", (app_dir / "solix.json").string()});
        CHECK(res.exit_code == 0);
        CHECK(res.out.find("Warning: Project 'util_lib' has multiple minor versions") != std::string::npos);

        auto run_res = run_cli({"run", (app_dir / "build/debug/out.slxbin").string()});
        CHECK(run_res.exit_code == 0);
    }

    SECTION("Positive - Case 3.15: Project Dependency Resolved from Installed Registry") {
        TempDir solix_home;

        // Create and install helper_pkg
        auto helper_dir = sandbox.path() / "helper_pkg";
        std::filesystem::create_directories(helper_dir / "src");
        nlohmann::json hm;
        hm["project"] = "helper_pkg";
        hm["version"] = "1.2.0";
        hm["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/helper.slx"}}
        });
        hm["profiles"]["debug"]["output_directory"] = "build/debug";
        hm["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        hm["profiles"]["debug"]["compilation"]["entry_point"] = "main";
        hm["profiles"]["debug"]["compilation"]["multithreaded"] = false;
        std::ofstream h_mf(helper_dir / "solix.json");
        h_mf << hm.dump(2);
        h_mf.close();
        std::ofstream h_sf(helper_dir / "src/helper.slx");
        h_sf << "class HelperPkg { public static int32 getAnswer() { return 42; } }\n";
        h_sf.close();

        auto install_res = run_cli({"install", helper_dir.string()}, solix_home.path());
        CHECK(install_res.exit_code == 0);

        // Create consumer app referencing helper_pkg without a path
        auto app_dir = sandbox.path() / "registry_app";
        std::filesystem::create_directories(app_dir / "src");
        nlohmann::json app_m;
        app_m["project"] = "registry_app";
        app_m["version"] = "1.0.0";
        app_m["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/main.slx"}},
            {{"type", "project"}, {"package", "helper_pkg@1.2.0"}}
        });
        app_m["profiles"]["debug"]["output_directory"] = "build/debug";
        app_m["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        app_m["profiles"]["debug"]["compilation"]["entry_point"] = "main";
        app_m["profiles"]["debug"]["compilation"]["multithreaded"] = false;
        std::ofstream app_file(app_dir / "solix.json");
        app_file << app_m.dump(2);
        app_file.close();
        std::ofstream app_src(app_dir / "src/main.slx");
        app_src << "static int32 main() { return HelperPkg.getAnswer() == 42 ? 0 : 1; }\n";
        app_src.close();

        auto res = run_cli({"build", (app_dir / "solix.json").string()}, solix_home.path());
        CHECK(res.exit_code == 0);
        CHECK(std::filesystem::exists(app_dir / "build/debug/out.slxbin"));

        auto run_res = run_cli({"run", (app_dir / "build/debug/out.slxbin").string()}, solix_home.path());
        CHECK(run_res.exit_code == 0);
    }

    SECTION("Negative - Case 3.16: Incompatible Major SemVer Collision") {
        auto v1_dir = sandbox.path() / "core_v1";
        std::filesystem::create_directories(v1_dir / "src");
        nlohmann::json m1;
        m1["project"] = "core_lib";
        m1["version"] = "1.0.0";
        m1["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/core.slx"}}
        });
        std::ofstream f1_m(v1_dir / "solix.json");
        f1_m << m1.dump(2);
        f1_m.close();
        std::ofstream f1_s(v1_dir / "src/core.slx");
        f1_s << "class Core { public static int32 val() { return 1; } }\n";
        f1_s.close();

        auto v2_dir = sandbox.path() / "core_v2";
        std::filesystem::create_directories(v2_dir / "src");
        nlohmann::json m2;
        m2["project"] = "core_lib";
        m2["version"] = "2.0.0";
        m2["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/core.slx"}}
        });
        std::ofstream f2_m(v2_dir / "solix.json");
        f2_m << m2.dump(2);
        f2_m.close();
        std::ofstream f2_s(v2_dir / "src/core.slx");
        f2_s << "class Core { public static int32 val() { return 2; } }\n";
        f2_s.close();

        auto app_dir = sandbox.path() / "collision_app";
        std::filesystem::create_directories(app_dir / "src");
        nlohmann::json app_m;
        app_m["project"] = "collision_app";
        app_m["version"] = "1.0.0";
        app_m["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/main.slx"}},
            {{"type", "project"}, {"name", "core_lib"}, {"version", "1.0.0"}, {"path", "../core_v1"}},
            {{"type", "project"}, {"name", "core_lib"}, {"version", "2.0.0"}, {"path", "../core_v2"}}
        });
        app_m["profiles"]["debug"]["output_directory"] = "build/debug";
        app_m["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        app_m["profiles"]["debug"]["compilation"]["entry_point"] = "main";
        app_m["profiles"]["debug"]["compilation"]["multithreaded"] = false;
        std::ofstream app_file(app_dir / "solix.json");
        app_file << app_m.dump(2);
        app_file.close();
        std::ofstream app_src(app_dir / "src/main.slx");
        app_src << "static int32 main() { return 0; }\n";
        app_src.close();

        auto res = run_cli({"build", (app_dir / "solix.json").string()});
        CHECK(res.exit_code != 0);
        CHECK(res.err.find("Incompatible major versions for dependency 'core_lib'") != std::string::npos);
    }

    SECTION("Negative - Case 3.17: Missing Project Dependency Manifest Path") {
        auto app_dir = sandbox.path() / "bad_path_app";
        std::filesystem::create_directories(app_dir / "src");
        nlohmann::json app_m;
        app_m["project"] = "bad_path_app";
        app_m["version"] = "1.0.0";
        app_m["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/main.slx"}},
            {{"type", "project"}, {"name", "missing_dep"}, {"version", "1.0.0"}, {"path", "../non_existent_dep"}}
        });
        app_m["profiles"]["debug"]["output_directory"] = "build/debug";
        app_m["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        app_m["profiles"]["debug"]["compilation"]["entry_point"] = "main";
        app_m["profiles"]["debug"]["compilation"]["multithreaded"] = false;
        std::ofstream app_file(app_dir / "solix.json");
        app_file << app_m.dump(2);
        app_file.close();
        std::ofstream app_src(app_dir / "src/main.slx");
        app_src << "static int32 main() { return 0; }\n";
        app_src.close();

        auto res = run_cli({"build", (app_dir / "solix.json").string()});
        CHECK(res.exit_code != 0);
        CHECK(res.err.find("Project dependency manifest does not exist") != std::string::npos);
    }

    SECTION("Negative - Case 3.18: Missing Source File in Dependent Project") {
        auto lib_dir = sandbox.path() / "broken_lib";
        std::filesystem::create_directories(lib_dir / "src");
        nlohmann::json lib_m;
        lib_m["project"] = "broken_lib";
        lib_m["version"] = "1.0.0";
        lib_m["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/non_existent.slx"}}
        });
        std::ofstream lib_file(lib_dir / "solix.json");
        lib_file << lib_m.dump(2);
        lib_file.close();
        // Notice src/non_existent.slx is not created!

        auto app_dir = sandbox.path() / "broken_dep_app";
        std::filesystem::create_directories(app_dir / "src");
        nlohmann::json app_m;
        app_m["project"] = "broken_dep_app";
        app_m["version"] = "1.0.0";
        app_m["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/main.slx"}},
            {{"type", "project"}, {"name", "broken_lib"}, {"version", "1.0.0"}, {"path", "../broken_lib"}}
        });
        app_m["profiles"]["debug"]["output_directory"] = "build/debug";
        app_m["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        app_m["profiles"]["debug"]["compilation"]["entry_point"] = "main";
        app_m["profiles"]["debug"]["compilation"]["multithreaded"] = false;
        std::ofstream app_file(app_dir / "solix.json");
        app_file << app_m.dump(2);
        app_file.close();
        std::ofstream app_src(app_dir / "src/main.slx");
        app_src << "static int32 main() { return 0; }\n";
        app_src.close();

        auto res = run_cli({"build", (app_dir / "solix.json").string()});
        CHECK(res.exit_code != 0);
        CHECK(res.err.find("Dependency source file does not exist") != std::string::npos);
    }

    SECTION("Positive - Case 3.19: Build Project by Directory Path Positional Argument") {
        auto proj_dir = sandbox.path() / "pos_dir_proj";
        std::filesystem::create_directories(proj_dir / "src");

        std::ofstream mf(proj_dir / "solix.json");
        mf << create_sample_manifest("pos_dir_proj");
        mf.close();

        std::ofstream sf(proj_dir / "src/main.slx");
        sf << "static int32 main() { return 0; }\n";
        sf.close();

        auto res = run_cli({"build", proj_dir.string()});
        CHECK(res.exit_code == 0);
        CHECK(std::filesystem::exists(proj_dir / "build/debug/out.slxbin"));
    }

    SECTION("Positive - Case 3.20: Build Installed Package by name@version Positional Argument") {
        TempDir solix_home;

        auto pkg_dir = sandbox.path() / "inst_pkg";
        std::filesystem::create_directories(pkg_dir / "src");
        nlohmann::json pm;
        pm["project"] = "demo_installed_pkg";
        pm["version"] = "2.0.0";
        pm["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/main.slx"}}
        });
        pm["profiles"]["debug"]["output_directory"] = "build/debug";
        pm["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        pm["profiles"]["debug"]["compilation"]["entry_point"] = "main";
        pm["profiles"]["debug"]["compilation"]["multithreaded"] = false;

        std::ofstream pf(pkg_dir / "solix.json");
        pf << pm.dump(2);
        pf.close();

        std::ofstream ps(pkg_dir / "src/main.slx");
        ps << "class DemoInst { public static int32 val() { return 100; } }\n"
           << "static int32 main() { return 0; }\n";
        ps.close();

        auto inst_res = run_cli({"install", pkg_dir.string()}, solix_home.path());
        CHECK(inst_res.exit_code == 0);

        // Build using name@version
        auto res = run_cli({"build", "demo_installed_pkg@2.0.0"}, solix_home.path());
        CHECK(res.exit_code == 0);
    }

    SECTION("Positive - Case 3.21: Build Project with Combined name@version in Manifest 'name' Field") {
        TempDir solix_home;

        auto pkg_dir = sandbox.path() / "name_field_pkg";
        std::filesystem::create_directories(pkg_dir / "src");
        nlohmann::json pm;
        pm["project"] = "name_lib";
        pm["version"] = "1.5.0";
        pm["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/lib.slx"}}
        });
        pm["profiles"]["debug"]["output_directory"] = "build/debug";
        pm["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        pm["profiles"]["debug"]["compilation"]["entry_point"] = "main";
        pm["profiles"]["debug"]["compilation"]["multithreaded"] = false;

        std::ofstream pf(pkg_dir / "solix.json");
        pf << pm.dump(2);
        pf.close();

        std::ofstream ps(pkg_dir / "src/lib.slx");
        ps << "class NameLib { public static int32 val() { return 77; } }\n";
        ps.close();

        auto inst_res = run_cli({"install", pkg_dir.string()}, solix_home.path());
        CHECK(inst_res.exit_code == 0);

        auto app_dir = sandbox.path() / "consumer_name_app";
        std::filesystem::create_directories(app_dir / "src");
        nlohmann::json app_m;
        app_m["project"] = "consumer_name_app";
        app_m["version"] = "1.0.0";
        app_m["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/main.slx"}},
            {{"type", "project"}, {"name", "name_lib@1.5.0"}}
        });
        app_m["profiles"]["debug"]["output_directory"] = "build/debug";
        app_m["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        app_m["profiles"]["debug"]["compilation"]["entry_point"] = "main";
        app_m["profiles"]["debug"]["compilation"]["multithreaded"] = false;

        std::ofstream af(app_dir / "solix.json");
        af << app_m.dump(2);
        af.close();

        std::ofstream as(app_dir / "src/main.slx");
        as << "static int32 main() { return NameLib.val() == 77 ? 0 : 1; }\n";
        as.close();

        auto build_res = run_cli({"build", app_dir.string()}, solix_home.path());
        CHECK(build_res.exit_code == 0);

        auto run_res = run_cli({"run", (app_dir / "build/debug/out.slxbin").string()}, solix_home.path());
        CHECK(run_res.exit_code == 0);
    }

    SECTION("Positive - Case 3.22: Build Project with Combined name@version in Manifest 'package' Field") {
        TempDir solix_home;

        auto pkg_dir = sandbox.path() / "pkg_field_pkg";
        std::filesystem::create_directories(pkg_dir / "src");
        nlohmann::json pm;
        pm["project"] = "pkg_lib";
        pm["version"] = "3.2.1";
        pm["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/lib.slx"}}
        });
        pm["profiles"]["debug"]["output_directory"] = "build/debug";
        pm["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        pm["profiles"]["debug"]["compilation"]["entry_point"] = "main";
        pm["profiles"]["debug"]["compilation"]["multithreaded"] = false;

        std::ofstream pf(pkg_dir / "solix.json");
        pf << pm.dump(2);
        pf.close();

        std::ofstream ps(pkg_dir / "src/lib.slx");
        ps << "class PkgLib { public static int32 val() { return 88; } }\n";
        ps.close();

        auto inst_res = run_cli({"install", pkg_dir.string()}, solix_home.path());
        CHECK(inst_res.exit_code == 0);

        auto app_dir = sandbox.path() / "consumer_pkg_app";
        std::filesystem::create_directories(app_dir / "src");
        nlohmann::json app_m;
        app_m["project"] = "consumer_pkg_app";
        app_m["version"] = "1.0.0";
        app_m["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/main.slx"}},
            {{"type", "project"}, {"package", "pkg_lib@3.2.1"}}
        });
        app_m["profiles"]["debug"]["output_directory"] = "build/debug";
        app_m["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        app_m["profiles"]["debug"]["compilation"]["entry_point"] = "main";
        app_m["profiles"]["debug"]["compilation"]["multithreaded"] = false;

        std::ofstream af(app_dir / "solix.json");
        af << app_m.dump(2);
        af.close();

        std::ofstream as(app_dir / "src/main.slx");
        as << "static int32 main() { return PkgLib.val() == 88 ? 0 : 1; }\n";
        as.close();

        auto build_res = run_cli({"build", app_dir.string()}, solix_home.path());
        CHECK(build_res.exit_code == 0);

        auto run_res = run_cli({"run", (app_dir / "build/debug/out.slxbin").string()}, solix_home.path());
        CHECK(run_res.exit_code == 0);
    }

    SECTION("Negative - Case 3.23: Build Non-Existent Installed Package name@version") {
        TempDir solix_home;
        auto res = run_cli({"build", "ghost_pkg@9.9.9"}, solix_home.path());
        CHECK(res.exit_code != 0);
        CHECK(res.err.find("Installed project 'ghost_pkg' with version '9.9.9' not found") != std::string::npos);
    }

    SECTION("Negative - Case 3.24: Build with Legacy -m Flag Reports CLI Error") {
        auto proj_dir = sandbox.path() / "legacy_flag_proj";
        std::filesystem::create_directories(proj_dir / "src");
        std::ofstream mf(proj_dir / "solix.json");
        mf << create_sample_manifest();
        mf.close();

        auto res = run_cli({"build", "-m", (proj_dir / "solix.json").string()});
        CHECK(res.exit_code != 0);
    }
}

