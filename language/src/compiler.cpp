#include "solix/compiler.hpp"
#include "solix/lexer.hpp"
#include "solix/parser.hpp"
#include "solix/semantic.hpp"
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <sstream>

namespace solix {
namespace compiler {

void Compiler::emitByte(uint8_t byte) {
    bytecode.push_back(byte);
}

void Compiler::emitInt32(uint32_t value) {
    bytecode.push_back((value >> 24) & 0xFF);
    bytecode.push_back((value >> 16) & 0xFF);
    bytecode.push_back((value >> 8) & 0xFF);
    bytecode.push_back(value & 0xFF);
}

void Compiler::emitInt64(uint64_t value) {
    bytecode.push_back((value >> 56) & 0xFF);
    bytecode.push_back((value >> 48) & 0xFF);
    bytecode.push_back((value >> 40) & 0xFF);
    bytecode.push_back((value >> 32) & 0xFF);
    bytecode.push_back((value >> 24) & 0xFF);
    bytecode.push_back((value >> 16) & 0xFF);
    bytecode.push_back((value >> 8) & 0xFF);
    bytecode.push_back(value & 0xFF);
}

void Compiler::emitFloat32(float value) {
    uint32_t bit_representation;
    std::memcpy(&bit_representation, &value, sizeof(float));
    emitInt32(bit_representation);
}

void Compiler::emitFloat64(double value) {
    uint64_t bit_representation;
    std::memcpy(&bit_representation, &value, sizeof(double));
    emitInt64(bit_representation);
}

void Compiler::emitString(const std::string& value) {
    emitInt32(static_cast<uint32_t>(value.size())); // Size of string
    for (char c : value) {
        emitByte(static_cast<uint8_t>(c)); // The raw bytes of the string
    }
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
        bytecode[hole_index] = (target_ip >> 24) & 0xFF;
        bytecode[hole_index + 1] = (target_ip >> 16) & 0xFF;
        bytecode[hole_index + 2] = (target_ip >> 8) & 0xFF;
        bytecode[hole_index + 3] = target_ip & 0xFF;
    }
}

std::vector<uint8_t> Compiler::compile(std::string_view source_code, std::string_view entry_point) {
    // 1. Lex and Parse
    ast_tree = parser::AstTree();
    ast_tree.include(source_code);
    
    // 2. Semantic Analysis
    semantic::SemanticAnalyzer analyzer;
    analyzer.analyze(ast_tree);
    
    // 3. Setup compiler state
    bytecode.clear();
    function_ips.clear();
    linker_patches.clear();

    // 4. Compile boot sequence
    compileBootSequence(entry_point);
    
    // 5. Compile all methods/functions
    for (const auto& node : ast_tree.nodes) {
        if (node->node_type == parser::NodeType::CLASS_DECLARATION) {
            auto class_decl = static_cast<parser::ClassDeclaration*>(node.get());
            for (const auto& child : class_decl->children) {
                if (child->node_type == parser::NodeType::METHOD_DECLARATION || 
                    child->node_type == parser::NodeType::CONSTRUCTOR_DECLARATION) {
                    compileFunction(child.get());
                }
            }
        }
    }
    
    // 6. Link function calls
    applyLinkerPatches();
    
    return std::move(bytecode);
}

void Compiler::compileBootSequence(std::string_view entry_point) {
    // Collect all static fields to know how much memory to allocate
    uint32_t static_count = 0;
    std::vector<parser::FieldDeclaration*> static_fields;
    
    for (const auto& node : ast_tree.nodes) {
        if (node->node_type == parser::NodeType::CLASS_DECLARATION) {
            auto class_decl = static_cast<parser::ClassDeclaration*>(node.get());
            for (const auto& child : class_decl->children) {
                if (child->node_type == parser::NodeType::FIELD_DECLARATION) {
                    auto field = static_cast<parser::FieldDeclaration*>(child.get());
                    if (field->is_static) {
                        static_count++;
                        static_fields.push_back(field);
                    }
                }
            }
        }
    }
    
    // Allocate global memory (+1 because memory_index starts at 1, 0 is null)
    emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
    uint32_t total_globals = 1; 
    for (auto* field : static_fields) {
        if (field->memory_index >= total_globals) {
            total_globals = field->memory_index + 1;
        }
    }
    emitInt32(total_globals);
    emitByte(static_cast<uint8_t>(OpCode::ALLOC_STATIC));
    
    // Assign global variables
    for (auto* field : static_fields) {
        if (field->initializer) {
            compileExpression(field->initializer.get());
            emitByte(static_cast<uint8_t>(OpCode::SET_GLOBAL));
            emitInt32(field->memory_index);
        }
    }
    
    // Call entry point
    if (!entry_point.empty()) {
        parser::MethodDeclaration* entry_method = nullptr;
        for (const auto& node : ast_tree.nodes) {
            if (node->node_type == parser::NodeType::CLASS_DECLARATION) {
                auto class_decl = static_cast<parser::ClassDeclaration*>(node.get());
                for (const auto& child : class_decl->children) {
                    if (child->node_type == parser::NodeType::METHOD_DECLARATION) {
                        auto method = static_cast<parser::MethodDeclaration*>(child.get());
                        if (method->method_name == entry_point) {
                            entry_method = method;
                            break;
                        }
                    }
                }
            }
        }
        
        if (!entry_method) {
            throw std::runtime_error("Entry point not found: " + std::string(entry_point));
        }
        if (!entry_method->is_static) {
            throw std::runtime_error("Entry point must be static");
        }
        
        // PUSH IP Placeholder
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
        linker_patches.push_back(std::make_pair(bytecode.size(), entry_method));
        emitInt32(0xFFFFFFFF);
        
        // Push Frame Size
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
        emitInt32(entry_method->frame_size);
        
        // Push Arg Count (0 arguments for main)
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
        emitInt32(0);
        
        emitByte(static_cast<uint8_t>(OpCode::CALL));
    }
    
    emitByte(static_cast<uint8_t>(OpCode::HALT));
}

void Compiler::compileFunction(parser::Node* function_node) {
    function_ips[function_node] = bytecode.size();
    
    if (function_node->node_type == parser::NodeType::METHOD_DECLARATION) {
        auto method = static_cast<parser::MethodDeclaration*>(function_node);
        for (const auto& child : method->children) compileNode(child.get());
    } else if (function_node->node_type == parser::NodeType::CONSTRUCTOR_DECLARATION) {
        auto ctor = static_cast<parser::ConstructorDeclaration*>(function_node);
        for (const auto& child : ctor->children) compileNode(child.get());
    }
    
    emitByte(static_cast<uint8_t>(OpCode::RETURN));
}

void Compiler::compileNode(parser::Node* node) {
    if (!node) return;
    
    if (node->node_type == parser::NodeType::BLOCK_STATEMENT) {
        for (const auto& child : node->children) {
            compileNode(child.get());
        }
    }
    else if (node->node_type == parser::NodeType::VARIABLE_DECLARATION) {
        auto var_decl = static_cast<parser::VariableDeclaration*>(node);
        if (var_decl->initializer) {
            compileExpression(var_decl->initializer.get());
            emitByte(static_cast<uint8_t>(OpCode::SET_LOCAL));
            emitInt32(var_decl->memory_index);
            
            if (var_decl->is_reference_type) {
                // ARC Retain (INC_REF) - The VM will likely do this internally during SET_LOCAL or it might expect explicit instructions.
                // The user said: ADD_REF <addr> execute every time that a class instance is referenced in a frame.
                emitByte(static_cast<uint8_t>(OpCode::GET_LOCAL));
                emitInt32(var_decl->memory_index);
                emitByte(static_cast<uint8_t>(OpCode::INC_REF));
            }
        }
    }
    else if (node->node_type == parser::NodeType::EXPRESSION_STATEMENT) {
        auto expr_stmt = static_cast<parser::ExpressionStatement*>(node);
        compileExpression(expr_stmt->expression.get());
        emitByte(static_cast<uint8_t>(OpCode::POP));
    }
    else if (node->node_type == parser::NodeType::RETURN_STATEMENT) {
        auto ret_stmt = static_cast<parser::ReturnStatement*>(node);
        if (ret_stmt->value) {
            compileExpression(ret_stmt->value.get());
        }
        emitByte(static_cast<uint8_t>(OpCode::RETURN));
    }
    // ... Implement If, While, Do-While, etc. if required
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
            case lexer::TokenType::OPERATOR_NOT_EQUAL: emitByte(static_cast<uint8_t>(OpCode::NOT_EQUAL)); break;
            case lexer::TokenType::OPERATOR_GREATER_THAN: emitByte(static_cast<uint8_t>(OpCode::GREATER)); break;
            case lexer::TokenType::OPERATOR_GREATER_EQUAL: emitByte(static_cast<uint8_t>(OpCode::GREATER_EQUAL)); break;
            case lexer::TokenType::OPERATOR_LESS_THAN: emitByte(static_cast<uint8_t>(OpCode::LESS)); break;
            case lexer::TokenType::OPERATOR_LESS_EQUAL: emitByte(static_cast<uint8_t>(OpCode::LESS_EQUAL)); break;
            default: throw std::runtime_error("Unsupported binary operator in compiler");
        }
    }
    else if (expr->node_type == parser::NodeType::CAST_EXPRESSION) {
        auto cast_expr = static_cast<parser::CastExpression*>(expr);
        compileExpression(cast_expr->expression.get());
        
        std::string target = cast_expr->target_type;
        if (target == "int8") emitByte(static_cast<uint8_t>(OpCode::CONV_I8));
        else if (target == "int16") emitByte(static_cast<uint8_t>(OpCode::CONV_I16));
        else if (target == "int32") emitByte(static_cast<uint8_t>(OpCode::CONV_I32));
        else if (target == "int64") emitByte(static_cast<uint8_t>(OpCode::CONV_I64));
        else if (target == "uint8") emitByte(static_cast<uint8_t>(OpCode::CONV_U8));
        else if (target == "uint16") emitByte(static_cast<uint8_t>(OpCode::CONV_U16));
        else if (target == "uint32") emitByte(static_cast<uint8_t>(OpCode::CONV_U32));
        else if (target == "uint64") emitByte(static_cast<uint8_t>(OpCode::CONV_U64));
        else if (target == "float32") emitByte(static_cast<uint8_t>(OpCode::CONV_F32));
        else if (target == "float64") emitByte(static_cast<uint8_t>(OpCode::CONV_F64));
        else throw std::runtime_error("Unsupported cast target type in compiler");
    }
    else if (expr->node_type == parser::NodeType::LITERAL_EXPRESSION) {
        auto lit = static_cast<parser::LiteralExpression*>(expr);
        if (lit->token.type == lexer::TokenType::NUMBER) {
            std::string val_str = lit->token.value.value_or("0");
            if (val_str.find('.') != std::string::npos) {
                emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_F64));
                emitFloat64(std::stod(val_str));
            } else {
                emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
                emitInt32(static_cast<uint32_t>(std::stoll(val_str)));
            }
        } else if (lit->token.type == lexer::TokenType::STRING) {
            emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_STRING));
            emitString(lit->token.value.value_or(""));
        }
    }
    else if (expr->node_type == parser::NodeType::IDENTIFIER_EXPRESSION) {
        auto ident = static_cast<parser::IdentifierExpression*>(expr);
        if (ident->name == "true") {
            emitByte(static_cast<uint8_t>(OpCode::PUSH_TRUE));
        } else if (ident->name == "false") {
            emitByte(static_cast<uint8_t>(OpCode::PUSH_FALSE));
        } else if (ident->name == "null") {
            emitByte(static_cast<uint8_t>(OpCode::PUSH_NULL));
        } else if (ident->resolved_declaration) {
            if (ident->resolved_declaration->node_type == parser::NodeType::FIELD_DECLARATION) {
                auto field = static_cast<parser::FieldDeclaration*>(ident->resolved_declaration);
                if (field->is_static) {
                    emitByte(static_cast<uint8_t>(OpCode::GET_GLOBAL));
                    emitInt32(field->memory_index);
                } else {
                    emitByte(static_cast<uint8_t>(OpCode::GET_PROPERTY));
                    emitInt32(field->memory_index);
                }
            } else if (ident->resolved_declaration->node_type == parser::NodeType::VARIABLE_DECLARATION) {
                auto var = static_cast<parser::VariableDeclaration*>(ident->resolved_declaration);
                emitByte(static_cast<uint8_t>(OpCode::GET_LOCAL));
                emitInt32(var->memory_index);
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
                
                if (var->is_reference_type) {
                    emitByte(static_cast<uint8_t>(OpCode::DUP));
                    emitByte(static_cast<uint8_t>(OpCode::INC_REF));
                    
                    emitByte(static_cast<uint8_t>(OpCode::GET_LOCAL));
                    emitInt32(var->memory_index);
                    emitByte(static_cast<uint8_t>(OpCode::DEC_REF));
                }
                
                emitByte(static_cast<uint8_t>(OpCode::SET_LOCAL));
                emitInt32(var->memory_index);
            }
        }
    }
    else if (expr->node_type == parser::NodeType::CALL_EXPRESSION) {
        auto call = static_cast<parser::CallExpression*>(expr);
        for (const auto& arg : call->arguments) compileExpression(arg.get());
        auto target_method = static_cast<parser::MethodDeclaration*>(call->resolved_declaration);
        
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
        linker_patches.push_back(std::make_pair(bytecode.size(), target_method));
        emitInt32(0xFFFFFFFF);
        
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
        emitInt32(target_method->frame_size);
        
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
        emitInt32(call->arguments.size() + (target_method->is_static ? 0 : 1));
        
        emitByte(static_cast<uint8_t>(OpCode::CALL));
    }
    else if (expr->node_type == parser::NodeType::NEW_INSTANCE_EXPRESSION) {
        auto inst = static_cast<parser::NewInstanceExpression*>(expr);
        auto class_decl = static_cast<parser::ClassDeclaration*>(inst->resolved_declaration);
        
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
        emitInt32(class_decl->instance_size);
        emitByte(static_cast<uint8_t>(OpCode::ALLOC_DYNAMIC));
        
        emitByte(static_cast<uint8_t>(OpCode::DUP));
        
        for (const auto& arg : inst->arguments) compileExpression(arg.get());
        auto ctor = static_cast<parser::ConstructorDeclaration*>(inst->resolved_constructor);
        
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
        linker_patches.push_back(std::make_pair(bytecode.size(), ctor));
        emitInt32(0xFFFFFFFF);
        
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
        emitInt32(ctor->frame_size);
        
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
        emitInt32(inst->arguments.size() + 1);
        
        emitByte(static_cast<uint8_t>(OpCode::CALL));
    }
}

