#include "solix/compiler.hpp"
#include <iostream>

using namespace solix;

int main() {
    parser::AstTree tree;
    tree.include("tests/resources/test.slx");
    semantic::SemanticAnalyzer analyzer;
    try {
        analyzer.analyze(tree);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        for (const auto& [name, symbol] : tree.symbols) {
            std::cout << "Symbol: " << name << "\n";
        }
    }
    return 0;
}
