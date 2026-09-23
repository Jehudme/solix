#include "solix/compilation.hpp"
#include "solix/processes/lexer.hpp"
#include "solix/processes/parser.hpp"
#include "solix/utilities/diagnostic.hpp"
#include "solix/utilities/token.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace solix;

TEST_CASE("Phase 23 - Ampersand Lexing and Parsing", "[phase23]") {
    SECTION("Single ampersand lexed as PUNCTUATION_AMPERSAND") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source src_key = std::string("test");
        options.sources[src_key] = "int32& x = a && b;";

        CompilationContext context(options);
        context.diagnostic = std::make_unique<Diagnostic>(context);

        Lexer lexer(context, "Lexer");
        lexer.execute();

        const auto& tokens = context.tokens[src_key].front();
        
        // int32, &, x, =, a, &&, b, ;
        REQUIRE(tokens[0].type == TokenType::PRIMITIVE_INT32);
        REQUIRE(tokens[1].type == TokenType::PUNCTUATION_AMPERSAND);
        REQUIRE(tokens[2].type == TokenType::IDENTIFIER);
        REQUIRE(tokens[3].type == TokenType::OPERATOR_ASSIGN);
        REQUIRE(tokens[4].type == TokenType::IDENTIFIER);
        REQUIRE(tokens[5].type == TokenType::OPERATOR_LOGICAL_AND);
        REQUIRE(tokens[6].type == TokenType::IDENTIFIER);
        REQUIRE(tokens[7].type == TokenType::PUNCTUATION_SEMICOLON);
    }

    SECTION("Reference variable and function parameter parsing") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source src_key = std::string("test");
        options.sources[src_key] = R"(
            class RefTest {
                public void process(int32& ref_param) {
                    int32& local_ref = ref_param;
                }
            }
        )";

        CompilationContext context(options);
        context.diagnostic = std::make_unique<Diagnostic>(context);

        Lexer lexer(context, "Lexer");
        lexer.execute();

        Parser parser(context, "Parser");
        REQUIRE_NOTHROW(parser.execute());
    }
}
