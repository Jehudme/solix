#include <catch2/catch_test_macros.hpp>
#include "solix/lexer.hpp"
#include <string_view>

using namespace solix::lexer;

TEST_CASE("Lexer - Operators Exhaustive", "[lexer][operators]") {
    SECTION("Single Character Operators") {
        auto tokens = tokenize("= + - * / % ! < >");
        REQUIRE(tokens.size() == 10);
        REQUIRE(tokens[0].type == TokenType::OPERATOR_ASSIGN);
        REQUIRE(tokens[1].type == TokenType::OPERATOR_PLUS);
        REQUIRE(tokens[2].type == TokenType::OPERATOR_MINUS);
        REQUIRE(tokens[3].type == TokenType::OPERATOR_MULTIPLY);
        REQUIRE(tokens[4].type == TokenType::OPERATOR_DIVIDE);
        REQUIRE(tokens[5].type == TokenType::OPERATOR_MODULO);
        REQUIRE(tokens[6].type == TokenType::OPERATOR_LOGICAL_NOT);
        REQUIRE(tokens[7].type == TokenType::OPERATOR_LESS_THAN);
        REQUIRE(tokens[8].type == TokenType::OPERATOR_GREATER_THAN);
    }

    SECTION("Double Character Operators") {
        auto tokens = tokenize("++ -- == != <= >= && ||");
        REQUIRE(tokens.size() == 9);
        REQUIRE(tokens[0].type == TokenType::OPERATOR_INCREMENT);
        REQUIRE(tokens[1].type == TokenType::OPERATOR_DECREMENT);
        REQUIRE(tokens[2].type == TokenType::OPERATOR_EQUAL);
        REQUIRE(tokens[3].type == TokenType::OPERATOR_NOT_EQUAL);
        REQUIRE(tokens[4].type == TokenType::OPERATOR_LESS_EQUAL);
        REQUIRE(tokens[5].type == TokenType::OPERATOR_GREATER_EQUAL);
        REQUIRE(tokens[6].type == TokenType::OPERATOR_LOGICAL_AND);
        REQUIRE(tokens[7].type == TokenType::OPERATOR_LOGICAL_OR);
    }

    SECTION("Ambiguous/Touching Operators") {
        // "+++" should be parsed as "++" then "+"
        auto t1 = tokenize("+++");
        REQUIRE(t1.size() == 3);
        REQUIRE(t1[0].type == TokenType::OPERATOR_INCREMENT);
        REQUIRE(t1[1].type == TokenType::OPERATOR_PLUS);

        // "---" should be "--" then "-"
        auto t2 = tokenize("---");
        REQUIRE(t2.size() == 3);
        REQUIRE(t2[0].type == TokenType::OPERATOR_DECREMENT);
        REQUIRE(t2[1].type == TokenType::OPERATOR_MINUS);

        // "<=" vs "< =" 
        auto t3 = tokenize("<= < =");
        REQUIRE(t3.size() == 4);
        REQUIRE(t3[0].type == TokenType::OPERATOR_LESS_EQUAL);
        REQUIRE(t3[1].type == TokenType::OPERATOR_LESS_THAN);
        REQUIRE(t3[2].type == TokenType::OPERATOR_ASSIGN);
    }
}
