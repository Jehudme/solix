#include <catch2/catch_test_macros.hpp>
#include "solix/lexer.hpp"
#include <string_view>

using namespace solix::lexer;

TEST_CASE("Lexer - Unknowns and Errors Exhaustive", "[lexer][unknowns]") {
    SECTION("Invalid characters") {
        auto tokens = tokenize("@ # $ ^ ~");
        REQUIRE(tokens.size() == 6);
        for (int i = 0; i < 5; i++) {
            REQUIRE(tokens[i].type == TokenType::UNKNOWN_TOKEN);
        }
    }

    SECTION("Unterminated String") {
        auto tokens = tokenize("\"this string has no end");
        REQUIRE(tokens.size() == 2);
        REQUIRE(tokens[0].type == TokenType::UNKNOWN_TOKEN); // Emitted because of EOF
        REQUIRE(tokens[0].value.value() == "\"this string has no end");
    }

    SECTION("Lone & or |") {
        // Our language uses && and ||. A lone & or | is an error.
        auto tokens = tokenize("& |");
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[0].type == TokenType::UNKNOWN_TOKEN);
        REQUIRE(tokens[0].value.value() == "&");
        REQUIRE(tokens[1].type == TokenType::UNKNOWN_TOKEN);
        REQUIRE(tokens[1].value.value() == "|");
    }
    
    SECTION("Error Recovery") {
        // Ensure lexer continues after an error
        auto tokens = tokenize("int32 @ var = 10;");
        REQUIRE(tokens.size() == 7);
        REQUIRE(tokens[0].type == TokenType::PRIMITIVE_INT32);
        REQUIRE(tokens[1].type == TokenType::UNKNOWN_TOKEN);
        REQUIRE(tokens[2].type == TokenType::IDENTIFIER);
        REQUIRE(tokens[3].type == TokenType::OPERATOR_ASSIGN);
        REQUIRE(tokens[4].type == TokenType::NUMBER);
        REQUIRE(tokens[5].type == TokenType::PUNCTUATION_SEMICOLON);
        REQUIRE(tokens[6].type == TokenType::EOF_TOKEN);
    }
}
