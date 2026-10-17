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
}
