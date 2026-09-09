#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/lexer.hpp"

using namespace solix;

TEST_CASE("Parser: FieldDeclaration", "[parser][field]") {
    SECTION("Valid Basic Field") {
        std::string source = "class Test { private int32 count; }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        auto* cls = dynamic_cast<parser::ClassDeclaration*>(tree.nodes[0].get());
        REQUIRE(cls != nullptr);
        REQUIRE(cls->children.size() == 1);
        
        auto* field = dynamic_cast<parser::FieldDeclaration*>(cls->children[0].get());
        REQUIRE(field != nullptr);
        REQUIRE(field->access_modifier == lexer::TokenType::KEYWORD_PRIVATE);
        REQUIRE(field->type_name == "int32 ");
        REQUIRE(field->field_name == "count");
        REQUIRE(field->initializer == nullptr);
    }

    SECTION("Valid Field with Initializer") {
        std::string source = "class Test { public static const float32 PI = 3.14159; }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        auto* cls = dynamic_cast<parser::ClassDeclaration*>(tree.nodes[0].get());
        REQUIRE(cls != nullptr);
        
        auto* field = dynamic_cast<parser::FieldDeclaration*>(cls->children[0].get());
        REQUIRE(field != nullptr);
        REQUIRE(field->access_modifier == lexer::TokenType::KEYWORD_PUBLIC);
        REQUIRE(field->is_static == true);
        REQUIRE(field->is_const == true);
        REQUIRE(field->type_name == "float32 ");
        REQUIRE(field->field_name == "PI");
        REQUIRE(field->initializer != nullptr);
    }

    SECTION("Error: Missing Expression after Equals") {
        std::string source = "class Test { int32 count = ; }";
        parser::AstTree tree;
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Expected expression after '='"));
    }
}
