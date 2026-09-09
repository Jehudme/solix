#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/lexer.hpp"

using namespace solix;

TEST_CASE("Parser: VariableDeclaration", "[parser][variable]") {
    SECTION("Valid Variable Without Initializer") {
        std::string source = "int32 count;";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        REQUIRE(tree.nodes.size() == 1);
        auto* var = dynamic_cast<parser::VariableDeclaration*>(tree.nodes[0].get());
        REQUIRE(var != nullptr);
        REQUIRE(var->type_name == "int32 ");
        REQUIRE(var->var_name == "count");
        REQUIRE(var->initializer == nullptr);
    }

    SECTION("Valid Variable With Initializer") {
        std::string source = "float64 speed = 1.5;";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        REQUIRE(tree.nodes.size() == 1);
        auto* var = dynamic_cast<parser::VariableDeclaration*>(tree.nodes[0].get());
        REQUIRE(var != nullptr);
        REQUIRE(var->type_name == "float64 ");
        REQUIRE(var->var_name == "speed");
        REQUIRE(var->initializer != nullptr);
    }

    SECTION("Valid Variable Const Modifiers") {
        std::string source = "const int32 id = 100;";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        REQUIRE(tree.nodes.size() == 1);
        auto* var = dynamic_cast<parser::VariableDeclaration*>(tree.nodes[0].get());
        REQUIRE(var != nullptr);
        REQUIRE(var->is_const == true);
        REQUIRE(var->type_name == "int32 ");
    }
}
