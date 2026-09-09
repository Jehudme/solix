#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"

using namespace solix;

TEST_CASE("Parser: Scoping Errors", "[parser][scoping]") {
    SECTION("Error: Duplicate local variable in same block") {
        std::string source = "class Test { public void method() { const int32 x = 5; const int32 x = 10; } }";
        parser::AstTree tree;
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Duplicate local variable in same scope: x"));
    }

    SECTION("Valid: Shadowing local variable in nested block") {
        std::string source = "class Test { public void method() { const int32 x = 5; { const int32 x = 10; } } }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
    }

    SECTION("Error: Duplicate field in same class") {
        std::string source = "class Test { public int32 x = 5; public int32 x = 10; }";
        parser::AstTree tree;
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Duplicate field symbol: Test.x"));
    }

    SECTION("Error: Duplicate class in same file") {
        std::string source = "class Test { } class Test { }";
        parser::AstTree tree;
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Duplicate global symbol: Test"));
    }
}
