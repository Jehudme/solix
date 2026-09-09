#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/lexer.hpp"

using namespace solix;

TEST_CASE("Parser: PackageStatement", "[parser][package]") {
    SECTION("Valid Package Statement") {
        std::string source = "package com.solix.math;";
        parser::AstTree tree;
        
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        REQUIRE(tree.nodes.size() == 1);
        
        auto* pkg = dynamic_cast<parser::PackageStatement*>(tree.nodes[0].get());
        REQUIRE(pkg != nullptr);
        REQUIRE(pkg->package_name == "com.solix.math");
    }

    SECTION("Valid Single Word Package Statement") {
        std::string source = "package math;";
        parser::AstTree tree;
        
        REQUIRE_NOTHROW(tree.include(std::string_view(source)));
        REQUIRE(tree.nodes.size() == 1);
        
        auto* pkg = dynamic_cast<parser::PackageStatement*>(tree.nodes[0].get());
        REQUIRE(pkg != nullptr);
        REQUIRE(pkg->package_name == "math");
    }

    SECTION("Error: Missing semicolon") {
        std::string source = "package com.solix";
        parser::AstTree tree;
        
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Expected ';' after package name")); 
        // Note: Without semicolon, it might be parsed as an expression statement or throw on missing semicolon
    }

    SECTION("Error: Missing package name") {
        std::string source = "package;";
        parser::AstTree tree;
        
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Expected package name"));
    }
}
TEST_CASE("Parser: Package Rules", "[parser][package]") {
    SECTION("Error: Package must be first") {
        std::string source = "class Test {} package a.b.c;";
        parser::AstTree tree;
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("Package statement must be the first statement"));
    }
    
    SECTION("Error: Multiple packages") {
        std::string source = "package a; package b;";
        parser::AstTree tree;
        REQUIRE_THROWS_WITH(tree.include(std::string_view(source)), Catch::Matchers::ContainsSubstring("A file can only have one package statement"));
    }
}
