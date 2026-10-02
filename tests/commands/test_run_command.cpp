#include <catch2/catch_all.hpp>
#include "cli_test_helper.hpp"

using namespace solix::test;

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
        auto bc = generate_bytecode("app_zero", "static int32 main() { return 0; }\n");
        auto res = run_cli({"run", bc.string()});

        CHECK(res.exit_code == 0);
    }

    SECTION("Positive - Case 2.2: Pass Program Arguments to Executing Bytecode") {
        auto bc = generate_bytecode("app_args", "static int32 main() { return 0; }\n");
        auto res = run_cli({"run", bc.string(), "arg1", "arg2", "foo"});

        CHECK(res.exit_code == 0);
    }

    SECTION("Positive - Case 2.3: Custom Stack and Heap Capacities") {
        auto bc = generate_bytecode("app_limits", "static int32 main() { return 0; }\n");
        auto res = run_cli({"run", bc.string(), "-s", "2048", "-p", "4096"});

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
static int32 main() {
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
        auto bc = generate_bytecode("app_exit7", "static int32 main() { return 7; }\n");
        auto res = run_cli({"run", bc.string()});

        CHECK(res.exit_code == 7);
    }
}
