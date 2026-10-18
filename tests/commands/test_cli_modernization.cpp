#include <catch2/catch_all.hpp>
#include "cli_test_helper.hpp"
#include "package_manager.hpp"
#include "solix/zip_utils.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

using namespace solix::test;
using namespace solix::cli;

TEST_CASE("CLI Command - Modernization, List Refinements, Pack Ingestion & Guards", "[command][modernization][qol]") {

    SECTION("Positive - Case 29.1: Scaffolding with solix create and solix new Alias with -T plugin Support") {
        TempDir sandbox;
        auto plugin_dir = sandbox.path() / "test_plugin";
        auto app_dir = sandbox.path() / "test_app_alias";

        auto res_plug = run_cli({"create", plugin_dir.string(), "-T", "plugin"});
        CHECK(res_plug.exit_code == 0);
        CHECK(std::filesystem::exists(plugin_dir / "manifest.json"));
        CHECK(std::filesystem::exists(plugin_dir / "src/plugin.slx"));

        std::ifstream pf(plugin_dir / "manifest.json");
        nlohmann::json plug_mf;
        pf >> plug_mf;
        CHECK(plug_mf.value("type", "") == "plugin");

        auto res_app = run_cli({"new", app_dir.string()});
        CHECK(res_app.exit_code == 0);
        CHECK(std::filesystem::exists(app_dir / "manifest.json"));
        CHECK(std::filesystem::exists(app_dir / "src/main.slx"));

        std::ifstream af(app_dir / "manifest.json");
        nlohmann::json app_mf;
        af >> app_mf;
        CHECK(app_mf.value("type", "") == "app");
    }

    SECTION("Positive - Case 29.2: solix list Displays TYPE Column with Filtering, Sorting, and JSON Output") {
        TempDir sandbox;
        TempDir solix_home;

        // Stage 1: App
        auto app_dir = sandbox.path() / "alpha_app";
        std::filesystem::create_directories(app_dir / "src");
        nlohmann::json app_mf;
        app_mf["project"] = "alpha_app";
        app_mf["version"] = "1.0.0";
        app_mf["type"] = "app";
        app_mf["profiles"]["release"]["output_directory"] = "build/release";
        app_mf["profiles"]["release"]["exe_filename"] = "out.slxbin";
        app_mf["profiles"]["release"]["compilation"]["entry_point"] = "main";
        std::ofstream(app_dir / "solix.json") << app_mf.dump(2);
        std::ofstream(app_dir / "src/main.slx") << "int32 main() { return 0; }\n";
        auto r1 = run_cli({"install", app_dir.string()}, solix_home.path());
        REQUIRE(r1.exit_code == 0);

        // Stage 2: Library
        auto lib_dir = sandbox.path() / "beta_lib";
        std::filesystem::create_directories(lib_dir / "src");
        nlohmann::json lib_mf;
        lib_mf["project"] = "beta_lib";
        lib_mf["version"] = "2.0.0";
        lib_mf["type"] = "library";
        lib_mf["profiles"]["release"]["output_directory"] = "build/release";
        std::ofstream(lib_dir / "solix.json") << lib_mf.dump(2);
        std::ofstream(lib_dir / "src/lib.slx") << "package beta_lib;\npublic class Lib {}\n";
        auto r2 = run_cli({"install", lib_dir.string()}, solix_home.path());
        REQUIRE(r2.exit_code == 0);

        // Stage 3: Plugin
        auto plug_dir = sandbox.path() / "gamma_plugin";
        std::filesystem::create_directories(plug_dir / "src");
        nlohmann::json plug_mf;
        plug_mf["project"] = "gamma_plugin";
        plug_mf["version"] = "0.5.0";
        plug_mf["type"] = "plugin";
        plug_mf["profiles"]["release"]["output_directory"] = "build/release";
        std::ofstream(plug_dir / "solix.json") << plug_mf.dump(2);
        std::ofstream(plug_dir / "src/plugin.slx") << "package gamma_plugin;\npublic class Plug {}\n";
        auto r3 = run_cli({"install", plug_dir.string()}, solix_home.path());
        REQUIRE(r3.exit_code == 0);

        // 1. Plain list: contains TYPE header, app, library, plugin
        auto list_res = run_cli({"list"}, solix_home.path());
        CHECK(list_res.exit_code == 0);
        CHECK(list_res.out.find("TYPE") != std::string::npos);
        CHECK(list_res.out.find("alpha_app") != std::string::npos);
        CHECK(list_res.out.find("beta_lib") != std::string::npos);
        CHECK(list_res.out.find("gamma_plugin") != std::string::npos);
        CHECK(list_res.out.find("library") != std::string::npos);
        CHECK(list_res.out.find("plugin") != std::string::npos);

        // 2. Type filter: -t app
        auto app_only_res = run_cli({"list", "-t", "app"}, solix_home.path());
        CHECK(app_only_res.exit_code == 0);
        CHECK(app_only_res.out.find("alpha_app") != std::string::npos);
        CHECK(app_only_res.out.find("beta_lib") == std::string::npos);
        CHECK(app_only_res.out.find("gamma_plugin") == std::string::npos);

        // 3. Sort by name descending
        auto rev_res = run_cli({"list", "-s", "name", "--reverse"}, solix_home.path());
        CHECK(rev_res.exit_code == 0);
        size_t p_gamma = rev_res.out.find("gamma_plugin");
        size_t p_beta = rev_res.out.find("beta_lib");
        size_t p_alpha = rev_res.out.find("alpha_app");
        CHECK(p_gamma != std::string::npos);
        CHECK(p_beta != std::string::npos);
        CHECK(p_alpha != std::string::npos);
        CHECK(p_gamma < p_beta);
        CHECK(p_beta < p_alpha);

        // 4. JSON output
        auto json_res = run_cli({"list", "--json"}, solix_home.path());
        CHECK(json_res.exit_code == 0);
        nlohmann::json j_list = nlohmann::json::parse(json_res.out);
        REQUIRE(j_list.is_array());
        CHECK(j_list.size() == 3);
        bool has_lib = false;
        for (const auto& item : j_list) {
            if (item.value("name", "") == "beta_lib") {
                has_lib = true;
                CHECK(item.value("type", "") == "library");
                CHECK(item.value("version", "") == "2.0.0");
            }
        }
        CHECK(has_lib);
    }

    SECTION("Positive - Case 29.3: solix details Scripting Query Flags") {
        TempDir sandbox;
        TempDir solix_home;

        auto lib_dir = sandbox.path() / "query_lib";
        std::filesystem::create_directories(lib_dir / "src");
        nlohmann::json lib_mf;
        lib_mf["project"] = "query_lib";
        lib_mf["version"] = "1.2.3";
        lib_mf["type"] = "library";
        lib_mf["description"] = "A library for querying";
        lib_mf["author"] = "Query Dev";
        lib_mf["license"] = "MIT";
        lib_mf["profiles"]["release"]["output_directory"] = "build/release";
        std::ofstream(lib_dir / "solix.json") << lib_mf.dump(2);
        std::ofstream(lib_dir / "src/lib.slx") << "package query_lib;\npublic class Lib {}\n";

        auto inst = run_cli({"install", lib_dir.string()}, solix_home.path());
        REQUIRE(inst.exit_code == 0);

        auto q_v = run_cli({"details", "query_lib", "-v"}, solix_home.path());
        CHECK(q_v.exit_code == 0);
        CHECK(q_v.out == "1.2.3\n");

        auto q_t = run_cli({"details", "query_lib", "-t"}, solix_home.path());
        CHECK(q_t.exit_code == 0);
        CHECK(q_t.out == "library\n");

        auto q_n = run_cli({"details", "query_lib", "-n"}, solix_home.path());
        CHECK(q_n.exit_code == 0);
        CHECK(q_n.out == "query_lib\n");

        auto q_d = run_cli({"details", "query_lib", "-d"}, solix_home.path());
        CHECK(q_d.exit_code == 0);
        CHECK(q_d.out == "A library for querying\n");

        auto q_j = run_cli({"details", "query_lib", "--json"}, solix_home.path());
        CHECK(q_j.exit_code == 0);
        nlohmann::json parsed = nlohmann::json::parse(q_j.out);
        CHECK(parsed["name"] == "query_lib");
        CHECK(parsed["version"] == "1.2.3");
        CHECK(parsed["type"] == "library");
        CHECK(parsed["author"] == "Query Dev");
        CHECK(parsed["license"] == "MIT");
    }

    SECTION("Positive - Case 29.4: solix details on .slxpac Archives and name@version Parsing") {
        TempDir sandbox;
        TempDir solix_home;

        auto app_dir = sandbox.path() / "pac_app";
        std::filesystem::create_directories(app_dir / "src");
        nlohmann::json app_mf;
        app_mf["project"] = "pac_app";
        app_mf["version"] = "3.2.1";
        app_mf["type"] = "app";
        app_mf["description"] = "Packaged app bundle test";
        app_mf["profiles"]["release"]["output_directory"] = "build/release";
        app_mf["profiles"]["release"]["exe_filename"] = "out.slxbin";
        app_mf["profiles"]["release"]["compilation"]["entry_point"] = "main";
        std::ofstream(app_dir / "solix.json") << app_mf.dump(2);
        std::ofstream(app_dir / "src/main.slx") << "int32 main() { return 0; }\n";

        auto pac_path = sandbox.path() / "pac_app.slxpac";
        auto pack_res = run_cli({"pack", app_dir.string(), "-o", pac_path.string()});
        REQUIRE(pack_res.exit_code == 0);
        REQUIRE(std::filesystem::exists(pac_path));

        // Details directly on .slxpac
        auto det_pac = run_cli({"details", pac_path.string(), "-t"});
        CHECK(det_pac.exit_code == 0);
        CHECK(det_pac.out == "app\n");

        auto det_pac_v = run_cli({"details", pac_path.string(), "-v"});
        CHECK(det_pac_v.exit_code == 0);
        CHECK(det_pac_v.out == "3.2.1\n");

        // Install and query via name@version
        auto inst_res = run_cli({"install", app_dir.string()}, solix_home.path());
        REQUIRE(inst_res.exit_code == 0);

        auto det_at = run_cli({"details", "pac_app@3.2.1", "-v"}, solix_home.path());
        CHECK(det_at.exit_code == 0);
        CHECK(det_at.out == "3.2.1\n");
    }

    SECTION("Positive - Case 29.5: Direct Installation of .slxpac Package Bundles via solix install") {
        TempDir sandbox;
        TempDir solix_home;

        auto app_dir = sandbox.path() / "installable_app";
        std::filesystem::create_directories(app_dir / "src");
        nlohmann::json app_mf;
        app_mf["project"] = "installable_app";
        app_mf["version"] = "1.0.0";
        app_mf["type"] = "app";
        app_mf["profiles"]["release"]["output_directory"] = "build/release";
        app_mf["profiles"]["release"]["exe_filename"] = "out.slxbin";
        app_mf["profiles"]["release"]["compilation"]["entry_point"] = "main";
        std::ofstream(app_dir / "solix.json") << app_mf.dump(2);
        std::ofstream(app_dir / "src/main.slx") << "int32 main() { return 0; }\n";

        auto pac_file = sandbox.path() / "installable_app.slxpac";
        auto pack_res = run_cli({"pack", app_dir.string(), "-o", pac_file.string()});
        REQUIRE(pack_res.exit_code == 0);

        // Install bundle directly
        auto inst_bundle = run_cli({"install", pac_file.string()}, solix_home.path());
        CHECK(inst_bundle.exit_code == 0);

        // Verify it is registered in PackageManager
        auto list_res = run_cli({"list", "--json"}, solix_home.path());
        CHECK(list_res.exit_code == 0);
        nlohmann::json j = nlohmann::json::parse(list_res.out);
        REQUIRE(j.size() == 1);
        CHECK(j[0]["name"] == "installable_app");
        CHECK(j[0]["version"] == "1.0.0");
        CHECK(j[0]["type"] == "app");

        // Verify executing installed app from bundle
        auto run_res = run_cli({"run", "installable_app@1.0.0"}, solix_home.path());
        CHECK(run_res.exit_code == 0);
    }

    SECTION("Positive - Case 29.6: Pack Installed App and Simplified Packaging with Embedded Manifest") {
        TempDir sandbox;
        TempDir solix_home;

        auto app_dir = sandbox.path() / "orig_app";
        std::filesystem::create_directories(app_dir / "src");
        nlohmann::json app_mf;
        app_mf["project"] = "orig_app";
        app_mf["version"] = "2.1.0";
        app_mf["type"] = "app";
        app_mf["profiles"]["release"]["output_directory"] = "build/release";
        app_mf["profiles"]["release"]["exe_filename"] = "out.slxbin";
        app_mf["profiles"]["release"]["compilation"]["entry_point"] = "main";
        std::ofstream(app_dir / "solix.json") << app_mf.dump(2);
        std::ofstream(app_dir / "src/main.slx") << "int32 main() { return 0; }\n";

        auto inst = run_cli({"install", app_dir.string()}, solix_home.path());
        REQUIRE(inst.exit_code == 0);

        auto exported_pac = sandbox.path() / "exported.slxpac";
        auto pack_installed = run_cli({"pack", "orig_app@2.1.0", "-o", exported_pac.string()}, solix_home.path());
        CHECK(pack_installed.exit_code == 0);
        CHECK(std::filesystem::exists(exported_pac));

        solix::ZipArchiveReader reader(exported_pac);
        CHECK(reader.is_valid());
        CHECK(reader.has_entry("manifest.json"));
        CHECK(reader.has_entry("bytecode.slxbin"));

        auto mf_str = reader.read_entry_as_string("manifest.json");
        REQUIRE(mf_str.has_value());
        nlohmann::json read_mf = nlohmann::json::parse(*mf_str);
        CHECK(read_mf["project"] == "orig_app");
        CHECK(read_mf["version"] == "2.1.0");
        CHECK(read_mf["type"] == "app");
    }

    SECTION("Positive - Case 29.7: Active Profile Additional Dependencies Overrides Root Dependency Profile") {
        TempDir sandbox;

        // Dependency with debug returning 10, optimized returning 42
        auto dep_dir = sandbox.path() / "dep_calc";
        std::filesystem::create_directories(dep_dir / "src");
        nlohmann::json dep_mf;
        dep_mf["project"] = "dep_calc";
        dep_mf["version"] = "1.0.0";
        dep_mf["type"] = "library";
        dep_mf["profiles"]["debug"]["output_directory"] = "build/debug";
        dep_mf["profiles"]["debug"]["compilation"]["additional_dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/debug_calc.slx"}}
        });
        dep_mf["profiles"]["optimized"]["output_directory"] = "build/opt";
        dep_mf["profiles"]["optimized"]["compilation"]["additional_dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/opt_calc.slx"}}
        });
        std::ofstream(dep_dir / "solix.json") << dep_mf.dump(2);
        std::ofstream(dep_dir / "src/debug_calc.slx") << "package dep_calc;\npublic class Calc { public static int32 result() { return 10; } }\n";
        std::ofstream(dep_dir / "src/opt_calc.slx") << "package dep_calc;\npublic class Calc { public static int32 result() { return 42; } }\n";

        // Consumer project: declares root dependency on dep_calc with profile: debug
        // BUT prod profile has additional_dependencies declaring dep_calc with profile: optimized
        auto root_dir = sandbox.path() / "consumer_app";
        std::filesystem::create_directories(root_dir / "src");
        nlohmann::json root_mf;
        root_mf["project"] = "consumer_app";
        root_mf["version"] = "1.0.0";
        root_mf["type"] = "app";
        root_mf["dependencies"] = nlohmann::json::array({
            {{"type", "library"}, {"path", dep_dir.string()}, {"profile", "debug"}}
        });
        root_mf["profiles"]["prod"]["output_directory"] = "build/prod";
        root_mf["profiles"]["prod"]["exe_filename"] = "out.slxbin";
        root_mf["profiles"]["prod"]["compilation"]["entry_point"] = "main";
        root_mf["profiles"]["prod"]["compilation"]["additional_dependencies"] = nlohmann::json::array({
            {{"type", "library"}, {"path", dep_dir.string()}, {"profile", "optimized"}}
        });
        std::ofstream(root_dir / "solix.json") << root_mf.dump(2);
        std::ofstream(root_dir / "src/main.slx") << "import dep_calc.Calc;\nint32 main() { return Calc.result() == 42 ? 0 : 1; }\n";

        auto build_res = run_cli({"build", root_dir.string(), "-p", "prod"});
        CHECK(build_res.exit_code == 0);

        auto run_res = run_cli({"run", root_dir.string(), "-P", "prod"});
        CHECK(run_res.exit_code == 0);
    }

    SECTION("Positive - Case 29.8: Staging Alias on solix archive and solix install -o") {
        TempDir sandbox;
        auto lib_dir = sandbox.path() / "orig_lib";
        std::filesystem::create_directories(lib_dir / "src");
        nlohmann::json lib_mf;
        lib_mf["project"] = "orig_lib";
        lib_mf["version"] = "1.0.0";
        lib_mf["type"] = "library";
        lib_mf["profiles"]["release"]["output_directory"] = "build/release";
        std::ofstream(lib_dir / "solix.json") << lib_mf.dump(2);
        std::ofstream(lib_dir / "src/lib.slx") << "package orig_lib;\npublic class Lib {}\n";

        auto dist_dir = sandbox.path() / "dist_renamed";
        auto arch_res = run_cli({"archive", lib_dir.string(), "-o", dist_dir.string(), "--alias", "custom_alias"});
        CHECK(arch_res.exit_code == 0);
        CHECK(std::filesystem::exists(dist_dir / "manifest.json"));

        std::ifstream mf(dist_dir / "manifest.json");
        nlohmann::json out_mf;
        mf >> out_mf;
        CHECK(out_mf["project"] == "custom_alias");
        CHECK(out_mf["name"] == "custom_alias");
    }

    SECTION("Negative - Case 29.9: Reject solix run on Library Projects") {
        TempDir sandbox;
        TempDir solix_home;

        auto lib_dir = sandbox.path() / "reject_run_lib";
        std::filesystem::create_directories(lib_dir / "src");
        nlohmann::json lib_mf;
        lib_mf["project"] = "reject_run_lib";
        lib_mf["version"] = "1.0.0";
        lib_mf["type"] = "library";
        lib_mf["profiles"]["release"]["output_directory"] = "build/release";
        std::ofstream(lib_dir / "solix.json") << lib_mf.dump(2);
        std::ofstream(lib_dir / "src/lib.slx") << "package reject_run_lib;\npublic class Lib {}\n";

        auto inst = run_cli({"install", lib_dir.string()}, solix_home.path());
        REQUIRE(inst.exit_code == 0);

        auto run_res = run_cli({"run", "reject_run_lib@1.0.0"}, solix_home.path());
        CHECK(run_res.exit_code != 0);
        CHECK(run_res.err.find("Cannot run 'reject_run_lib' because it is a library (libraries do not contain an executable entry point).") != std::string::npos);
    }

    SECTION("Negative - Case 29.10: Reject solix run on Library Package Bundle (.slxpac)") {
        TempDir sandbox;
        auto lib_dir = sandbox.path() / "test_lib_bundle";
        std::filesystem::create_directories(lib_dir / "src");
        nlohmann::json lib_mf;
        lib_mf["project"] = "test_lib_bundle";
        lib_mf["version"] = "1.0.0";
        lib_mf["type"] = "library";
        std::ofstream(lib_dir / "solix.json") << lib_mf.dump(2);
        std::ofstream(lib_dir / "src/lib.slx") << "package test_lib_bundle;\npublic class Lib {}\n";

        auto out_pac = sandbox.path() / "test_lib.slxpac";
        auto pack_res = run_cli({"pack", lib_dir.string(), "-o", out_pac.string()});
        CHECK(pack_res.exit_code == 0);
        CHECK(std::filesystem::exists(out_pac));

        // Details on library pack bundle works
        auto det_res = run_cli({"details", out_pac.string(), "-t"});
        CHECK(det_res.exit_code == 0);
        CHECK(det_res.out.find("library") != std::string::npos);

        // solix run on library bundle is rejected
        auto run_res = run_cli({"run", out_pac.string()});
        CHECK(run_res.exit_code != 0);
        CHECK(run_res.err.find("is a library (libraries do not contain an executable entry point)") != std::string::npos);
    }

    SECTION("Positive - Case 29.11: Warning Diagnostic When Running Pre-Built Target with Profile Option") {
        TempDir sandbox;
        TempDir solix_home;

        auto app_dir = sandbox.path() / "warn_app";
        std::filesystem::create_directories(app_dir / "src");
        nlohmann::json app_mf;
        app_mf["project"] = "warn_app";
        app_mf["version"] = "1.0.0";
        app_mf["type"] = "app";
        app_mf["profiles"]["release"]["output_directory"] = "build/release";
        app_mf["profiles"]["release"]["exe_filename"] = "out.slxbin";
        app_mf["profiles"]["release"]["compilation"]["entry_point"] = "main";
        std::ofstream(app_dir / "solix.json") << app_mf.dump(2);
        std::ofstream(app_dir / "src/main.slx") << "int32 main() { return 0; }\n";

        auto inst = run_cli({"install", app_dir.string()}, solix_home.path());
        REQUIRE(inst.exit_code == 0);

        auto run_warn = run_cli({"run", "warn_app@1.0.0", "-P", "debug"}, solix_home.path());
        CHECK(run_warn.exit_code == 0);
        CHECK(run_warn.err.find("Warning: Option --profile is ignored when running a pre-built package or binary.") != std::string::npos);
    }
}
