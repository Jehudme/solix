#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/lexer.hpp"

using namespace solix;

TEST_CASE("Parser: ControlFlow", "[parser][controlflow]") {
    SECTION("If Statement") {
        std::string source = "class Test { public void method() { if (x > 0) { return 1; } else { return 0; } } }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        auto* cls = dynamic_cast<parser::ClassDeclaration*>(tree.nodes[0].get());
        auto* method = dynamic_cast<parser::MethodDeclaration*>(cls->children[0].get());
        auto* block = dynamic_cast<parser::BlockStatement*>(method->children[0].get());
        
        auto* if_stmt = dynamic_cast<parser::IfStatement*>(block->children[0].get());
        REQUIRE(if_stmt != nullptr);
        REQUIRE(if_stmt->condition != nullptr);
        REQUIRE(if_stmt->then_branch != nullptr);
        REQUIRE(if_stmt->else_branch != nullptr);
    }

    SECTION("While Statement") {
        std::string source = "class Test { public void method() { while (running) { count = count + 1; } } }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        auto* cls = dynamic_cast<parser::ClassDeclaration*>(tree.nodes[0].get());
        auto* method = dynamic_cast<parser::MethodDeclaration*>(cls->children[0].get());
        auto* block = dynamic_cast<parser::BlockStatement*>(method->children[0].get());
        
        auto* while_stmt = dynamic_cast<parser::WhileStatement*>(block->children[0].get());
        REQUIRE(while_stmt != nullptr);
        REQUIRE(while_stmt->condition != nullptr);
        REQUIRE(while_stmt->body != nullptr);
    }

    SECTION("For Statement") {
        std::string source = "class Test { public void method() { for (int32 i = 0; i < 10; i = i + 1) { print(i); } } }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        auto* cls = dynamic_cast<parser::ClassDeclaration*>(tree.nodes[0].get());
        auto* method = dynamic_cast<parser::MethodDeclaration*>(cls->children[0].get());
        auto* block = dynamic_cast<parser::BlockStatement*>(method->children[0].get());
        
        auto* for_stmt = dynamic_cast<parser::ForStatement*>(block->children[0].get());
        REQUIRE(for_stmt != nullptr);
        REQUIRE(for_stmt->initialization != nullptr);
        REQUIRE(for_stmt->condition != nullptr);
        REQUIRE(for_stmt->iteration != nullptr);
        REQUIRE(for_stmt->body != nullptr);
    }
    
    SECTION("Error: Invalid Top-Level Control Flow") {
        std::string source = "if (x > 0) { return 1; }";
        parser::AstTree tree;
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Invalid top-level declaration"));
    }
}
