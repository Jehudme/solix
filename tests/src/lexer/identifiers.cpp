#include <catch2/catch_test_macros.hpp>
#include "solix/lexer.hpp"
#include <string_view>

using namespace solix::lexer;

TEST_CASE("Lexer - Identifiers Exhaustive", "[lexer][identifiers]") {
    SECTION("Valid Identifiers") {
        auto tokens = tokenize("myVar _myVar camelCase PascalCase snake_case var123 _123");
        REQUIRE(tokens.size() == 8);
        for(int i = 0; i < 7; i++) {
            REQUIRE(tokens[i].type == TokenType::IDENTIFIER);
        }
        REQUIRE(tokens[0].value.value() == "myVar");
        REQUIRE(tokens[1].value.value() == "_myVar");
        REQUIRE(tokens[6].value.value() == "_123");
    }

    SECTION("Invalid Identifiers (Starting with number)") {
        // "123var" should lex as NUMBER("123") followed by IDENTIFIER("var")
        auto tokens = tokenize("123var");
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[0].type == TokenType::NUMBER);
        REQUIRE(tokens[0].value.value() == "123");
        REQUIRE(tokens[1].type == TokenType::IDENTIFIER);
        REQUIRE(tokens[1].value.value() == "var");
    }
}
