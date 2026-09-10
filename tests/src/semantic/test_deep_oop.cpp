#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/semantic.hpp"

using namespace solix;

TEST_CASE("Semantic: Deep OOP Resolution", "[semantic][oop]") {
    SECTION("Valid Member Access") {
        std::string source = "package com.test; class Data { public int32 id; } class App { public void run() { Data d; int32 v = d.id; } }";
        parser::AstTree tree;
        tree.include(std::string_view(source));
        
        semantic::SemanticAnalyzer analyzer;
        REQUIRE_NOTHROW(analyzer.analyze(tree));
    }

    SECTION("Error: Private Member Access") {
        std::string source = "package com.test; class Data { private int32 id; } class App { public void run() { Data d; int32 v = d.id; } }";
        parser::AstTree tree;
        tree.include(std::string_view(source));
        
        semantic::SemanticAnalyzer analyzer;
        REQUIRE_THROWS_WITH(analyzer.analyze(tree), Catch::Matchers::ContainsSubstring("Cannot access private member outside its class"));
    }

    SECTION("Valid Private Access Within Same Class") {
        std::string source = "package com.test; class Data { private int32 id; public void run() { int32 v = this.id; } }";
        parser::AstTree tree;
        tree.include(std::string_view(source));
        
        semantic::SemanticAnalyzer analyzer;
        REQUIRE_NOTHROW(analyzer.analyze(tree));
    }

    SECTION("Error: Type Mismatch Assignment") {
        std::string source = "class App { public void run() { int32 v = \"hello\"; } }";
        parser::AstTree tree;
        tree.include(std::string_view(source));
        
        semantic::SemanticAnalyzer analyzer;
        REQUIRE_THROWS_WITH(analyzer.analyze(tree), Catch::Matchers::ContainsSubstring("Type mismatch in assignment"));
    }
    
    SECTION("Error: Undefined Member Access") {
        std::string source = "class Data {} class App { public void run() { Data d; d.not_exist; } }";
        parser::AstTree tree;
        tree.include(std::string_view(source));
        
        semantic::SemanticAnalyzer analyzer;
        REQUIRE_THROWS_WITH(analyzer.analyze(tree), Catch::Matchers::ContainsSubstring("Undefined member: not_exist"));
    }
    
    SECTION("Error: Invalid L-Value") {
        std::string source = "class App { public void run() { 5 = 10; } }";
        parser::AstTree tree;
        tree.include(std::string_view(source));
        
        semantic::SemanticAnalyzer analyzer;
        REQUIRE_THROWS_WITH(analyzer.analyze(tree), Catch::Matchers::ContainsSubstring("Invalid assignment target"));
    }
}
