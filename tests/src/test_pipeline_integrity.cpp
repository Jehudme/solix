#include "solix/compilation.hpp"
#include "utilities/diagnostic.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace solix;

TEST_CASE("Pipeline Integrity - Halt Compilation on Errors", "[pipeline_integrity]") {
    SECTION("Syntax error halts compilation before binder and throws CompilationFailedException") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source key = std::string("syntax_err.slx");
        // Malformed syntax: missing expression in assignment
        options.sources[key] = R"(
            class BrokenClass {
                public void test() {
                    int32 a = ;
                }
            }
        )";

        REQUIRE_THROWS_AS(solix::run(options), CompilationFailedException);
    }

    SECTION("Lexical error halts compilation and throws CompilationFailedException") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source key = std::string("lex_err.slx");
        // Unknown token: backtick ` is not a valid token in Solix
        options.sources[key] = R"(
            class LexErrClass {
                public void test() {
                    int32 ` = 5;
                }
            }
        )";

        REQUIRE_THROWS_AS(solix::run(options), CompilationFailedException);
    }

    SECTION("Semantic/Type error halts compilation before assembler and throws CompilationFailedException") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source key = std::string("type_err.slx");
        // Semantic error: undeclared identifier and type mismatch
        options.sources[key] = R"(
            class TypeErrClass {
                public void test() {
                    int32 a = undeclared_identifier_xyz;
                }
            }
        )";

        REQUIRE_THROWS_AS(solix::run(options), CompilationFailedException);
    }

    SECTION("Valid source compiles successfully without throwing") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source key = std::string("valid.slx");
        options.sources[key] = R"(
            class ValidClass {
                public static int32 main() {
                    int32 a = 42;
                    return a;
                }
            }
        )";

        std::vector<uint8_t> bytecode;
        REQUIRE_NOTHROW(bytecode = solix::run(options));
        REQUIRE_FALSE(bytecode.empty());
    }
}
