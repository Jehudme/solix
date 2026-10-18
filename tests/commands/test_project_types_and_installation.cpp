#include <catch2/catch_all.hpp>
#include "cli_test_helper.hpp"
#include "package_manager.hpp"
#include <nlohmann/json.hpp>

using namespace solix::test;
using namespace solix::cli;

TEST_CASE("CLI Command - Project Types, Clean Installation, Archive & Discovery", "[command][package][types]") {

    SECTION("Positive - Case 28.1: Scaffolding Library vs Application Projects") {
        TempDir sandbox;
        auto lib_dir = sandbox.path() / "my_scaffold_lib";
        auto app_dir = sandbox.path() / "my_scaffold_app";

        auto res_lib = run_cli({"new", lib_dir.string(), "-T", "library"});
        CHECK(res_lib.exit_code == 0);
        CHECK(std::filesystem::exists(lib_dir / "manifest.json"));
        CHECK(std::filesystem::exists(lib_dir / "src/lib.slx"));
        CHECK(!std::filesystem::exists(lib_dir / "src/main.slx"));

        std::ifstream lf(lib_dir / "manifest.json");
        nlohmann::json lib_mf;
        lf >> lib_mf;
        CHECK(lib_mf.value("type", "") == "library");

        auto res_app = run_cli({"new", app_dir.string(), "-T", "app"});
        CHECK(res_app.exit_code == 0);
        CHECK(std::filesystem::exists(app_dir / "manifest.json"));
        CHECK(std::filesystem::exists(app_dir / "src/main.slx"));

        std::ifstream af(app_dir / "manifest.json");
        nlohmann::json app_mf;
        af >> app_mf;
        CHECK(app_mf.value("type", "") == "app");
    }

    SECTION("Positive - Case 28.2: Automatic Recursive .slx Source Discovery") {
        TempDir sandbox;
        auto proj_dir = sandbox.path() / "nested_proj";
        std::filesystem::create_directories(proj_dir / "src/math");
        std::filesystem::create_directories(proj_dir / "src/core");

        nlohmann::json manifest;
        manifest["project"] = "nested_proj";
        manifest["version"] = "1.0.0";
        manifest["type"] = "app";
        // dependencies contains NO individual source listings
        manifest["dependencies"] = nlohmann::json::array();
        manifest["profiles"]["debug"]["output_directory"] = "build/debug";
        manifest["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        manifest["profiles"]["debug"]["compilation"]["entry_point"] = "main";
        manifest["profiles"]["debug"]["compilation"]["multithreaded"] = false;

        std::ofstream mf(proj_dir / "solix.json");
        mf << manifest.dump(2);
        mf.close();

        std::ofstream mf_src(proj_dir / "src/main.slx");
        mf_src << "import nested_proj.math.Calc;\n"
               << "import nested_proj.core.Greet;\n"
               << "int32 main() {\n"
               << "    if (Greet.greet_code() != 7) return 1;\n"
               << "    return Calc.add(20, 22) == 42 ? 0 : 2;\n"
               << "}\n";
        mf_src.close();

        std::ofstream c_src(proj_dir / "src/math/Calc.slx");
        c_src << "package nested_proj.math;\n"
              << "public class Calc {\n"
              << "    public static int32 add(int32 a, int32 b) { return a + b; }\n"
              << "}\n";
        c_src.close();

        std::ofstream g_src(proj_dir / "src/core/Greet.slx");
        g_src << "package nested_proj.core;\n"
              << "public class Greet {\n"
              << "    public static int32 greet_code() { return 7; }\n"
              << "}\n";
        g_src.close();

        auto build_res = run_cli({"build", proj_dir.string()});
        CHECK(build_res.exit_code == 0);
        CHECK(std::filesystem::exists(proj_dir / "build/debug/out.slxbin"));

        auto run_res = run_cli({"run", proj_dir.string()});
        CHECK(run_res.exit_code == 0);
    }

    SECTION("Positive - Case 28.3: Clean Library Installation and Reduced Manifest") {
        TempDir sandbox;
        TempDir solix_home;
        auto lib_dir = sandbox.path() / "my_clean_lib";
        std::filesystem::create_directories(lib_dir / "src/sub");
        std::filesystem::create_directories(lib_dir / "resources");

        nlohmann::json manifest;
        manifest["project"] = "my_clean_lib";
        manifest["version"] = "1.5.0";
        manifest["type"] = "library";
        manifest["author"] = "Lib Author";
        manifest["description"] = "A clean library";
        manifest["profiles"]["release"]["output_directory"] = "build/release";
        manifest["profiles"]["release"]["exe_filename"] = "lib.slxbin";
        manifest["profiles"]["release"]["compilation"]["entry_point"] = "lib_init";
        manifest["commands"]["pre_compile"] = "echo pre";

        std::ofstream mf(lib_dir / "solix.json");
        mf << manifest.dump(2);
        mf.close();

        std::ofstream sf1(lib_dir / "src/sub/helper.slx");
        sf1 << "package my_clean_lib.sub;\npublic class Helper { public static int32 val() { return 100; } }\n";
        sf1.close();

        // Optional test entry point inside library that should be stripped on install
        std::ofstream sf_main(lib_dir / "src/main.slx");
        sf_main << "int32 lib_init() { return 0; }\n";
        sf_main.close();

        std::ofstream rf(lib_dir / "resources/data.txt");
        rf << "sample data";
        rf.close();

        auto inst_res = run_cli({"install", lib_dir.string(), "-p", "release"}, solix_home.path());
        CHECK(inst_res.exit_code == 0);

        std::string lib_id = PackageManager::compute_project_id("my_clean_lib", "1.5.0");
        auto installed_path = solix_home.path() / "installed" / lib_id;
        CHECK(std::filesystem::exists(installed_path));
        CHECK(std::filesystem::exists(installed_path / "src/sub/helper.slx"));
        CHECK(!std::filesystem::exists(installed_path / "src/main.slx"));
        CHECK(std::filesystem::exists(installed_path / "resources/data.txt"));
        CHECK(!std::filesystem::exists(installed_path / "bytecode.slxbin"));

        // Verify reduced manifest
        std::ifstream imf(installed_path / "solix.json");
        nlohmann::json red_mf;
        imf >> red_mf;
        CHECK(red_mf["installed"] == true);
        CHECK(red_mf["type"] == "library");
        CHECK(!red_mf.contains("profiles"));
        CHECK(!red_mf.contains("commands"));
    }

    SECTION("Positive - Case 28.4: Clean Application Installation and Direct Execution") {
        TempDir sandbox;
        TempDir solix_home;
        auto app_dir = sandbox.path() / "clean_app";
        std::filesystem::create_directories(app_dir / "src");
        std::filesystem::create_directories(app_dir / "resources");

        nlohmann::json manifest;
        manifest["project"] = "clean_app";
        manifest["version"] = "2.0.0";
        manifest["type"] = "app";
        manifest["profiles"]["release"]["output_directory"] = "build/release";
        manifest["profiles"]["release"]["exe_filename"] = "out.slxbin";
        manifest["profiles"]["release"]["compilation"]["entry_point"] = "main";
        manifest["profiles"]["release"]["compilation"]["multithreaded"] = false;

        std::ofstream mf(app_dir / "solix.json");
        mf << manifest.dump(2);
        mf.close();

        std::ofstream sf(app_dir / "src/main.slx");
        sf << "int32 main() {\n"
           << "    return 0;\n"
           << "}\n";
        sf.close();

        std::ofstream rf(app_dir / "resources/app_cfg.json");
        rf << "{\"status\":\"ok\"}";
        rf.close();

        auto inst_res = run_cli({"install", app_dir.string(), "-p", "release"}, solix_home.path());
        CHECK(inst_res.exit_code == 0);

        std::string app_id = PackageManager::compute_project_id("clean_app", "2.0.0");
        auto installed_path = solix_home.path() / "installed" / app_id;
        CHECK(std::filesystem::exists(installed_path));
        CHECK(std::filesystem::exists(installed_path / "bytecode.slxbin"));
        CHECK(!std::filesystem::exists(installed_path / "src"));
        CHECK(std::filesystem::exists(installed_path / "resources/app_cfg.json"));

        // Direct execution of the installed application
        auto run_res = run_cli({"run", "clean_app@2.0.0"}, solix_home.path());
        CHECK(run_res.exit_code == 0);

        // Also test direct execution via installed directory path
        auto run_res_dir = run_cli({"run", installed_path.string()}, solix_home.path());
        CHECK(run_res_dir.exit_code == 0);
    }

    SECTION("Positive - Case 28.5: Standalone Staging Export via solix archive and solix install -o") {
        TempDir sandbox;
        TempDir solix_home;
        auto lib_dir = sandbox.path() / "arch_lib";
        std::filesystem::create_directories(lib_dir / "src");

        nlohmann::json manifest;
        manifest["project"] = "arch_lib";
        manifest["version"] = "1.0.0";
        manifest["type"] = "library";
        manifest["profiles"]["debug"]["output_directory"] = "build/debug";
        manifest["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        manifest["profiles"]["debug"]["compilation"]["entry_point"] = "main";

        std::ofstream mf(lib_dir / "solix.json");
        mf << manifest.dump(2);
        mf.close();

        std::ofstream sf(lib_dir / "src/util.slx");
        sf << "class Util {}\n";
        sf.close();

        auto out_dir = sandbox.path() / "exported_lib";
        auto arch_res = run_cli({"archive", lib_dir.string(), "-o", out_dir.string()}, solix_home.path());
        CHECK(arch_res.exit_code == 0);
        CHECK(std::filesystem::exists(out_dir / "src/util.slx"));
        CHECK((std::filesystem::exists(out_dir / "solix.json") || std::filesystem::exists(out_dir / "manifest.json")));

        // Registry should NOT have registered this export
        CHECK(!solix_home.exists("installed/" + PackageManager::compute_project_id("arch_lib", "1.0.0")));

        // Also test install -o
        auto out_dir2 = sandbox.path() / "exported_via_install";
        auto inst_o_res = run_cli({"install", lib_dir.string(), "-o", out_dir2.string()}, solix_home.path());
        CHECK(inst_o_res.exit_code == 0);
        CHECK(std::filesystem::exists(out_dir2 / "src/util.slx"));
    }

    SECTION("Positive - Case 28.6: Fast Zero-Rebuild Installation via --as-is") {
        TempDir sandbox;
        TempDir solix_home;
        auto pre_dir = sandbox.path() / "prepared_pkg";
        std::filesystem::create_directories(pre_dir / "src");

        nlohmann::json manifest;
        manifest["project"] = "prepared_pkg";
        manifest["version"] = "3.2.1";
        manifest["type"] = "library";
        manifest["installed"] = true;

        std::ofstream mf(pre_dir / "manifest.json");
        mf << manifest.dump(2);
        mf.close();

        std::ofstream sf(pre_dir / "src/mod.slx");
        sf << "class Mod {}\n";
        sf.close();

        auto res = run_cli({"install", pre_dir.string(), "--as-is"}, solix_home.path());
        CHECK(res.exit_code == 0);
        CHECK(res.out.find("Installing pre-packaged distribution as-is") != std::string::npos);

        std::string id = PackageManager::compute_project_id("prepared_pkg", "3.2.1");
        CHECK(solix_home.exists("installed/" + id));
        CHECK(solix_home.exists("installed/" + id + "/src/mod.slx"));
    }

    SECTION("Negative - Case 28.7: Reject App Project as a Dependency") {
        TempDir sandbox;
        auto app_dep = sandbox.path() / "dep_app";
        std::filesystem::create_directories(app_dep / "src");

        nlohmann::json app_mf;
        app_mf["project"] = "dep_app";
        app_mf["version"] = "1.0.0";
        app_mf["type"] = "app";
        app_mf["profiles"]["debug"]["output_directory"] = "build/debug";
        app_mf["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        app_mf["profiles"]["debug"]["compilation"]["entry_point"] = "main";

        std::ofstream amf(app_dep / "solix.json");
        amf << app_mf.dump(2);
        amf.close();

        std::ofstream asf(app_dep / "src/main.slx");
        asf << "int32 main() { return 0; }\n";
        asf.close();

        // Consumer project attempting to depend on dep_app
        auto consumer = sandbox.path() / "consumer_proj";
        std::filesystem::create_directories(consumer / "src");

        nlohmann::json cons_mf;
        cons_mf["project"] = "consumer_proj";
        cons_mf["version"] = "1.0.0";
        cons_mf["type"] = "app";
        cons_mf["dependencies"] = nlohmann::json::array({
            {{"type", "library"}, {"name", "dep_app"}, {"path", app_dep.string()}}
        });
        cons_mf["profiles"]["debug"]["output_directory"] = "build/debug";
        cons_mf["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        cons_mf["profiles"]["debug"]["compilation"]["entry_point"] = "main";

        std::ofstream cmf(consumer / "solix.json");
        cmf << cons_mf.dump(2);
        cmf.close();

        std::ofstream csf(consumer / "src/main.slx");
        csf << "int32 main() { return 0; }\n";
        csf.close();

        auto res = run_cli({"build", consumer.string()});
        CHECK(res.exit_code != 0);
        CHECK(res.err.find("Cannot include project 'dep_app' as a dependency because it is an 'app'") != std::string::npos);
    }

    SECTION("Negative - Case 28.8: Invalid Project Type Rejection in solix new") {
        TempDir sandbox;
        auto bad_dir = sandbox.path() / "bad_proj";
        auto res = run_cli({"new", bad_dir.string(), "-T", "invalid_type"});
        CHECK(res.exit_code != 0);
        CHECK(res.err.find("Invalid project type 'invalid_type'") != std::string::npos);
    }
}
