#include <catch2/catch_all.hpp>
#include "cli_test_helper.hpp"

using namespace solix::test;

TEST_CASE("CLI Command - compile", "[command][compile]") {
    TempDir sandbox;

    SECTION("Positive - Case 1.1: Single Source File Compilation to Default Output") {
        auto src = sandbox.create_file("main.slx", "int32 main() { return 0; }\n");
        // Run from sandbox directory
        auto orig_cwd = std::filesystem::current_path();
        std::filesystem::current_path(sandbox.path());

        auto res = run_cli({"compile", "main.slx"});
        std::filesystem::current_path(orig_cwd);

        CHECK(res.exit_code == 0);
        CHECK(sandbox.exists("out.slxb"));
        CHECK(std::filesystem::file_size(sandbox.path() / "out.slxb") > 0);
        CHECK(res.out.find("Successfully compiled to out.slxb") != std::string::npos);
    }

    SECTION("Positive - Case 1.2: Custom Output Bytecode Path") {
        auto src = sandbox.create_file("src/app.slx", "int32 main() { return 0; }\n");
        auto out = sandbox.path() / "bin/nested/custom.slxb";

        auto res = run_cli({"compile", src.string(), "-o", out.string()});

        CHECK(res.exit_code == 0);
        CHECK(std::filesystem::exists(out));
        CHECK(std::filesystem::file_size(out) > 0);
    }

    SECTION("Positive - Case 1.3: Multiple Source Files Compilation") {
        auto f1 = sandbox.create_file("helper.slx", "class Helper { public static int32 val() { return 42; } }\n");
        auto f2 = sandbox.create_file("entry.slx", "int32 main() { return Helper.val() == 42 ? 0 : 1; }\n");
        auto out = sandbox.path() / "multi.slxb";

        auto res = run_cli({"compile", f1.string(), f2.string(), "-o", out.string()});

        CHECK(res.exit_code == 0);
        CHECK(std::filesystem::exists(out));
    }

    SECTION("Positive - Case 1.4: Disassembly / Assembly Emission") {
        auto src = sandbox.create_file("asm_test.slx", "int32 main() { return 0; }\n");
        auto out = sandbox.path() / "asm_test.slxb";
        auto asm_out = sandbox.path() / "asm_test.s";

        auto res = run_cli({"compile", src.string(), "-o", out.string(), "-a", asm_out.string()});

        CHECK(res.exit_code == 0);
        CHECK(std::filesystem::exists(out));
        CHECK(std::filesystem::exists(asm_out));
        CHECK(std::filesystem::file_size(asm_out) > 0);
    }

    SECTION("Positive - Case 1.5: Custom Entry Point Specification") {
        auto src = sandbox.create_file("entry_test.slx", "int32 custom_start() { return 0; }\n");
        auto out = sandbox.path() / "entry_test.slxb";

        auto res = run_cli({"compile", src.string(), "-e", "custom_start", "-o", out.string()});

        CHECK(res.exit_code == 0);
        CHECK(std::filesystem::exists(out));
    }

    SECTION("Positive - Case 1.6: Advanced Logging and Sink Flags") {
        auto src = sandbox.create_file("log_test.slx", "int32 main() { return 0; }\n");
        auto out = sandbox.path() / "log_test.slxb";

        auto res = run_cli({"compile", src.string(), "-o", out.string(), "--log-level", "DEBUG", "--sink-type", "STDOUT"});

        CHECK(res.exit_code == 0);
        CHECK(std::filesystem::exists(out));
    }

    SECTION("Negative - Case 1.7: Missing Source File Argument") {
        auto res = run_cli({"compile"});
        CHECK(res.exit_code != 0);
        CHECK(res.err.find("files is required") != std::string::npos);
    }

    SECTION("Negative - Case 1.8: Non-Existent Source File") {
        auto missing = sandbox.path() / "does_not_exist.slx";
        auto res = run_cli({"compile", missing.string()});
        CHECK(res.exit_code != 0);
        CHECK((res.err.find("does_not_exist.slx") != std::string::npos || res.err.find("does not exist") != std::string::npos));
    }

    SECTION("Negative - Case 1.9: Source Code Syntax Error") {
        auto src = sandbox.create_file("syntax_err.slx", "class { missing_ident\n");
        auto out = sandbox.path() / "syntax_err.slxb";

        auto res = run_cli({"compile", src.string(), "-o", out.string()});

        CHECK(res.exit_code != 0);
        CHECK(!std::filesystem::exists(out));
    }

    SECTION("Negative - Case 1.10: Source Code Semantic Error") {
        auto src = sandbox.create_file("sem_err.slx", "int32 main() { return UndeclaredClass.foo(); }\n");
        auto out = sandbox.path() / "sem_err.slxb";

        auto res = run_cli({"compile", src.string(), "-o", out.string()});

        CHECK(res.exit_code != 0);
        CHECK(!std::filesystem::exists(out));
    }

    SECTION("Negative - Case 1.11: Invalid Log Level Option") {
        auto src = sandbox.create_file("valid.slx", "int32 main() { return 0; }\n");
        auto res = run_cli({"compile", src.string(), "--log-level", "NON_EXISTENT_LOG_LEVEL"});

        CHECK(res.exit_code != 0);
    }
}
