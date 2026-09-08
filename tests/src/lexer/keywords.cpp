#include <catch2/catch_test_macros.hpp>
#include "solix/lexer.hpp"
#include <string_view>

using namespace solix::lexer;

void assert_keyword(std::string_view code, TokenType expected) {
    auto tokens = tokenize(code);
    REQUIRE(tokens.size() == 2);
    REQUIRE(tokens[0].type == expected);
}

TEST_CASE("Lexer - Keywords Exhaustive", "[lexer][keywords]") {
    SECTION("Control Flow") {
        assert_keyword("if", TokenType::KEYWORD_IF);
        assert_keyword("else", TokenType::KEYWORD_ELSE);
        assert_keyword("for", TokenType::KEYWORD_FOR);
        assert_keyword("while", TokenType::KEYWORD_WHILE);
        assert_keyword("return", TokenType::KEYWORD_RETURN);
        assert_keyword("break", TokenType::KEYWORD_BREAK);
        assert_keyword("continue", TokenType::KEYWORD_CONTINUE);
        assert_keyword("switch", TokenType::KEYWORD_SWITCH);
        assert_keyword("case", TokenType::KEYWORD_CASE);
        assert_keyword("default", TokenType::KEYWORD_DEFAULT);
    }

    SECTION("Object Orientation") {
        assert_keyword("class", TokenType::KEYWORD_CLASS);
        assert_keyword("enum", TokenType::KEYWORD_ENUM);
        assert_keyword("new", TokenType::KEYWORD_NEW);
    }

    SECTION("Modifiers") {
        assert_keyword("const", TokenType::KEYWORD_CONST);
        assert_keyword("static", TokenType::KEYWORD_STATIC);
        assert_keyword("inline", TokenType::KEYWORD_INLINE);
        assert_keyword("public", TokenType::KEYWORD_PUBLIC);
        assert_keyword("protected", TokenType::KEYWORD_PROTECTED);
        assert_keyword("private", TokenType::KEYWORD_PRIVATE);
        assert_keyword("internal", TokenType::KEYWORD_INTERNAL);
    }

    SECTION("Module & Types") {
        assert_keyword("alias", TokenType::KEYWORD_ALIAS);
        assert_keyword("package", TokenType::KEYWORD_PACKAGE);
    }

    SECTION("Edge Cases - Boundaries") {
        // "if" is a keyword, but "iffy" is an identifier.
        auto tokens = tokenize("if iffy _if if_");
        REQUIRE(tokens.size() == 5);
        REQUIRE(tokens[0].type == TokenType::KEYWORD_IF);
        REQUIRE(tokens[1].type == TokenType::IDENTIFIER);
        REQUIRE(tokens[2].type == TokenType::IDENTIFIER);
        REQUIRE(tokens[3].type == TokenType::IDENTIFIER);
    }
}
