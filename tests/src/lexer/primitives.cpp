#include <catch2/catch_test_macros.hpp>
#include "solix/lexer.hpp"
#include <string_view>

using namespace solix::lexer;

void assert_primitive(std::string_view code, TokenType expected) {
    auto tokens = tokenize(code);
    REQUIRE(tokens.size() == 2);
    REQUIRE(tokens[0].type == expected);
    REQUIRE(tokens[1].type == TokenType::EOF_TOKEN);
}

TEST_CASE("Lexer - Primitives Exhaustive", "[lexer][primitives]") {
    SECTION("Base types") {
        assert_primitive("void", TokenType::PRIMITIVE_VOID);
        assert_primitive("bool", TokenType::PRIMITIVE_BOOL);
        assert_primitive("char", TokenType::PRIMITIVE_CHAR);
        assert_primitive("string", TokenType::PRIMITIVE_STRING);
    }
    
    SECTION("Signed Integers") {
        assert_primitive("int8", TokenType::PRIMITIVE_INT8);
        assert_primitive("int16", TokenType::PRIMITIVE_INT16);
        assert_primitive("int32", TokenType::PRIMITIVE_INT32);
        assert_primitive("int64", TokenType::PRIMITIVE_INT64);
    }

    SECTION("Unsigned Integers") {
        assert_primitive("uint8", TokenType::PRIMITIVE_UINT8);
        assert_primitive("uint16", TokenType::PRIMITIVE_UINT16);
        assert_primitive("uint32", TokenType::PRIMITIVE_UINT32);
        assert_primitive("uint64", TokenType::PRIMITIVE_UINT64);
    }

    SECTION("Floating point") {
        assert_primitive("float32", TokenType::PRIMITIVE_FLOAT32);
        assert_primitive("float64", TokenType::PRIMITIVE_FLOAT64);
    }

    SECTION("Boundary constraints") {
        // "int32_t" should NOT be lexed as PRIMITIVE_INT32, it should be an IDENTIFIER
        auto tokens = tokenize("int32_t int32var int32");
        REQUIRE(tokens.size() == 4);
        REQUIRE(tokens[0].type == TokenType::IDENTIFIER);
        REQUIRE(tokens[1].type == TokenType::IDENTIFIER);
        REQUIRE(tokens[2].type == TokenType::PRIMITIVE_INT32);
    }

    SECTION("Array Syntax (Lexer handles as 3 tokens)") {
        // In modern compilers, int8[] is parsed as [PRIMITIVE_INT8, OPEN_BRACKET, CLOSE_BRACKET]
        // The parser groups these later!
        auto tokens = tokenize("int8[] bool[]");
        
        REQUIRE(tokens.size() == 7); // int8, [, ], bool, [, ], EOF
        
        REQUIRE(tokens[0].type == TokenType::PRIMITIVE_INT8);
        REQUIRE(tokens[1].type == TokenType::PUNCTUATION_OPEN_BRACKET);
        REQUIRE(tokens[2].type == TokenType::PUNCTUATION_CLOSE_BRACKET);
        
        REQUIRE(tokens[3].type == TokenType::PRIMITIVE_BOOL);
        REQUIRE(tokens[4].type == TokenType::PUNCTUATION_OPEN_BRACKET);
        REQUIRE(tokens[5].type == TokenType::PUNCTUATION_CLOSE_BRACKET);
    }
}
