#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/semantic.hpp"

using namespace solix;

TEST_CASE("Semantic: Global Symbol Registration (Pass 1)", "[semantic][global]") {
    SECTION("Valid Global Symbols") {
        std::string source = "package com.test; class Engine { public float64 temperature; public void start() {} } enum State { ON, OFF }";
        parser::AstTree tree;
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        
        semantic::SemanticAnalyzer analyzer;
        REQUIRE_NOTHROW(analyzer.analyze(tree));
        
        // Assert symbols exist
        REQUIRE(tree.symbols.count("com.test"));
        REQUIRE(tree.symbols.count("com.test.Engine"));
        REQUIRE(tree.symbols.count("com.test.Engine.temperature"));
        REQUIRE(tree.symbols.count("com.test.State"));
    }

    SECTION("Error: Duplicate Class") {
        std::string source = "class Engine {} class Engine {}";
        parser::AstTree tree;
        tree.include(std::string_view(source));
        
        semantic::SemanticAnalyzer analyzer;
        REQUIRE_THROWS_WITH(analyzer.analyze(tree), Catch::Matchers::ContainsSubstring("Duplicate global symbol: Engine"));
    }

    SECTION("Error: Duplicate Field") {
        std::string source = "class Engine { public int32 x = 1; public int32 x = 2; }";
        parser::AstTree tree;
        tree.include(std::string_view(source));
        
        semantic::SemanticAnalyzer analyzer;
        REQUIRE_THROWS_WITH(analyzer.analyze(tree), Catch::Matchers::ContainsSubstring("Duplicate field symbol: Engine.x"));
    }
}
