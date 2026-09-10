#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/semantic.hpp"

using namespace solix;

TEST_CASE("Semantic: Control Flow Limits (Pass 3)", "[semantic][controlflow]") {
    SECTION("Error: Break outside loop") {
        std::string source = "class Test { public void run() { break; } }";
        parser::AstTree tree;
        tree.include(std::string_view(source));
        
        semantic::SemanticAnalyzer analyzer;
        REQUIRE_THROWS_WITH(analyzer.analyze(tree), Catch::Matchers::ContainsSubstring("break/continue statement outside of loop"));
    }

    SECTION("Valid: Break inside while loop") {
        std::string source = "class Test { public void run() { while(true) { break; } } }";
        parser::AstTree tree;
        tree.include(std::string_view(source));
        
        semantic::SemanticAnalyzer analyzer;
        REQUIRE_NOTHROW(analyzer.analyze(tree));
    }
    
    SECTION("Error: Continue inside if outside loop") {
        std::string source = "class Test { public void run() { if (true) { continue; } } }";
        parser::AstTree tree;
        tree.include(std::string_view(source));
        
        semantic::SemanticAnalyzer analyzer;
        REQUIRE_THROWS_WITH(analyzer.analyze(tree), Catch::Matchers::ContainsSubstring("break/continue statement outside of loop"));
    }
}
