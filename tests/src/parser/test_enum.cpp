#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/lexer.hpp"

using namespace solix;

TEST_CASE("Parser: EnumDeclaration", "[parser][enum]") {
    SECTION("Valid Enum") {
        std::string source = "public enum State { START, STOP }";
        parser::AstTree tree;
        
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        REQUIRE(tree.nodes.size() == 1);
        
        auto* enum_node = dynamic_cast<parser::EnumDeclaration*>(tree.nodes[0].get());
        REQUIRE(enum_node != nullptr);
        REQUIRE(enum_node->enum_name == "State");
        REQUIRE(enum_node->access_modifier == lexer::TokenType::KEYWORD_PUBLIC);
        REQUIRE(enum_node->members.size() == 2);
        REQUIRE(enum_node->members[0] == "START");
        REQUIRE(enum_node->members[1] == "STOP");
    }

    SECTION("Valid Enum with Trailing Comma") {
        std::string source = "enum Colors { RED, GREEN, BLUE, }";
        parser::AstTree tree;
        
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        REQUIRE(tree.nodes.size() == 1);
        
        auto* enum_node = dynamic_cast<parser::EnumDeclaration*>(tree.nodes[0].get());
        REQUIRE(enum_node != nullptr);
        REQUIRE(enum_node->enum_name == "Colors");
        REQUIRE(enum_node->members.size() == 3);
        REQUIRE(enum_node->members[2] == "BLUE");
    }

    SECTION("Error: Missing Enum Body") {
        std::string source = "enum State;";
        parser::AstTree tree;
        
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Expected '{' for enum body"));
    }

    SECTION("Error: Invalid Enum Member") {
        std::string source = "enum State { START, 123 }";
        parser::AstTree tree;
        
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Enum members must be valid identifiers"));
    }
}
