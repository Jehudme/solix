#include <catch2/catch_all.hpp>
#include "cli_test_helper.hpp"

#ifndef SOLIX_VERSION
#define SOLIX_VERSION "0.1.0"
#endif

using namespace solix::test;

TEST_CASE("CLI Command - version", "[command][version]") {
    SECTION("Case 9.1: Version Subcommand (solix version)") {
        auto res = run_cli({"version"});
        CHECK(res.exit_code == 0);
        CHECK(res.out.find("Solix version") != std::string::npos);
        CHECK(res.out.find(SOLIX_VERSION) != std::string::npos);
    }

    SECTION("Case 9.2: Version Flag (solix --version)") {
        auto res = run_cli({"--version"});
        CHECK(res.exit_code == 0);
        CHECK(res.out.find("Solix version") != std::string::npos);
        CHECK(res.out.find(SOLIX_VERSION) != std::string::npos);
    }

    SECTION("Case 9.3: Version Short Flag (solix -v)") {
        auto res = run_cli({"-v"});
        CHECK(res.exit_code == 0);
        CHECK(res.out.find("Solix version") != std::string::npos);
        CHECK(res.out.find(SOLIX_VERSION) != std::string::npos);
    }
}
