#include <catch2/catch_test_macros.hpp>
#include "solix/lexer.hpp"
#include <string_view>

using namespace solix::lexer;

TEST_CASE("Lexer - Punctuations Exhaustive", "[lexer][punctuations]") {
    SECTION("All Punctuation") {
        auto tokens = tokenize("; , . : ( ) { } [ ]");
        REQUIRE(tokens.size() == 11);
        REQUIRE(tokens[0].type == TokenType::PUNCTUATION_SEMICOLON);
        REQUIRE(tokens[1].type == TokenType::PUNCTUATION_COMMA);
        REQUIRE(tokens[2].type == TokenType::PUNCTUATION_DOT);
        REQUIRE(tokens[3].type == TokenType::PUNCTUATION_COLON);
        REQUIRE(tokens[4].type == TokenType::PUNCTUATION_OPEN_PAREN);
        REQUIRE(tokens[5].type == TokenType::PUNCTUATION_CLOSE_PAREN);
        REQUIRE(tokens[6].type == TokenType::PUNCTUATION_OPEN_BRACE);
        REQUIRE(tokens[7].type == TokenType::PUNCTUATION_CLOSE_BRACE);
        REQUIRE(tokens[8].type == TokenType::PUNCTUATION_OPEN_BRACKET);
        REQUIRE(tokens[9].type == TokenType::PUNCTUATION_CLOSE_BRACKET);
    }
}
