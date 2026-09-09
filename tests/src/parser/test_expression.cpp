#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/lexer.hpp"

using namespace solix;

TEST_CASE("Parser: Expression", "[parser][expression]") {
    SECTION("Binary Expression") {
        std::string source = "class Test { public void method() { a + b * c; } }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        auto* cls = dynamic_cast<parser::ClassDeclaration*>(tree.nodes[0].get());
        auto* method = dynamic_cast<parser::MethodDeclaration*>(cls->children[0].get());
        auto* block = dynamic_cast<parser::BlockStatement*>(method->children[0].get());
        
        auto* expr_stmt = dynamic_cast<parser::ExpressionStatement*>(block->children[0].get());
        REQUIRE(expr_stmt != nullptr);
        
        auto* binary = dynamic_cast<parser::BinaryExpression*>(expr_stmt->expression.get());
        REQUIRE(binary != nullptr);
        REQUIRE(binary->op == lexer::TokenType::OPERATOR_PLUS);
        
        auto* left = dynamic_cast<parser::IdentifierExpression*>(binary->left.get());
        REQUIRE(left != nullptr);
        REQUIRE(left->name == "a");
        
        auto* right = dynamic_cast<parser::BinaryExpression*>(binary->right.get());
        REQUIRE(right != nullptr);
        REQUIRE(right->op == lexer::TokenType::OPERATOR_MULTIPLY);
    }

    SECTION("Call Expression") {
        std::string source = "class Test { public void method() { print(123, \"hello\"); } }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        auto* cls = dynamic_cast<parser::ClassDeclaration*>(tree.nodes[0].get());
        auto* method = dynamic_cast<parser::MethodDeclaration*>(cls->children[0].get());
        auto* block = dynamic_cast<parser::BlockStatement*>(method->children[0].get());
        
        auto* expr_stmt = dynamic_cast<parser::ExpressionStatement*>(block->children[0].get());
        REQUIRE(expr_stmt != nullptr);
        
        auto* call = dynamic_cast<parser::CallExpression*>(expr_stmt->expression.get());
        REQUIRE(call != nullptr);
        REQUIRE(call->arguments.size() == 2);
    }

    SECTION("Member Access") {
        std::string source = "class Test { public void method() { engine.start(); } }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        auto* cls = dynamic_cast<parser::ClassDeclaration*>(tree.nodes[0].get());
        auto* method = dynamic_cast<parser::MethodDeclaration*>(cls->children[0].get());
        auto* block = dynamic_cast<parser::BlockStatement*>(method->children[0].get());
        
        auto* expr_stmt = dynamic_cast<parser::ExpressionStatement*>(block->children[0].get());
        REQUIRE(expr_stmt != nullptr);
        
        auto* call = dynamic_cast<parser::CallExpression*>(expr_stmt->expression.get());
        REQUIRE(call != nullptr);
        
        auto* member = dynamic_cast<parser::MemberAccessExpression*>(call->callee.get());
        REQUIRE(member != nullptr);
        REQUIRE(member->member_name == "start");
    }
    
    SECTION("Error: Invalid Top-Level Expression") {
        std::string source = "a + b * c;";
        parser::AstTree tree;
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Invalid top-level declaration"));
    }
}
