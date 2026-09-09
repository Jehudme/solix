#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/lexer.hpp"

using namespace solix;

TEST_CASE("Parser: Structural Modifiers Rules", "[parser][rules]") {
    SECTION("Error: Access modifiers on local variables") {
        std::string source = "class Test { public void method() { private int32 x = 5; } }";
        parser::AstTree tree;
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Local variables cannot have access modifiers"));
    }

    SECTION("Valid: Const on local variable") {
        std::string source = "class Test { public void method() { const int32 x = 5; } }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
    }

    SECTION("Error: Methods outside of class") {
        std::string source = "public void method() { }";
        parser::AstTree tree;
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Invalid top-level declaration"));
    }
    
    SECTION("Error: Loose statements in class body") {
        std::string source = "class Test { x = 5; }";
        parser::AstTree tree;
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Invalid class member declaration"));
    }
}
