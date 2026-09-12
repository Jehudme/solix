#include "solix/compiler.hpp"
#include <stdexcept>
#include <iostream>

namespace solix {
namespace compiler {

uint32_t Compiler::emitConstant(const ConstantValue& value) {
    program.constants.push_back(value);
    return program.constants.size() - 1;
}

void Compiler::emitByte(uint8_t byte) {
    program.flat_bytecode.push_back(byte);
}

void Compiler::emitInt(uint32_t value) {
    program.flat_bytecode.push_back((value >> 24) & 0xFF);
    program.flat_bytecode.push_back((value >> 16) & 0xFF);
    program.flat_bytecode.push_back((value >> 8) & 0xFF);
    program.flat_bytecode.push_back(value & 0xFF);
}

void Compiler::applyLinkerPatches() {
    for (const auto& patch : linker_patches) {
        size_t hole_index = patch.first;
        parser::Node* target_func = patch.second;
        
        if (function_ips.find(target_func) == function_ips.end()) {
            throw std::runtime_error("Linker Error: Unresolved function call!");
        }
        
        uint32_t target_ip = function_ips[target_func];
        
        // Overwrite the 0xFFFFFFFF hole
        program.flat_bytecode[hole_index] = (target_ip >> 24) & 0xFF;
        program.flat_bytecode[hole_index + 1] = (target_ip >> 16) & 0xFF;
        program.flat_bytecode[hole_index + 2] = (target_ip >> 8) & 0xFF;
        program.flat_bytecode[hole_index + 3] = target_ip & 0xFF;
    }
}

BytecodeProgram Compiler::compile(parser::AstTree& ast) {
    program = BytecodeProgram{};
    function_ips.clear();
    linker_patches.clear();

    // 1. Compile all classes and functions
    for (const auto& node : ast.nodes) {
        if (node->node_type == parser::NodeType::CLASS_DECLARATION) {
            auto class_decl = static_cast<parser::ClassDeclaration*>(node.get());
            for (const auto& child : class_decl->children) {
                if (child->node_type == parser::NodeType::METHOD_DECLARATION || 
                    child->node_type == parser::NodeType::CONSTRUCTOR_DECLARATION) {
                    compileNode(child.get());
                }
            }
        } else if (node->node_type == parser::NodeType::METHOD_DECLARATION) {
            compileNode(node.get());
        }
    }
    
    // 2. Link
    applyLinkerPatches();
    
    return std::move(program);
}

void Compiler::compileNode(parser::Node* node) {
    if (!node) return;
    
    if (node->node_type == parser::NodeType::METHOD_DECLARATION) {
        auto method = static_cast<parser::MethodDeclaration*>(node);
        // Record where this function starts
        function_ips[node] = program.flat_bytecode.size();
        
        for (const auto& child : method->children) {
            compileNode(child.get());
        }
        emitByte(static_cast<uint8_t>(OpCode::RETURN));
    }
    else if (node->node_type == parser::NodeType::CONSTRUCTOR_DECLARATION) {
        auto ctor = static_cast<parser::ConstructorDeclaration*>(node);
        function_ips[node] = program.flat_bytecode.size();
        
        for (const auto& child : ctor->children) {
            compileNode(child.get());
        }
        emitByte(static_cast<uint8_t>(OpCode::RETURN));
    }
    else if (node->node_type == parser::NodeType::BLOCK_STATEMENT) {
        for (const auto& child : node->children) {
            compileNode(child.get());
        }
        // At end of block, we should inject REMOVE_REF for local variables!
        // Future feature.
    }
    else if (node->node_type == parser::NodeType::VARIABLE_DECLARATION) {
        auto var_decl = static_cast<parser::VariableDeclaration*>(node);
        if (var_decl->initializer) {
            compileExpression(var_decl->initializer.get());
            emitByte(static_cast<uint8_t>(OpCode::SET_LOCAL));
            emitInt(var_decl->memory_index);
        }
    }
    else if (node->node_type == parser::NodeType::EXPRESSION_STATEMENT) {
        compileExpression(node->children[0].get());
        emitByte(static_cast<uint8_t>(OpCode::POP));
    }
    else if (node->node_type == parser::NodeType::RETURN_STATEMENT) {
        if (!node->children.empty()) {
            compileExpression(node->children[0].get());
        }
        emitByte(static_cast<uint8_t>(OpCode::RETURN));
    }
    // ... Implement If, While, Do-While, etc.
}

void Compiler::compileExpression(parser::Node* expr) {
    if (!expr) return;
    
    if (expr->node_type == parser::NodeType::BINARY_EXPRESSION) {
        auto bin = static_cast<parser::BinaryExpression*>(expr);
        compileExpression(bin->left.get());
        compileExpression(bin->right.get());
        
        switch (bin->op) {
            case lexer::TokenType::OPERATOR_PLUS: emitByte(static_cast<uint8_t>(OpCode::ADD)); break;
            case lexer::TokenType::OPERATOR_MINUS: emitByte(static_cast<uint8_t>(OpCode::SUBTRACT)); break;
            case lexer::TokenType::OPERATOR_MULTIPLY: emitByte(static_cast<uint8_t>(OpCode::MULTIPLY)); break;
            case lexer::TokenType::OPERATOR_DIVIDE: emitByte(static_cast<uint8_t>(OpCode::DIVIDE)); break;
            case lexer::TokenType::OPERATOR_EQUAL: emitByte(static_cast<uint8_t>(OpCode::EQUAL)); break;
            default: throw std::runtime_error("Unsupported binary operator in compiler");
        }
    }
    else if (expr->node_type == parser::NodeType::LITERAL_EXPRESSION) {
        auto lit = static_cast<parser::LiteralExpression*>(expr);
        if (lit->token.type == lexer::TokenType::NUMBER) {
            uint32_t c_idx = emitConstant(static_cast<int64_t>(std::stoll(lit->token.value.value())));
            emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST));
            emitInt(c_idx);
        } else if (lit->token.type == lexer::TokenType::IDENTIFIER && lit->token.value == "null") {
            emitByte(static_cast<uint8_t>(OpCode::PUSH_NULL));
        }
    }
    else if (expr->node_type == parser::NodeType::IDENTIFIER_EXPRESSION) {
        auto ident = static_cast<parser::IdentifierExpression*>(expr);
        if (ident->resolved_declaration) {
            if (ident->resolved_declaration->node_type == parser::NodeType::FIELD_DECLARATION) {
                auto field = static_cast<parser::FieldDeclaration*>(ident->resolved_declaration);
                if (field->is_static) {
                    emitByte(static_cast<uint8_t>(OpCode::GET_GLOBAL));
                    emitInt(field->memory_index);
                } else {
                    emitByte(static_cast<uint8_t>(OpCode::GET_PROPERTY));
                    emitInt(field->memory_index);
                }
            } else if (ident->resolved_declaration->node_type == parser::NodeType::VARIABLE_DECLARATION) {
                auto var = static_cast<parser::VariableDeclaration*>(ident->resolved_declaration);
                emitByte(static_cast<uint8_t>(OpCode::GET_LOCAL));
                emitInt(var->memory_index);
            }
        }
    }
    else if (expr->node_type == parser::NodeType::ASSIGNMENT_EXPRESSION) {
        auto assign = static_cast<parser::AssignmentExpression*>(expr);
        compileExpression(assign->value.get());
        
        if (assign->target->node_type == parser::NodeType::IDENTIFIER_EXPRESSION) {
            auto ident = static_cast<parser::IdentifierExpression*>(assign->target.get());
            if (ident->resolved_declaration->node_type == parser::NodeType::VARIABLE_DECLARATION) {
                auto var = static_cast<parser::VariableDeclaration*>(ident->resolved_declaration);
                emitByte(static_cast<uint8_t>(OpCode::SET_LOCAL));
                emitInt(var->memory_index);
            }
        }
    }
    else if (expr->node_type == parser::NodeType::CALL_EXPRESSION) {
        auto call = static_cast<parser::CallExpression*>(expr);
        
        // Push arguments
        for (const auto& arg : call->arguments) {
            compileExpression(arg.get());
        }
        
        auto target_method = static_cast<parser::MethodDeclaration*>(call->resolved_declaration);
        
        // Push destination IP placeholder
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST));
        linker_patches.push_back(std::make_pair(program.flat_bytecode.size(), target_method));
        emitInt(0xFFFFFFFF); // Hole to be patched
        
        // Push Frame Size
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST));
        emitInt(target_method->frame_size);
        
        // Push Arg Count
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST));
        emitInt(call->arguments.size() + (target_method->is_static ? 0 : 1));
        
        emitByte(static_cast<uint8_t>(OpCode::CALL));
    }
    else if (expr->node_type == parser::NodeType::NEW_INSTANCE_EXPRESSION) {
        auto inst = static_cast<parser::NewInstanceExpression*>(expr);
        auto class_decl = static_cast<parser::ClassDeclaration*>(inst->resolved_declaration);
        
        // 1. Allocate memory
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST));
        emitInt(class_decl->instance_size);
        emitByte(static_cast<uint8_t>(OpCode::ALLOC_DYNAMIC));
        
        // 2. Duplicate pointer
        emitByte(static_cast<uint8_t>(OpCode::DUP));
        
        // 3. Push args
        for (const auto& arg : inst->arguments) {
            compileExpression(arg.get());
        }
        
        // 4. Push constructor call data
        auto ctor = static_cast<parser::ConstructorDeclaration*>(inst->resolved_constructor);
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST));
        linker_patches.push_back(std::make_pair(program.flat_bytecode.size(), ctor));
        emitInt(0xFFFFFFFF);
        
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST));
        emitInt(ctor->frame_size);
        
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST));
        emitInt(inst->arguments.size() + 1); // +1 for 'this'
        
        emitByte(static_cast<uint8_t>(OpCode::CALL));
    }
}

} // namespace compiler
} // namespace solix
