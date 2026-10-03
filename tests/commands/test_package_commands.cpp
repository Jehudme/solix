#include <catch2/catch_all.hpp>
#include "cli_test_helper.hpp"
#include "package_manager.hpp"
#include <nlohmann/json.hpp>

using namespace solix::test;
using namespace solix::cli;

static std::filesystem::path create_test_project(TempDir& sandbox,
                                                 const std::string& folder_name,
                                                 const std::string& name,
                                                 const std::string& version,
                                                 const std::string& author = "Tester <test@solix.org>",
                                                 const std::string& desc = "A test project") {
    auto proj_dir = sandbox.path() / folder_name;
    std::filesystem::create_directories(proj_dir / "src");

    nlohmann::json manifest;
    manifest["project"] = name;
    manifest["version"] = version;
    manifest["author"] = author;
    manifest["description"] = desc;
    manifest["license"] = "MIT";
    manifest["dependencies"] = nlohmann::json::array({
        {{"type", "source"}, {"path", "src/main.slx"}}
    });
    manifest["profiles"]["debug"]["output_directory"] = "build/debug";
    manifest["profiles"]["debug"]["exe_filename"] = "out.slxbin";
    manifest["profiles"]["debug"]["compilation"]["entry_point"] = "main";
    manifest["profiles"]["debug"]["compilation"]["multithreaded"] = false;

    std::ofstream mf(proj_dir / "solix.json");
    mf << manifest.dump(2);
    mf.close();

    std::ofstream sf(proj_dir / "src/main.slx");
    sf << "static int32 main() { return 0; }\n";
    sf.close();

    return proj_dir;
}

