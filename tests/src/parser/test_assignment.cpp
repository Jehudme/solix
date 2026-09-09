#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/lexer.hpp"

using namespace solix;

TEST_CASE("Parser: AssignmentExpression", "[parser][assignment]") {
    SECTION("Valid Assignment") {
        std::string source = "count = 10;";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        REQUIRE(tree.nodes.size() == 1);
        auto* expr_stmt = dynamic_cast<parser::ExpressionStatement*>(tree.nodes[0].get());
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
        std::string source = "nums[0] = 5;";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        REQUIRE(tree.nodes.size() == 1);
        auto* expr_stmt = dynamic_cast<parser::ExpressionStatement*>(tree.nodes[0].get());
        REQUIRE(expr_stmt != nullptr);
        
        auto* assign = dynamic_cast<parser::AssignmentExpression*>(expr_stmt->expression.get());
        REQUIRE(assign != nullptr);
        
        auto* target = dynamic_cast<parser::ArrayAccessExpression*>(assign->target.get());
        REQUIRE(target != nullptr);
    }
}
