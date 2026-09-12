#include "solix/compiler.hpp"
#include <iostream>

void print_node(solix::parser::Node* node, int depth = 0) {
    if (!node) return;
    for(int i=0;i<depth;i++) std::cout << "  ";
    std::cout << node->node_type << "\n";
    if (node->node_type == solix::parser::NodeType::CLASS_DECLARATION) {
        auto cls = static_cast<solix::parser::ClassDeclaration*>(node);
        for(int i=0;i<depth;i++) std::cout << "  ";
        std::cout << "Class: " << cls->class_name << " symbol: " << cls->symbol_name << "\n";
    }
    for (auto& child : node->children) {
        print_node(child.get(), depth + 1);
    }
}

int main() {
    solix::parser::AstTree tree;
    tree.include("tests/resources/test.slx");
    solix::semantic::SemanticAnalyzer analyzer;
    try {
        analyzer.analyze(tree);
    } catch(...) {}
    for (auto& n : tree.nodes) {
        print_node(n.get());
    }
}