TEST_CASE("CLI Command - package management (install, uninstall, list, details)", "[command][package]") {
    TempDir solix_home;
    TempDir proj_workspace;

    SECTION("Positive - Case 5.1 & 5.2: Install Valid Project with Deterministic SHA-256 Hash ID") {
        auto proj = create_test_project(proj_workspace, "pkg_one", "alpha_pkg", "1.0.0");
        std::string expected_id = PackageManager::compute_project_id("alpha_pkg", "1.0.0");

        auto res = run_cli({"install", proj.string()}, solix_home.path());

        CHECK(res.exit_code == 0);
        CHECK(res.out.find("Successfully installed project") != std::string::npos);
        CHECK(res.out.find(expected_id) != std::string::npos);

        // Verify registry file exists and contains the project
        CHECK(solix_home.exists("installed.json"));
        std::ifstream rf(solix_home.path() / "installed.json");
        nlohmann::json registry;
        rf >> registry;
        CHECK(registry.contains("projects"));
        CHECK(registry["projects"].contains(expected_id));
        CHECK(registry["projects"][expected_id]["name"] == "alpha_pkg");
        CHECK(registry["projects"][expected_id]["version"] == "1.0.0");

        // Verify installed directory contains project files
        CHECK(solix_home.exists("installed/" + expected_id + "/solix.json"));
        CHECK(solix_home.exists("installed/" + expected_id + "/src/main.slx"));
    }

    SECTION("Positive - Case 5.3: Overwrite Existing Installation with Force Flag") {
        auto proj = create_test_project(proj_workspace, "pkg_force", "beta_pkg", "1.2.0");

        // First install
        auto res1 = run_cli({"install", proj.string()}, solix_home.path());
        CHECK(res1.exit_code == 0);

        // Reinstall with --force
        auto res2 = run_cli({"install", proj.string(), "--force"}, solix_home.path());
        CHECK(res2.exit_code == 0);
        CHECK(res2.out.find("Successfully installed project") != std::string::npos);
    }

    SECTION("Negative - Case 5.4: Target Directory Does Not Exist") {
        auto missing = proj_workspace.path() / "non_existent_folder";
        auto res = run_cli({"install", missing.string()}, solix_home.path());

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("solix.json not found") != std::string::npos);
    }

    SECTION("Negative - Case 5.5: Target Directory Missing solix.json") {
        auto empty_dir = proj_workspace.path() / "empty_dir";
        std::filesystem::create_directories(empty_dir);

        auto res = run_cli({"install", empty_dir.string()}, solix_home.path());

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("solix.json not found") != std::string::npos);
    }

    SECTION("Negative - Case 5.6: Reinstall Existing Project Without Force Flag") {
        auto proj = create_test_project(proj_workspace, "pkg_dup", "gamma_pkg", "1.0.0");

        auto res1 = run_cli({"install", proj.string()}, solix_home.path());
        CHECK(res1.exit_code == 0);

        auto res2 = run_cli({"install", proj.string()}, solix_home.path());
        CHECK(res2.exit_code != 0);
        CHECK(res2.err.find("already installed") != std::string::npos);
        CHECK(res2.err.find("--force") != std::string::npos);
    }

    SECTION("Positive - Case 5.7 & 5.8: Install Solix Standard Library Project (solixlib) with Native Companion") {
        std::filesystem::path solixlib_path = std::filesystem::path(SOLIX_PROJECT_ROOT) / "solixlib" / "project";
        REQUIRE(std::filesystem::exists(solixlib_path / "solix.json"));

        std::string expected_id = PackageManager::compute_project_id("solixlib", "0.1.0");

        auto res = run_cli({"install", solixlib_path.string()}, solix_home.path());
        CHECK(res.exit_code == 0);
        CHECK(res.out.find("Successfully installed project") != std::string::npos);
        CHECK(res.out.find(expected_id) != std::string::npos);

        // Verify installed directory and native companion lib/
        auto installed_dir = solix_home.path() / "installed" / expected_id;
        CHECK(std::filesystem::exists(installed_dir / "solix.json"));
        CHECK(std::filesystem::exists(installed_dir / "lib"));

        // Verify list shows solixlib
        auto list_res = run_cli({"list"}, solix_home.path());
        CHECK(list_res.exit_code == 0);
        CHECK(list_res.out.find("solixlib") != std::string::npos);
        CHECK(list_res.out.find("0.1.0") != std::string::npos);
    }

    SECTION("Positive - Case 5.9: Consumer Project Builds and Runs with Installed Standard Library (solixlib)") {
        std::filesystem::path solixlib_path = std::filesystem::path(SOLIX_PROJECT_ROOT) / "solixlib" / "project";
        REQUIRE(std::filesystem::exists(solixlib_path / "solix.json"));

        // Install solixlib
        auto inst_res = run_cli({"install", solixlib_path.string()}, solix_home.path());
        REQUIRE(inst_res.exit_code == 0);

        // Create consumer project referencing solixlib
        auto consumer_dir = proj_workspace.path() / "consumer_app";
        std::filesystem::create_directories(consumer_dir / "src");

        nlohmann::json manifest;
        manifest["project"] = "consumer_app";
        manifest["version"] = "1.0.0";
        manifest["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/main.slx"}},
            {{"type", "project"}, {"package", "solixlib@0.1.0"}}
        });
        manifest["profiles"]["debug"]["output_directory"] = "build/debug";
        manifest["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        manifest["profiles"]["debug"]["compilation"]["entry_point"] = "main";
        manifest["profiles"]["debug"]["compilation"]["multithreaded"] = false;

        std::ofstream mf(consumer_dir / "solix.json");
        mf << manifest.dump(2);
        mf.close();

        std::ofstream sf(consumer_dir / "src/main.slx");
        sf << "import solix.core.Internal;\n"
           << "static int32 main() {\n"
           << "    return Internal.version() == 1 ? 0 : 1;\n"
           << "}\n";
        sf.close();

        auto run_res = run_cli({"run", consumer_dir.string()}, solix_home.path());
        CHECK(run_res.exit_code == 0);
    }



    SECTION("Negative - Case 5.10: Reinstall solixlib Without Force Flag Fails") {
        std::filesystem::path solixlib_path = std::filesystem::path(SOLIX_PROJECT_ROOT) / "solixlib" / "project";
        REQUIRE(std::filesystem::exists(solixlib_path / "solix.json"));

        auto res1 = run_cli({"install", solixlib_path.string()}, solix_home.path());
        REQUIRE(res1.exit_code == 0);

        auto res2 = run_cli({"install", solixlib_path.string()}, solix_home.path());
        CHECK(res2.exit_code != 0);
        CHECK(res2.err.find("already installed") != std::string::npos);
        CHECK(res2.err.find("--force") != std::string::npos);
    }

    SECTION("Positive - Case 6.1: Uninstall Successfully Removes Project and Directory") {
        auto proj = create_test_project(proj_workspace, "pkg_uninst", "del_pkg", "1.0.0");
        std::string id = PackageManager::compute_project_id("del_pkg", "1.0.0");

        auto inst_res = run_cli({"install", proj.string()}, solix_home.path());
        CHECK(inst_res.exit_code == 0);
        CHECK(solix_home.exists("installed/" + id));

        auto uninst_res = run_cli({"uninstall", "del_pkg", "1.0.0"}, solix_home.path());
        CHECK(uninst_res.exit_code == 0);
        CHECK(uninst_res.out.find("Successfully uninstalled 'del_pkg' version '1.0.0'") != std::string::npos);

        // Verify directory and registry entry are deleted
        CHECK(!solix_home.exists("installed/" + id));
        std::ifstream rf(solix_home.path() / "installed.json");
        nlohmann::json registry;
        rf >> registry;
        CHECK(!registry["projects"].contains(id));
    }

    SECTION("Negative - Case 6.2: Missing Required Name or Version Arguments") {
        auto res = run_cli({"uninstall", "only_name"}, solix_home.path());

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("version is required") != std::string::npos);
    }

    SECTION("Negative - Case 6.3: Package Not Found in Registry") {
        auto res = run_cli({"uninstall", "unknown_pkg", "9.9.9"}, solix_home.path());

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("No installed project found matching") != std::string::npos);
    }

    SECTION("Positive - Case 7.1: Empty Installation Registry") {
        auto res = run_cli({"list"}, solix_home.path());

        CHECK(res.exit_code == 0);
        CHECK(res.out.find("No Solix projects are currently installed") != std::string::npos);
    }

    SECTION("Positive - Case 7.2: List Multiple Installed Projects") {
        auto p1 = create_test_project(proj_workspace, "p1", "first_lib", "1.0.0");
        auto p2 = create_test_project(proj_workspace, "p2", "second_lib", "2.0.0");

        run_cli({"install", p1.string()}, solix_home.path());
        run_cli({"install", p2.string()}, solix_home.path());

        auto res = run_cli({"list"}, solix_home.path());

        CHECK(res.exit_code == 0);
        CHECK(res.out.find("NAME") != std::string::npos);
        CHECK(res.out.find("VERSION") != std::string::npos);
        CHECK(res.out.find("ID") != std::string::npos);
        CHECK(res.out.find("first_lib") != std::string::npos);
        CHECK(res.out.find("1.0.0") != std::string::npos);
        CHECK(res.out.find("second_lib") != std::string::npos);
        CHECK(res.out.find("2.0.0") != std::string::npos);
    }

    SECTION("Positive - Case 8.1: Extensive Details by Name and Version") {
        auto proj = create_test_project(proj_workspace, "detail_proj", "detail_pkg", "3.0.0",
                                        "Bob <bob@test.com>", "An extensive test lib");
        std::string id = PackageManager::compute_project_id("detail_pkg", "3.0.0");

        run_cli({"install", proj.string()}, solix_home.path());

        auto res = run_cli({"details", "detail_pkg", "3.0.0"}, solix_home.path());

        CHECK(res.exit_code == 0);
        CHECK(res.out.find("Project:") != std::string::npos);
        CHECK(res.out.find("detail_pkg") != std::string::npos);
        CHECK(res.out.find("3.0.0") != std::string::npos);
        CHECK(res.out.find(id) != std::string::npos);
        CHECK(res.out.find("Bob <bob@test.com>") != std::string::npos);
        CHECK(res.out.find("An extensive test lib") != std::string::npos);
        CHECK(res.out.find("MIT") != std::string::npos);
        CHECK(res.out.find("Defined Profiles:") != std::string::npos);
        CHECK(res.out.find("debug") != std::string::npos);
        CHECK(res.out.find("Dependencies (") != std::string::npos);
    }

    SECTION("Positive - Case 8.2: Details by Name When Single Version Installed") {
        auto proj = create_test_project(proj_workspace, "single_ver_proj", "unique_pkg", "1.0.0");

        run_cli({"install", proj.string()}, solix_home.path());

        auto res = run_cli({"details", "unique_pkg"}, solix_home.path());

        CHECK(res.exit_code == 0);
        CHECK(res.out.find("Project:") != std::string::npos);
        CHECK(res.out.find("unique_pkg") != std::string::npos);
        CHECK(res.out.find("1.0.0") != std::string::npos);
    }

    SECTION("Negative - Case 8.3: Project Not Found by Name") {
        auto res = run_cli({"details", "ghost_pkg"}, solix_home.path());

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("No installed project found with name 'ghost_pkg'") != std::string::npos);
    }

    SECTION("Negative - Case 8.4: Specific Version Not Found") {
        auto proj = create_test_project(proj_workspace, "v_find_proj", "ver_pkg", "1.0.0");
        run_cli({"install", proj.string()}, solix_home.path());

        auto res = run_cli({"details", "ver_pkg", "2.0.0"}, solix_home.path());

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("No installed project found for 'ver_pkg' version '2.0.0'") != std::string::npos);
    }

    SECTION("Negative - Case 8.5: Multiple Versions Installed and Version Omitted") {
        auto p1 = create_test_project(proj_workspace, "multi_v1", "multi_pkg", "1.0.0");
        auto p2 = create_test_project(proj_workspace, "multi_v2", "multi_pkg", "2.0.0");

        run_cli({"install", p1.string()}, solix_home.path());
        run_cli({"install", p2.string()}, solix_home.path());

        auto res = run_cli({"details", "multi_pkg"}, solix_home.path());

        // Should advise specifying a version
        CHECK(res.out.find("Multiple versions installed for 'multi_pkg'") != std::string::npos);
        CHECK(res.out.find("1.0.0") != std::string::npos);
        CHECK(res.out.find("2.0.0") != std::string::npos);
    }
}
