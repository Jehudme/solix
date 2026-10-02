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

    SECTION("Positive - Case 3.3: Custom Manifest Path via Flag") {
        auto proj_dir = sandbox.path() / "nested_proj";
        std::filesystem::create_directories(proj_dir / "src");

        auto manifest_path = proj_dir / "solix.json";
        std::ofstream mf(manifest_path);
        mf << create_sample_manifest();
        mf.close();

        std::ofstream sf(proj_dir / "src/main.slx");
        sf << "static int32 main() { return 0; }\n";
        sf.close();

        auto res = run_cli({"build", "-m", manifest_path.string()});

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
        auto res = run_cli({"build", "-m", missing.string()});

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
}
