#include "solix/compilation.hpp"
#include "solix/runtime.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace solix;

static int32_t compile_and_run(const std::string &source) {
    CompilationOptions comp_opts;
    comp_opts.log_level = CompilationOptions::LogLevel::OFF;
    Source src_key = std::string("test_exit_code.slx");
    comp_opts.sources[src_key] = source;

    Bytecode bytecode = solix::run(comp_opts);

    RuntimeOptions run_opts;
    run_opts.bytecode_source = bytecode;
    return solix::run(run_opts);
}

TEST_CASE("Phase 3 - VM Exit Code Propagation", "[phase3][cli_ergonomics]") {
    SECTION("main() returning 0 propagates exit code 0") {
        std::string source = R"(
            public class ExitZeroTest {
                public static int32 main(char[][] args) {
                    return 0;
                }
            }
        )";
        REQUIRE(compile_and_run(source) == 0);
    }

    SECTION("main() returning 1 propagates exit code 1") {
        std::string source = R"(
            public class ExitOneTest {
                public static int32 main(char[][] args) {
                    return 1;
                }
            }
        )";
        REQUIRE(compile_and_run(source) == 1);
    }

    SECTION("main() returning 42 propagates exit code 42") {
        std::string source = R"(
            public class Exit42Test {
                public static int32 main(char[][] args) {
                    return 42;
                }
            }
        )";
        REQUIRE(compile_and_run(source) == 42);
    }

    SECTION("main() returning -1 propagates exit code -1") {
        std::string source = R"(
            public class ExitMinusOneTest {
                public static int32 main(char[][] args) {
                    return -1;
                }
            }
        )";
        REQUIRE(compile_and_run(source) == -1);
    }

    SECTION("void main() defaults to exit code 0") {
        std::string source = R"(
            public class ExitVoidTest {
                public static void main(char[][] args) {
                    int32 x = 10 + 20;
                }
            }
        )";
        REQUIRE(compile_and_run(source) == 0);
    }
}
