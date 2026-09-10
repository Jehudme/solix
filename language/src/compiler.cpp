#include "solix/compiler.hpp"
#include <stdexcept>
#include <iostream>

namespace solix::compiler {

Compiler::Compiler() {}

Chunk Compiler::compile(const parser::AstTree& tree) {
    current_chunk = Chunk{};
    
    // Iterate over the global AST nodes
    for (const auto& node : tree.nodes) {
        compileNode(node.get());
    }
    
    // Add a HALT instruction at the end of the global scope execution
    current_chunk.writeOp(OpCode::HALT, 0); // 0 = unknown line for now
    
    return current_chunk;
}

void Compiler::compileNode(parser::Node* node) {
    if (!node) return;
    
    // Dispatch to statement or expression compiler
    switch (node->node_type) {
        // Statements
        case parser::NodeType::EXPRESSION_STATEMENT:
        case parser::NodeType::VARIABLE_DECLARATION:
        case parser::NodeType::BLOCK_STATEMENT:
        case parser::NodeType::IF_STATEMENT:
        case parser::NodeType::WHILE_STATEMENT:
        case parser::NodeType::FOR_STATEMENT:
        case parser::NodeType::RETURN_STATEMENT:
        case parser::NodeType::BREAK_STATEMENT:
        case parser::NodeType::CONTINUE_STATEMENT:
            compileStatement(node);
            break;
            
        // Expressions
        case parser::NodeType::BINARY_EXPRESSION:
        case parser::NodeType::UNARY_EXPRESSION:
        case parser::NodeType::LITERAL_EXPRESSION:
        case parser::NodeType::IDENTIFIER_EXPRESSION:
        case parser::NodeType::ASSIGNMENT_EXPRESSION:
        case parser::NodeType::CALL_EXPRESSION:
        case parser::NodeType::MEMBER_ACCESS_EXPRESSION:
        case parser::NodeType::NEW_INSTANCE_EXPRESSION:
        case parser::NodeType::ARRAY_ACCESS_EXPRESSION:
        case parser::NodeType::ARRAY_CREATION_EXPRESSION:
        case parser::NodeType::ARRAY_LITERAL_EXPRESSION:
        case parser::NodeType::TERNARY_EXPRESSION:
        case parser::NodeType::CAST_EXPRESSION:
            compileExpression(node);
            break;
            
        // Declarations (Classes, Methods, Packages, Enums, Aliases)
        // Note: For now, we just skip these or handle them elsewhere
        case parser::NodeType::CLASS_DECLARATION:
        case parser::NodeType::METHOD_DECLARATION:
        case parser::NodeType::FIELD_DECLARATION:
        case parser::NodeType::ENUM_DECLARATION:
        case parser::NodeType::PACKAGE_STATEMENT:
        case parser::NodeType::ALIAS_STATEMENT:
        case parser::NodeType::CONSTRUCTOR_DECLARATION:
            // TODO: Handle compiling methods into their own Chunks later
            break;
            
        default:
            throw std::runtime_error("Compiler error: Unhandled AST Node type");
    }
}

void Compiler::compileStatement(parser::Node* stmt) {
    // TODO: Handle statements
}

void Compiler::compileExpression(parser::Node* expr) {
    // TODO: Handle expressions
}

} // namespace solix::compiler
