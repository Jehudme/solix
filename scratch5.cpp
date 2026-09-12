#include "solix/compiler.hpp"
#include <iostream>

int main() {
    solix::parser::AstTree tree;
    tree.include("tests/resources/test.slx");
    solix::semantic::SemanticAnalyzer analyzer;
    try {
        analyzer.analyze(tree);
    } catch(std::exception& e) {
        std::cout << e.what() << "\n";
    }
}
