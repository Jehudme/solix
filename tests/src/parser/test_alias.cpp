#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/lexer.hpp"

using namespace solix;

TEST_CASE("Parser: AliasStatement", "[parser][alias]") {
    SECTION("Valid Alias Statement") {
        std::string source = "alias IntAlias = int32;";
        parser::AstTree tree;
        
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        REQUIRE(tree.nodes.size() == 1);
        
        auto* alias = dynamic_cast<parser::AliasStatement*>(tree.nodes[0].get());
        REQUIRE(alias != nullptr);
        REQUIRE(alias->alias_name == "IntAlias");
        REQUIRE(alias->target_type == "int32");
    }

    SECTION("Valid Compound Alias") {
        std::string source = "alias ThreadId = System.Threading.Id;";
        parser::AstTree tree;
        
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        REQUIRE(tree.nodes.size() == 1);
        
        auto* alias = dynamic_cast<parser::AliasStatement*>(tree.nodes[0].get());
        REQUIRE(alias != nullptr);
        REQUIRE(alias->alias_name == "ThreadId");
        REQUIRE(alias->target_type == "System.Threading.Id");
    }

    SECTION("Error: Missing equals") {
        std::string source = "alias IntAlias int32;";
        parser::AstTree tree;
        
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Expected '=' in alias declaration"));
    }

    SECTION("Error: Invalid syntax") {
        std::string source = "alias;";
        parser::AstTree tree;
        
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Invalid alias declaration syntax"));
    }
}
