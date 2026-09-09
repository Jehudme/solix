#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/lexer.hpp"

using namespace solix;

TEST_CASE("Parser: VariableDeclaration", "[parser][variable]") {
    SECTION("Valid Variable Without Initializer") {
        std::string source = "class Test { public void method() { int32 count; } }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        auto* cls = dynamic_cast<parser::ClassDeclaration*>(tree.nodes[0].get());
        REQUIRE(cls != nullptr);
        auto* method = dynamic_cast<parser::MethodDeclaration*>(cls->children[0].get());
        REQUIRE(method != nullptr);
        auto* block = dynamic_cast<parser::BlockStatement*>(method->children[0].get());
        REQUIRE(block != nullptr);
        
        auto* var = dynamic_cast<parser::VariableDeclaration*>(block->children[0].get());
        REQUIRE(var != nullptr);
        REQUIRE(var->type_name == "int32 ");
        REQUIRE(var->var_name == "count");
        REQUIRE(var->initializer == nullptr);
    }

    SECTION("Valid Variable With Initializer") {
        std::string source = "class Test { public void method() { float64 speed = 1.5; } }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        auto* cls = dynamic_cast<parser::ClassDeclaration*>(tree.nodes[0].get());
        auto* method = dynamic_cast<parser::MethodDeclaration*>(cls->children[0].get());
        auto* block = dynamic_cast<parser::BlockStatement*>(method->children[0].get());
        
        auto* var = dynamic_cast<parser::VariableDeclaration*>(block->children[0].get());
        REQUIRE(var != nullptr);
        REQUIRE(var->type_name == "float64 ");
        REQUIRE(var->var_name == "speed");
        REQUIRE(var->initializer != nullptr);
    }

    SECTION("Valid Variable Const Modifiers") {
        std::string source = "class Test { public void method() { const int32 id = 100; } }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        auto* cls = dynamic_cast<parser::ClassDeclaration*>(tree.nodes[0].get());
        auto* method = dynamic_cast<parser::MethodDeclaration*>(cls->children[0].get());
        auto* block = dynamic_cast<parser::BlockStatement*>(method->children[0].get());
        
        auto* var = dynamic_cast<parser::VariableDeclaration*>(block->children[0].get());
        REQUIRE(var != nullptr);
        REQUIRE(var->is_const == true);
        REQUIRE(var->type_name == "int32 ");
    }
    
    SECTION("Error: Invalid Top-Level Variable") {
        std::string source = "int32 count = 10;";
        parser::AstTree tree;
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Invalid top-level declaration"));
    }
}
