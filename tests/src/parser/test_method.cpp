#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/lexer.hpp"

using namespace solix;

TEST_CASE("Parser: MethodDeclaration", "[parser][method]") {
    SECTION("Valid Method with Body") {
        std::string source = "class Test { public static inline int32 add(int32 a, int32 b) { return a + b; } }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        auto* cls = dynamic_cast<parser::ClassDeclaration*>(tree.nodes[0].get());
        REQUIRE(cls != nullptr);
        REQUIRE(cls->children.size() == 1);
        
        auto* method = dynamic_cast<parser::MethodDeclaration*>(cls->children[0].get());
        REQUIRE(method != nullptr);
        REQUIRE(method->access_modifier == lexer::TokenType::KEYWORD_PUBLIC);
        REQUIRE(method->is_static == true);
        REQUIRE(method->is_inline == true);
        REQUIRE(method->return_type == "int32 ");
        REQUIRE(method->method_name == "add");
        REQUIRE(method->children.size() == 1); // The BlockStatement
    }

    SECTION("Valid Constructor with Body") {
        std::string source = "class Test { public Test() { } }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        auto* cls = dynamic_cast<parser::ClassDeclaration*>(tree.nodes[0].get());
        REQUIRE(cls != nullptr);
        REQUIRE(cls->children.size() == 1);
        
        auto* ctor = dynamic_cast<parser::ConstructorDeclaration*>(cls->children[0].get());
        REQUIRE(ctor != nullptr);
        REQUIRE(ctor->access_modifier == lexer::TokenType::KEYWORD_PUBLIC);
        REQUIRE(ctor->children.size() == 1); // The BlockStatement
    }

    SECTION("Error: Missing return type") {
        std::string source = "class Test { public myFunc() { } }";
        parser::AstTree tree;
        
        // This will be parsed as a constructor, which is syntactically valid in the parser stage
        // as the parser just looks for `name()` and considers it a constructor if there's no return type.
        // It will fail Semantic Analysis if `myFunc` is not the class name `Test`.
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
    }
}
