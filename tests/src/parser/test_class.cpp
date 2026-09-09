#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/lexer.hpp"

using namespace solix;

TEST_CASE("Parser: ClassDeclaration", "[parser][class]") {
    SECTION("Valid Empty Class") {
        std::string source = "public class Engine { }";
        parser::AstTree tree;
        
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        REQUIRE(tree.nodes.size() == 1);
        
        auto* cls = dynamic_cast<parser::ClassDeclaration*>(tree.nodes[0].get());
        REQUIRE(cls != nullptr);
        REQUIRE(cls->class_name == "Engine");
        REQUIRE(cls->access_modifier == lexer::TokenType::KEYWORD_PUBLIC);
        REQUIRE(cls->children.empty() == true);
    }

    SECTION("Error: Missing Class Name") {
        std::string source = "class { }";
        parser::AstTree tree;
        
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Expected identifier for class name"));
    }

    SECTION("Error: Missing Braces") {
        std::string source = "class Engine;";
        parser::AstTree tree;
        
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Expected '{' for class body"));
    }
}