// Simple Disassembler for debugging
std::string Compiler::disassemble(const std::vector<uint8_t>& bcode) const {
    std::stringstream ss;
    size_t i = 0;
    while (i < bcode.size()) {
        ss << i << ": ";
        OpCode op = static_cast<OpCode>(bcode[i++]);
        switch (op) {
            case OpCode::PUSH_CONST_I32: {
                uint32_t val = (bcode[i] << 24) | (bcode[i+1] << 16) | (bcode[i+2] << 8) | bcode[i+3];
                i += 4;
                ss << "PUSH_CONST_I32 " << val << "\n";
                break;
            }
            case OpCode::SET_LOCAL: {
                uint32_t val = (bcode[i] << 24) | (bcode[i+1] << 16) | (bcode[i+2] << 8) | bcode[i+3];
                i += 4;
                ss << "SET_LOCAL " << val << "\n";
                break;
            }
            case OpCode::GET_LOCAL: {
                uint32_t val = (bcode[i] << 24) | (bcode[i+1] << 16) | (bcode[i+2] << 8) | bcode[i+3];
                i += 4;
                ss << "GET_LOCAL " << val << "\n";
                break;
            }
            case OpCode::SET_GLOBAL: {
                uint32_t val = (bcode[i] << 24) | (bcode[i+1] << 16) | (bcode[i+2] << 8) | bcode[i+3];
                i += 4;
                ss << "SET_GLOBAL " << val << "\n";
                break;
            }
            case OpCode::ALLOC_STATIC: ss << "ALLOC_STATIC\n"; break;
            case OpCode::HALT: ss << "HALT\n"; break;
            case OpCode::CALL: ss << "CALL\n"; break;
            case OpCode::RETURN: ss << "RETURN\n"; break;
            case OpCode::ADD: ss << "ADD\n"; break;
            case OpCode::POP: ss << "POP\n"; break;
            case OpCode::INC_REF: ss << "INC_REF\n"; break;
            case OpCode::DEC_REF: ss << "DEC_REF\n"; break;
            default: ss << "UNKNOWN (" << static_cast<int>(op) << ")\n"; break;
        }
    }
    return ss.str();
}

} // namespace compiler
} // namespace solix
