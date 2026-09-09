#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/lexer.hpp"

using namespace solix;

TEST_CASE("Parser: ControlFlow", "[parser][controlflow]") {
    SECTION("If Statement") {
        std::string source = "if (x > 0) { return 1; } else { return 0; }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        REQUIRE(tree.nodes.size() == 1);
        
        auto* if_stmt = dynamic_cast<parser::IfStatement*>(tree.nodes[0].get());
        REQUIRE(if_stmt != nullptr);
        REQUIRE(if_stmt->condition != nullptr);
        REQUIRE(if_stmt->then_branch != nullptr);
        REQUIRE(if_stmt->else_branch != nullptr);
    }

    SECTION("While Statement") {
        std::string source = "while (running) { count = count + 1; }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        REQUIRE(tree.nodes.size() == 1);
        
        auto* while_stmt = dynamic_cast<parser::WhileStatement*>(tree.nodes[0].get());
        REQUIRE(while_stmt != nullptr);
        REQUIRE(while_stmt->condition != nullptr);
        REQUIRE(while_stmt->body != nullptr);
    }

    SECTION("For Statement") {
        std::string source = "for (int32 i = 0; i < 10; i = i + 1) { print(i); }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        REQUIRE(tree.nodes.size() == 1);
        
        auto* for_stmt = dynamic_cast<parser::ForStatement*>(tree.nodes[0].get());
        REQUIRE(for_stmt != nullptr);
        REQUIRE(for_stmt->initialization != nullptr);
        REQUIRE(for_stmt->condition != nullptr);
        REQUIRE(for_stmt->iteration != nullptr);
        REQUIRE(for_stmt->body != nullptr);
    }
}
