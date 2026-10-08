#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <string>

TEST_CASE("Suite 17: CPack Multi-OS Packaging & Installer Staging", "[packaging][cpack]") {
    std::filesystem::path project_root = SOLIX_PROJECT_ROOT;
    std::filesystem::path build_dir = project_root / "build";

    SECTION("Case 17.1: CPack Staging Manifest and Component Verification") {
        // Verify CPack configuration presence in build directory
        std::filesystem::path cpack_cfg = build_dir / "CPackConfig.cmake";
        REQUIRE(std::filesystem::exists(cpack_cfg));

        std::ifstream f(cpack_cfg);
        REQUIRE(f.is_open());
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

        // Verify package metadata
        REQUIRE(content.find("CPACK_PACKAGE_NAME \"solix\"") != std::string::npos);
        REQUIRE(content.find("CPACK_PACKAGE_VERSION \"0.1.0\"") != std::string::npos);
        REQUIRE(content.find("CPACK_PACKAGE_VENDOR \"Solix Team\"") != std::string::npos);
    }

    SECTION("Case 17.2: Standard Library Bundling Integrity in Package Tree") {
        std::filesystem::path solixlib_src = project_root / "solixlib" / "project";
        REQUIRE(std::filesystem::exists(solixlib_src / "solix.json"));
        REQUIRE(std::filesystem::exists(solixlib_src / "src"));
        REQUIRE(std::filesystem::exists(solixlib_src / "src" / "solix" / "system" / "Console.slx"));
        REQUIRE(std::filesystem::exists(solixlib_src / "src" / "solix" / "collections" / "List.slx"));

        // Verify CMake install manifest specifies solixlib destination
        std::filesystem::path cmake_install = build_dir / "cmake_install.cmake";
        REQUIRE(std::filesystem::exists(cmake_install));

        std::ifstream f(cmake_install);
        REQUIRE(f.is_open());
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

        REQUIRE(content.find("share/solix/solixlib") != std::string::npos);
    }

    SECTION("Case 17.3: Portable Archive File Structure and Permissions") {
        // Verify VS Code extension and documentation staging in install script
        std::filesystem::path cmake_install = build_dir / "cmake_install.cmake";
        REQUIRE(std::filesystem::exists(cmake_install));

        std::ifstream f(cmake_install);
        REQUIRE(f.is_open());
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

        REQUIRE(content.find("share/doc/solix") != std::string::npos);
        if (std::filesystem::exists(project_root / "editors" / "vscode" / "solix-0.1.0.vsix")) {
            REQUIRE(content.find("share/solix/vscode") != std::string::npos);
        }

        // Verify top-level LICENSE exists and is readable
        std::filesystem::path lic = project_root / "LICENSE";
        REQUIRE(std::filesystem::exists(lic));
        REQUIRE(std::filesystem::file_size(lic) > 0);
    }

    SECTION("Case 17.4: Packaging Missing Critical Files Fails Gracefully") {
        // Verify CPack check: non-existent license in a dummy config throws error
        std::filesystem::path temp_test_dir = std::filesystem::temp_directory_path() / "solix_cpack_negative_test";
        std::error_code ec;
        std::filesystem::create_directories(temp_test_dir, ec);

        std::filesystem::path dummy_cmake = temp_test_dir / "CMakeLists.txt";
        {
            std::ofstream df(dummy_cmake);
            df << "cmake_minimum_required(VERSION 3.20)\n"
               << "project(dummy VERSION 1.0.0)\n"
               << "set(CPACK_RESOURCE_FILE_LICENSE \"/non_existent_file_license_12345.txt\")\n"
               << "include(CPack)\n";
        }

        // Running cmake on dummy dir should fail because license file does not exist
        std::string cmd = "cmake -S " + temp_test_dir.string() + " -B " + (temp_test_dir / "build").string() + " > /dev/null 2>&1";
        int res = std::system(cmd.c_str());
        REQUIRE(res != 0);

        std::filesystem::remove_all(temp_test_dir, ec);
    }
}
