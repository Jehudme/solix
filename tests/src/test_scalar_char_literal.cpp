#include "solix/compilation.hpp"
#include "solix/runtime.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace solix;

static int32_t compile_and_run_char(const std::string &source) {
    CompilationOptions comp_opts;
    comp_opts.log_level = CompilationOptions::LogLevel::OFF;
    Source src_key = std::string("test_char.slx");
    comp_opts.sources[src_key] = source;

    Bytecode bytecode = solix::run(comp_opts);

    RuntimeOptions run_opts;
    run_opts.bytecode_source = bytecode;
    return solix::run(run_opts);
}

TEST_CASE("Phase 4 - Scalar Character Literals and Escapes", "[phase4][char_literal]") {
    SECTION("Basic scalar char assignment and return") {
        std::string source = R"(
            public class CharBasicTest {
                public static int32 main(char[][] args) {
                    char c = 'A';
                    if (c == 'A') return 0;
                    return 1;
                }
            }
        )";
        REQUIRE(compile_and_run_char(source) == 0);
    }

    SECTION("Character escape sequences: newline, tab, zero, and hex") {
        std::string source = R"(
            public class CharEscapeTest {
                public static int32 main(char[][] args) {
                    char nl = '\n';
                    char tab = '\t';
                    char nul = '\0';
                    char slash = '\\';
                    char quote = '\'';
                    char hex_a = '\x41'; // ASCII 65 = 'A'

                    if (nl != 10) return 1;
                    if (tab != 9) return 2;
                    if (nul != 0) return 3;
                    if (slash != 92) return 4;
                    if (quote != 39) return 5;
                    if (hex_a != 'A') return 6;

                    return 0;
                }
            }
        )";
        REQUIRE(compile_and_run_char(source) == 0);
    }

    SECTION("Overloaded method dispatch distinguishing char from char[]") {
        std::string source = R"(
            public class OverloadCharTest {
                public static int32 test_overload(char c) {
                    return 100;
                }

                public static int32 test_overload(char[] s) {
                    return 200;
                }

                public static int32 main(char[][] args) {
                    int32 res_char = OverloadCharTest.test_overload('z');
                    int32 res_str = OverloadCharTest.test_overload("hello");
                    if (res_char == 100 && res_str == 200) {
                        return 0;
                    }
                    return 1;
                }
            }
        )";
        REQUIRE(compile_and_run_char(source) == 0);
    }
}
