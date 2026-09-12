#include <catch2/catch_test_macros.hpp>
#include "solix/lexer.hpp"
#include <string_view>

using namespace solix::lexer;

TEST_CASE("Lexer - Literals Exhaustive", "[lexer][literals]") {
    SECTION("Integer Numbers") {
        auto tokens = tokenize("0 123456789");
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[0].type == TokenType::NUMBER);
        REQUIRE(tokens[0].value.value() == "0");
        REQUIRE(tokens[1].type == TokenType::NUMBER);
        REQUIRE(tokens[1].value.value() == "123456789");
    }

    SECTION("Floating Point Numbers") {
        auto tokens = tokenize("0.0 3.14159 10.");
        // Note: 10. usually lexes as NUMBER(10) then PUNCTUATION_DOT unless handled. 
        // In our lexer, we require a digit after the dot for it to be a float.
        REQUIRE(tokens.size() == 5);
        REQUIRE(tokens[0].type == TokenType::NUMBER);
        REQUIRE(tokens[0].value.value() == "0.0");
        REQUIRE(tokens[1].type == TokenType::NUMBER);
        REQUIRE(tokens[1].value.value() == "3.14159");
        REQUIRE(tokens[2].type == TokenType::NUMBER);
        REQUIRE(tokens[2].value.value() == "10");
        REQUIRE(tokens[3].type == TokenType::PUNCTUATION_DOT); // Dot follows
    }

    SECTION("Strings") {
        auto tokens = tokenize("\"hello world\" 'A' \"\" \"esc\\\"aped\"");
        REQUIRE(tokens.size() == 5);
        REQUIRE(tokens[0].type == TokenType::STRING);
        REQUIRE(tokens[0].value.value() == "hello world");
        REQUIRE(tokens[1].type == TokenType::STRING); // 'A' acts as string token in our lexer
        REQUIRE(tokens[1].value.value() == "A");
        REQUIRE(tokens[2].type == TokenType::STRING);
        REQUIRE(tokens[2].value.value() == "");
        REQUIRE(tokens[3].type == TokenType::STRING);
        REQUIRE(tokens[3].value.value() == "esc\"aped");
    }
}
