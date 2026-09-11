#include "solix/compiler.hpp"
#include <stdexcept>
#include <iostream>

namespace solix::compiler {

Compiler::Compiler() : current_chunk(nullptr), scope_depth(0) {}

BytecodeProgram Compiler::compile(const parser::AstTree& tree) {
    program = BytecodeProgram{};
    global_variables.clear();
    
    // Create the global / entry-point chunk
    program.functions.push_back(Chunk{"__global__"});
    current_chunk = &program.functions.back();
    program.exports["__global__"] = 0;
    
    // Iterate over the global AST nodes
    for (const auto& node : tree.nodes) {
        compileNode(node.get());
    }
    
    // Add a HALT instruction at the end of the global scope execution
    current_chunk->writeOp(OpCode::HALT, 0);
    
    program.global_variable_count = global_variables.size();
    
    return program;
}

void Compiler::beginScope() {
    scope_depth++;
}

void Compiler::endScope() {
    scope_depth--;
    // Pop all variables declared in the scope that just ended
    while (!locals.empty() && locals.back().depth > scope_depth) {
        current_chunk->writeOp(OpCode::POP, 0); // Drop the value from the VM stack
        locals.pop_back();
    }
}

int Compiler::addLocal(const std::string& name) {
    locals.push_back(Local{name, scope_depth});
    if (locals.size() > current_chunk->max_local_slots) {
        current_chunk->max_local_slots = locals.size();
    }
    return locals.size() - 1;
}

int Compiler::resolveLocal(const std::string& name) {
    // Search backward to find the innermost declaration
    for (int i = locals.size() - 1; i >= 0; i--) {
        if (locals[i].name == name) {
            return i;
        }
    }
    return -1; // Not found (must be a global or field)
}

int Compiler::registerGlobal(const std::string& name) {
    if (global_variables.count(name)) return global_variables[name];
    // +1 because index 0 is reserved for NULL in the VM Memory Pool!
    int index = global_variables.size() + 1; 
    global_variables[name] = index;
    return index;
}

int Compiler::resolveGlobal(const std::string& name) {
    auto it = global_variables.find(name);
    if (it != global_variables.end()) return it->second;
    return -1;
}

void Compiler::compileNode(parser::Node* node) {
    if (!node) return;
    
    // Dispatch to statement or expression compiler
    switch (node->node_type) {
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
            
        case parser::NodeType::CLASS_DECLARATION:
        case parser::NodeType::METHOD_DECLARATION:
        case parser::NodeType::FIELD_DECLARATION:
        case parser::NodeType::ENUM_DECLARATION:
        case parser::NodeType::PACKAGE_STATEMENT:
        case parser::NodeType::ALIAS_STATEMENT:
        case parser::NodeType::CONSTRUCTOR_DECLARATION:
            // TODO: Compile methods into their own slots in program.functions
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
