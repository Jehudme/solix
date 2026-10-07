#include <catch2/catch_all.hpp>
#include "cli_test_helper.hpp"
#include <nlohmann/json.hpp>

using namespace solix::test;

TEST_CASE("CLI Command - new", "[command][new]") {
    TempDir sandbox;

    SECTION("Positive - Case 4.1: Default Project Scaffolding") {
        auto proj_dir = sandbox.path() / "default_proj";
        auto res = run_cli({"new", proj_dir.string()});

        CHECK(res.exit_code == 0);
        CHECK(std::filesystem::exists(proj_dir / "solix.json"));
        CHECK(std::filesystem::exists(proj_dir / "src/main.slx"));

        // Verify solix.json contents
        std::ifstream mf(proj_dir / "solix.json");
        nlohmann::json manifest;
        mf >> manifest;
        CHECK(manifest["project"] == "default_proj");
        CHECK(manifest["version"] == "0.1.0");

        // Verify the generated project builds cleanly
        auto build_res = run_cli({"build", (proj_dir / "solix.json").string()});
        CHECK(build_res.exit_code == 0);
        CHECK(std::filesystem::exists(proj_dir / "build/debug/out.slxbin"));
    }

    SECTION("Positive - Case 4.2: Scaffolding with Custom Metadata Options") {
        auto proj_dir = sandbox.path() / "custom_proj";
        auto res = run_cli({
            "new", proj_dir.string(),
            "-n", "my_custom_package",
            "-v", "2.1.0",
            "-a", "Alice Dev <alice@example.com>",
            "-d", "Custom Solix utility library",
            "-l", "Apache-2.0",
            "-t", "math",
            "-t", "fast",
            "-e", "start"
        });

        CHECK(res.exit_code == 0);

        std::ifstream mf(proj_dir / "solix.json");
        nlohmann::json manifest;
        mf >> manifest;

        CHECK(manifest["project"] == "my_custom_package");
        CHECK(manifest["version"] == "2.1.0");
        CHECK(manifest["author"] == "Alice Dev <alice@example.com>");
        CHECK(manifest["description"] == "Custom Solix utility library");
        CHECK(manifest["license"] == "Apache-2.0");
        CHECK(manifest["tags"] == std::vector<std::string>{"math", "fast"});

        // Verify custom entry point in main.slx
        std::string src_content = sandbox.read_file("custom_proj/src/main.slx");
        CHECK(src_content.find("int32 start()") != std::string::npos);

        // Verify project builds cleanly with the custom entry point using directory path
        auto build_res = run_cli({"build", proj_dir.string()});
        CHECK(build_res.exit_code == 0);
    }

    SECTION("Positive - Case 4.3: Scaffolding into an Existing Empty Directory") {
        auto empty_dir = sandbox.path() / "pre_existing_empty";
        std::filesystem::create_directories(empty_dir);

        auto res = run_cli({"new", empty_dir.string()});

        CHECK(res.exit_code == 0);
        CHECK(std::filesystem::exists(empty_dir / "solix.json"));
        CHECK(std::filesystem::exists(empty_dir / "src/main.slx"));
    }

    SECTION("Positive - Case 4.4: Overwriting Non-Empty Directory with Force Flag") {
        auto occupied = sandbox.path() / "occupied_force";
        std::filesystem::create_directories(occupied);
        sandbox.create_file("occupied_force/dummy.txt", "old data");

        auto res = run_cli({"new", occupied.string(), "--force"});

        CHECK(res.exit_code == 0);
        CHECK(std::filesystem::exists(occupied / "solix.json"));
        CHECK(std::filesystem::exists(occupied / "dummy.txt"));
    }

    SECTION("Positive - Case 4.5: Scaffolding Using Custom Template Folder") {
        auto tmpl_dir = sandbox.path() / "custom_template";
        std::filesystem::create_directories(tmpl_dir / "src");

        // Create template solix.json
        nlohmann::json tmpl_manifest;
        tmpl_manifest["project"] = "TEMPLATE_NAME";
        tmpl_manifest["version"] = "0.0.1";
        tmpl_manifest["dependencies"] = nlohmann::json::array({
            {{"type", "source"}, {"path", "src/main.slx"}}
        });
        tmpl_manifest["profiles"]["debug"]["output_directory"] = "build/debug";
        tmpl_manifest["profiles"]["debug"]["exe_filename"] = "out.slxbin";
        tmpl_manifest["profiles"]["debug"]["compilation"]["entry_point"] = "main";
        tmpl_manifest["profiles"]["debug"]["compilation"]["multithreaded"] = false;

        std::ofstream mf(tmpl_dir / "solix.json");
        mf << tmpl_manifest.dump(2);
        mf.close();

        std::ofstream sf(tmpl_dir / "src/main.slx");
        sf << "int32 main() { return 0; }\n";
        sf.close();

        std::ofstream extra(tmpl_dir / "extra_file.txt");
        extra << "Custom template asset\n";
        extra.close();

        auto proj_dir = sandbox.path() / "templated_proj";
        auto res = run_cli({"new", proj_dir.string(), "--template", tmpl_dir.string()});

        CHECK(res.exit_code == 0);
        CHECK(std::filesystem::exists(proj_dir / "extra_file.txt"));
        CHECK(std::filesystem::exists(proj_dir / "solix.json"));

        std::ifstream out_mf(proj_dir / "solix.json");
        nlohmann::json manifest;
        out_mf >> manifest;
        CHECK(manifest["project"] == "templated_proj");
    }

    SECTION("Negative - Case 4.6: Missing Target Path Argument") {
        auto res = run_cli({"new"});

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("path is required") != std::string::npos);
    }

    SECTION("Negative - Case 4.7: Target Directory Already Exists and Non-Empty Without Force") {
        auto occupied = sandbox.path() / "occupied_noforce";
        std::filesystem::create_directories(occupied);
        sandbox.create_file("occupied_noforce/existing.txt", "data");

        auto res = run_cli({"new", occupied.string()});

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("already exists and is not empty") != std::string::npos);
        CHECK(res.err.find("--force") != std::string::npos);
    }

    SECTION("Negative - Case 4.8: Target Path Exists as a Regular File") {
        auto regular_file = sandbox.create_file("file_not_dir", "some content");

        auto res = run_cli({"new", regular_file.string()});

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("exists and is not a directory") != std::string::npos);
    }

    SECTION("Negative - Case 4.9: Custom Template Path Does Not Exist") {
        auto proj_dir = sandbox.path() / "will_fail_proj";
        auto missing_tmpl = sandbox.path() / "non_existent_template_folder";

        auto res = run_cli({"new", proj_dir.string(), "--template", missing_tmpl.string()});

        CHECK(res.exit_code != 0);
        CHECK(res.err.find("Template directory does not exist") != std::string::npos);
    }
}
