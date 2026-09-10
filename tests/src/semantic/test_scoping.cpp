#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/semantic.hpp"

using namespace solix;

TEST_CASE("Semantic: Local Scoping & Shadowing (Pass 2)", "[semantic][scoping]") {
    SECTION("Error: Duplicate Local Variable in Same Scope") {
        std::string source = "class Test { public void run() { int32 x = 5; int32 x = 10; } }";
        parser::AstTree tree;
        tree.include(std::string_view(source));
        
        semantic::SemanticAnalyzer analyzer;
        REQUIRE_THROWS_WITH(analyzer.analyze(tree), Catch::Matchers::ContainsSubstring("Duplicate local variable in same scope: x"));
    }

    SECTION("Valid: Shadowing Local Variable in Nested Block") {
        std::string source = "class Test { public void run() { int32 x = 5; { int32 x = 10; } } }";
        parser::AstTree tree;
        tree.include(std::string_view(source));
        
        semantic::SemanticAnalyzer analyzer;
        REQUIRE_NOTHROW(analyzer.analyze(tree));
    }

    SECTION("Error: Duplicate Parameter") {
        std::string source = "class Test { public void run(int32 x, float64 x) {} }";
        parser::AstTree tree;
        tree.include(std::string_view(source));
        
        semantic::SemanticAnalyzer analyzer;
        REQUIRE_THROWS_WITH(analyzer.analyze(tree), Catch::Matchers::ContainsSubstring("Duplicate local variable in same scope: x"));
    }

    SECTION("Error: Undefined Variable Usage") {
        std::string source = "class Test { public void run() { x = 10; } }";
        parser::AstTree tree;
        tree.include(std::string_view(source));
        
        semantic::SemanticAnalyzer analyzer;
        REQUIRE_THROWS_WITH(analyzer.analyze(tree), Catch::Matchers::ContainsSubstring("Undefined variable: x"));
    }
}
