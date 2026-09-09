#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/lexer.hpp"

using namespace solix;

TEST_CASE("Parser: AssignmentExpression", "[parser][assignment]") {
    SECTION("Valid Assignment") {
        std::string source = "class Test { public void method() { count = 10; } }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        auto* cls = dynamic_cast<parser::ClassDeclaration*>(tree.nodes[0].get());
        auto* method = dynamic_cast<parser::MethodDeclaration*>(cls->children[0].get());
        auto* block = dynamic_cast<parser::BlockStatement*>(method->children[0].get());
        
        auto* expr_stmt = dynamic_cast<parser::ExpressionStatement*>(block->children[0].get());
        REQUIRE(expr_stmt != nullptr);
        
        auto* assign = dynamic_cast<parser::AssignmentExpression*>(expr_stmt->expression.get());
        REQUIRE(assign != nullptr);
        
        auto* target = dynamic_cast<parser::IdentifierExpression*>(assign->target.get());
        REQUIRE(target != nullptr);
        REQUIRE(target->name == "count");
        
        auto* value = dynamic_cast<parser::LiteralExpression*>(assign->value.get());
        REQUIRE(value != nullptr);
        REQUIRE(value->token.value.value_or("") == "10");
    }

    SECTION("Valid Array Assignment") {
        std::string source = "class Test { public void method() { nums[0] = 5; } }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        auto* cls = dynamic_cast<parser::ClassDeclaration*>(tree.nodes[0].get());
        auto* method = dynamic_cast<parser::MethodDeclaration*>(cls->children[0].get());
        auto* block = dynamic_cast<parser::BlockStatement*>(method->children[0].get());
        
        auto* expr_stmt = dynamic_cast<parser::ExpressionStatement*>(block->children[0].get());
        REQUIRE(expr_stmt != nullptr);
        
        auto* assign = dynamic_cast<parser::AssignmentExpression*>(expr_stmt->expression.get());
        REQUIRE(assign != nullptr);
        
        auto* target = dynamic_cast<parser::ArrayAccessExpression*>(assign->target.get());
        REQUIRE(target != nullptr);
    }
    
    SECTION("Error: Invalid Top-Level Assignment") {
        std::string source = "count = 10;";
        parser::AstTree tree;
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Invalid top-level declaration"));
    }
}
