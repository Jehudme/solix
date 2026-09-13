#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/parser.hpp"
#include "solix/semantic.hpp"

using namespace solix;

TEST_CASE("Semantic: Type Resolution (Pass 2)", "[semantic][type]") {
    SECTION("Valid Type: Assignment of Literals") {
        std::string source = "class Test { public void run() { int32 a = 5; float64 b = 5.5; char[] c = \"hello\"; bool d = true; } }";
        parser::AstTree tree;
        tree.include(std::string_view(source));
        
        semantic::SemanticAnalyzer analyzer;
        REQUIRE_NOTHROW(analyzer.analyze(tree));
        
        // Assert AST Types were populated correctly
        auto cls = static_cast<parser::ClassDeclaration*>(tree.nodes[0].get());
        auto method = static_cast<parser::MethodDeclaration*>(cls->children[0].get());
        auto block = static_cast<parser::BlockStatement*>(method->children[0].get());
        
        auto varA = static_cast<parser::VariableDeclaration*>(block->children[0].get());
        REQUIRE(varA->initializer->resolved_type == "int32");

        auto varB = static_cast<parser::VariableDeclaration*>(block->children[1].get());
        REQUIRE(varB->initializer->resolved_type == "float64");

        auto varC = static_cast<parser::VariableDeclaration*>(block->children[2].get());
        REQUIRE(varC->initializer->resolved_type == "char");
        REQUIRE(varC->initializer->resolved_array_depth == 1);

        auto varD = static_cast<parser::VariableDeclaration*>(block->children[3].get());
        REQUIRE(varD->initializer->resolved_type == "bool");
    }
}
