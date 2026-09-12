#include "solix/compiler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace solix;
using namespace solix::compiler;

int main() {
    std::ifstream file("tests/resources/test.slx");
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string code = buffer.str();
    
    parser::AstTree ast_tree;
    ast_tree.include(std::string_view(code));
    
    for (const auto& node : ast_tree.nodes) {
        if (node->node_type == parser::NodeType::CLASS_DECLARATION) {
            auto class_decl = static_cast<parser::ClassDeclaration*>(node.get());
            std::cout << "Class: " << class_decl->class_name << "\n";
            for (const auto& child : class_decl->children) {
                if (child->node_type == parser::NodeType::FIELD_DECLARATION) {
                    auto field = static_cast<parser::FieldDeclaration*>(child.get());
                    std::cout << "  Field: " << field->field_name << "\n";
                }
            }
        }
    }
    return 0;
}
