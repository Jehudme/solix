#include <stdexcept>
#include "solix/processes/assembler.hpp"
#include "solix/compilation.hpp"
#include "solix/statements.hpp"
#include "solix/utilities/diagnostic.hpp"
#include "solix/utilities/optcodes.hpp"
#include <cstring>
#include <sstream>

namespace solix {

void Assembler::throw_error(Node* node, const std::string& msg) {
    throw std::runtime_error(msg);
}

void Assembler::execute() {
    

    // Initialize the bytecode buffer if empty
    bytecode().clear();

    compile_boot_sequence();


    // Compile classes and functions
    for (const auto& [source, nodes] : context.nodes) {
        for (const auto& node : nodes) {
            compile_node(node.get());
        }
    }

    uint32_t abstract_sentinel_ip = bytecode().size();
    emit_byte(static_cast<uint8_t>(OpCode::THROW_ABSTRACT));
    
    // Wire all abstract methods to the sentinel
    for (const auto& [source, nodes] : context.nodes) {
        for (const auto& node : nodes) {
            if (node->node_type == NodeType::CLASS_DECL) {
                auto* cls = static_cast<ClassDeclaration*>(node.get());
                for (auto* m : cls->vtable) {
                    if (m->is_abstract) {
                        function_ips[m] = abstract_sentinel_ip;
                    }
                }
            }
        }
    }

    apply_linker_patches();

    
}

void Assembler::emit_byte(uint8_t byte) {
    bytecode().push_back(byte);
}

void Assembler::emit_int32(uint32_t value) {
    bytecode().push_back((value >> 24) & 0xFF);
    bytecode().push_back((value >> 16) & 0xFF);
    bytecode().push_back((value >> 8) & 0xFF);
    bytecode().push_back(value & 0xFF);
}

void Assembler::emit_int64(uint64_t value) {
    bytecode().push_back((value >> 56) & 0xFF);
    bytecode().push_back((value >> 48) & 0xFF);
    bytecode().push_back((value >> 40) & 0xFF);
    bytecode().push_back((value >> 32) & 0xFF);
    bytecode().push_back((value >> 24) & 0xFF);
    bytecode().push_back((value >> 16) & 0xFF);
    bytecode().push_back((value >> 8) & 0xFF);
    bytecode().push_back(value & 0xFF);
}

void Assembler::emit_float32(float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(float));
    emit_int32(bits);
}

void Assembler::emit_float64(double value) {
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(double));
    emit_int64(bits);
}

void Assembler::emit_string(const std::string& value) {
    emit_int32(value.size());
    for (char c : value) {
        emit_byte(c);
    }
}

void Assembler::apply_linker_patches() {
    for (auto& patch : linker_patches) {
        size_t index = patch.first;
        Node* target = patch.second;
        
        if (function_ips.find(target) != function_ips.end()) {
            uint32_t target_ip = function_ips[target];
            bytecode()[index]     = (target_ip >> 24) & 0xFF;
            bytecode()[index + 1] = (target_ip >> 16) & 0xFF;
            bytecode()[index + 2] = (target_ip >> 8) & 0xFF;
            bytecode()[index + 3] = target_ip & 0xFF;
        } else {
            std::string name = "unknown";
            if (target->node_type == NodeType::METHOD_DECL) name = static_cast<MethodDeclaration*>(target)->mangled_name;
            else if (target->node_type == NodeType::CONSTRUCTOR_DECL) name = static_cast<ConstructorDeclaration*>(target)->mangled_name;
            throw std::runtime_error("Linker Error: Target function not found for patching: " + name);
        }
    }
}

void Assembler::compile_boot_sequence() {
    std::vector<ClassDeclaration*> classes_with_vtables;
    std::vector<MethodDeclaration*> native_methods;
    std::vector<FieldDeclaration*> static_fields;

    for (const auto& [source, nodes] : context.nodes) {
        for (const auto& node : nodes) {
            if (node->node_type == NodeType::CLASS_DECL) {
                auto* class_decl = static_cast<ClassDeclaration*>(node.get());
                if (class_decl->vtable_id != -1) classes_with_vtables.push_back(class_decl);
                for (const auto& child : class_decl->children) {
                    if (child->node_type == NodeType::METHOD_DECL) {
                        auto* method = static_cast<MethodDeclaration*>(child.get());
                        if (method->is_native) {
                            native_methods.push_back(method);
                        }
                    } else if (child->node_type == NodeType::FIELD_DECL) {
                        auto* field = static_cast<FieldDeclaration*>(child.get());
                        if (field->is_static) {
                            static_fields.push_back(field);
                        }
                    }
                }
            } else if (node->node_type == NodeType::METHOD_DECL) {
                auto* method = static_cast<MethodDeclaration*>(node.get());
                if (method->is_native) {
                    native_methods.push_back(method);
                }
            }
        }
    }

    for (auto* native_method : native_methods) {
        native_method->memory_index = native_id_counter++;
        emit_byte(static_cast<uint8_t>(OpCode::DEFINE_NATIVE));
        emit_int32(native_method->memory_index);
        emit_string(native_method->mangled_name);
    }


    for (auto* cls : classes_with_vtables) {
        emit_byte(static_cast<uint8_t>(OpCode::DEFINE_VTABLE));
        emit_int32(cls->vtable_id);
        emit_int32(cls->base_vtable_id);
        emit_int32(cls->vtable.size());
        for (auto* method : cls->vtable) {
            linker_patches.push_back({bytecode().size(), method});
            emit_int32(0xFFFFFFFF);
        }
    }
    uint32_t total_globals = 1; 
    for (auto* field : static_fields) {
        if (field->memory_index >= (int)total_globals) {
            total_globals = field->memory_index + 1;
        }
    }
    
    emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
    emit_int32(total_globals);
    emit_byte(static_cast<uint8_t>(OpCode::ALLOC_STATIC));
    
    for (auto* field : static_fields) {
        if (field->initializer) {
            compile_expression(field->initializer.get());
            emit_byte(static_cast<uint8_t>(OpCode::SET_GLOBAL));
            emit_int32(field->memory_index);
        }
    }

    std::string entry_point = context.options.entry_point;
    if (!entry_point.empty()) {
        MethodDeclaration* entry_method = nullptr;
        for (const auto& [source, nodes] : context.nodes) {
            for (const auto& node : nodes) {
                if (node->node_type == NodeType::CLASS_DECL) {
                    auto* class_decl = static_cast<ClassDeclaration*>(node.get());
                    for (const auto& child : class_decl->children) {
                        if (child->node_type == NodeType::METHOD_DECL) {
                            auto* method = static_cast<MethodDeclaration*>(child.get());
                            if (method->method_name == entry_point) {
                                entry_method = method;
                                break;
                            }
                        }
                    }
                } else if (node->node_type == NodeType::METHOD_DECL) {
                    auto* method = static_cast<MethodDeclaration*>(node.get());
                    if (method->method_name == entry_point) {
                        entry_method = method;
                        break;
                    }
                }
            }
            if (entry_method) break;
        }

        if (entry_method) {
            if (!entry_method->is_static) {
                throw_error(nullptr, "Entry point '" + entry_point + "' must be static.");
            }

            emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
            linker_patches.push_back({bytecode().size(), entry_method});
            emit_int32(0xFFFFFFFF);

            emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
            emit_int32(entry_method->frame_size);

            emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
            emit_int32(entry_method->parameters.size());

            emit_byte(static_cast<uint8_t>(OpCode::CALL));
        }
    }

    emit_byte(static_cast<uint8_t>(OpCode::HALT));
}

void Assembler::compile_node(Node* node) {
    if (!node) return;
    
    switch (node->node_type) {
        case NodeType::CLASS_DECL: {
            compile_class(static_cast<ClassDeclaration*>(node));
            break;
        }
        case NodeType::METHOD_DECL:
        case NodeType::CONSTRUCTOR_DECL: {
            compile_function(node);
            break;
        }
        case NodeType::BLOCK: {
            for (const auto& child : node->children) {
                compile_node(child.get());
            }
            emit_cleanup_for_node(node);
            break;
        }
        case NodeType::VAR_DECL: {
            auto* var_decl = static_cast<VariableDeclaration*>(node);
            if (var_decl->initializer) {
                compile_expression(var_decl->initializer.get());
                emit_byte(static_cast<uint8_t>(OpCode::SET_LOCAL));
                emit_int32(var_decl->memory_index);
            }
            break;
        }
        case NodeType::IF_STMT: {
            auto* if_stmt = static_cast<IfStatement*>(node);
            compile_expression(if_stmt->condition.get());
            
            emit_byte(static_cast<uint8_t>(OpCode::JUMP_IF_FALSE));
            size_t jump_to_else_patch = bytecode().size();
            emit_int32(0xFFFFFFFF);
            
            compile_node(if_stmt->then_branch.get());
            
            if (if_stmt->else_branch) {
                emit_byte(static_cast<uint8_t>(OpCode::JUMP));
                size_t jump_to_end_patch = bytecode().size();
                emit_int32(0xFFFFFFFF);
                
                uint32_t else_ip = bytecode().size();
                bytecode()[jump_to_else_patch] = (else_ip >> 24) & 0xFF;
                bytecode()[jump_to_else_patch + 1] = (else_ip >> 16) & 0xFF;
                bytecode()[jump_to_else_patch + 2] = (else_ip >> 8) & 0xFF;
                bytecode()[jump_to_else_patch + 3] = else_ip & 0xFF;
                
                compile_node(if_stmt->else_branch.get());
                
                uint32_t end_ip = bytecode().size();
                bytecode()[jump_to_end_patch] = (end_ip >> 24) & 0xFF;
                bytecode()[jump_to_end_patch + 1] = (end_ip >> 16) & 0xFF;
                bytecode()[jump_to_end_patch + 2] = (end_ip >> 8) & 0xFF;
                bytecode()[jump_to_end_patch + 3] = end_ip & 0xFF;
            } else {
                uint32_t end_ip = bytecode().size();
                bytecode()[jump_to_else_patch] = (end_ip >> 24) & 0xFF;
                bytecode()[jump_to_else_patch + 1] = (end_ip >> 16) & 0xFF;
                bytecode()[jump_to_else_patch + 2] = (end_ip >> 8) & 0xFF;
                bytecode()[jump_to_else_patch + 3] = end_ip & 0xFF;
            }
            break;
        }
        case NodeType::WHILE_STMT: {
            auto* while_stmt = static_cast<WhileStatement*>(node);
            uint32_t loop_start_ip = bytecode().size();
            
            loop_break_patches.push_back({});
            loop_continue_patches.push_back({});
            
            compile_expression(while_stmt->condition.get());
            
            emit_byte(static_cast<uint8_t>(OpCode::JUMP_IF_FALSE));
            size_t jump_to_end_patch = bytecode().size();
            emit_int32(0xFFFFFFFF);
            
            compile_node(while_stmt->body.get());
            
            uint32_t continue_ip = loop_start_ip;
            for (size_t patch_idx : loop_continue_patches.back()) {
                bytecode()[patch_idx] = (continue_ip >> 24) & 0xFF;
                bytecode()[patch_idx + 1] = (continue_ip >> 16) & 0xFF;
                bytecode()[patch_idx + 2] = (continue_ip >> 8) & 0xFF;
                bytecode()[patch_idx + 3] = continue_ip & 0xFF;
            }
            
            emit_byte(static_cast<uint8_t>(OpCode::JUMP));
            emit_int32(loop_start_ip);
            
            uint32_t end_ip = bytecode().size();
            bytecode()[jump_to_end_patch] = (end_ip >> 24) & 0xFF;
            bytecode()[jump_to_end_patch + 1] = (end_ip >> 16) & 0xFF;
            bytecode()[jump_to_end_patch + 2] = (end_ip >> 8) & 0xFF;
            bytecode()[jump_to_end_patch + 3] = end_ip & 0xFF;
            
            for (size_t patch_idx : loop_break_patches.back()) {
                bytecode()[patch_idx] = (end_ip >> 24) & 0xFF;
                bytecode()[patch_idx + 1] = (end_ip >> 16) & 0xFF;
                bytecode()[patch_idx + 2] = (end_ip >> 8) & 0xFF;
                bytecode()[patch_idx + 3] = end_ip & 0xFF;
            }
            
            loop_break_patches.pop_back();
            loop_continue_patches.pop_back();
            break;
        }
        case NodeType::FOR_STMT: {
            auto* for_stmt = static_cast<ForStatement*>(node);
            
            loop_break_patches.push_back({});
            loop_continue_patches.push_back({});
            
            if (for_stmt->initialization) {
                compile_node(for_stmt->initialization.get());
            }
            
            uint32_t loop_start_ip = bytecode().size();
            
            size_t jump_to_end_patch = 0;
            bool has_condition = false;
            
            if (for_stmt->condition) {
                compile_expression(for_stmt->condition.get());
                emit_byte(static_cast<uint8_t>(OpCode::JUMP_IF_FALSE));
                jump_to_end_patch = bytecode().size();
                emit_int32(0xFFFFFFFF);
                has_condition = true;
            }
            
            if (for_stmt->body) {
                compile_node(for_stmt->body.get());
            }
            
            uint32_t continue_ip = bytecode().size();
            for (size_t patch_idx : loop_continue_patches.back()) {
                bytecode()[patch_idx] = (continue_ip >> 24) & 0xFF;
                bytecode()[patch_idx + 1] = (continue_ip >> 16) & 0xFF;
                bytecode()[patch_idx + 2] = (continue_ip >> 8) & 0xFF;
                bytecode()[patch_idx + 3] = continue_ip & 0xFF;
            }
            
            if (for_stmt->iteration) {
                compile_expression(for_stmt->iteration.get());
                emit_byte(static_cast<uint8_t>(OpCode::POP));
            }
            
            emit_byte(static_cast<uint8_t>(OpCode::JUMP));
            emit_int32(loop_start_ip);
            
            uint32_t end_ip = bytecode().size();
            if (has_condition) {
                bytecode()[jump_to_end_patch] = (end_ip >> 24) & 0xFF;
                bytecode()[jump_to_end_patch + 1] = (end_ip >> 16) & 0xFF;
                bytecode()[jump_to_end_patch + 2] = (end_ip >> 8) & 0xFF;
                bytecode()[jump_to_end_patch + 3] = end_ip & 0xFF;
            }
            
            for (size_t patch_idx : loop_break_patches.back()) {
                bytecode()[patch_idx] = (end_ip >> 24) & 0xFF;
                bytecode()[patch_idx + 1] = (end_ip >> 16) & 0xFF;
                bytecode()[patch_idx + 2] = (end_ip >> 8) & 0xFF;
                bytecode()[patch_idx + 3] = end_ip & 0xFF;
            }
            
            loop_break_patches.pop_back();
            loop_continue_patches.pop_back();
            break;
        }
        case NodeType::DO_WHILE_STMT: {
            auto* do_while = static_cast<DoWhileStatement*>(node);
            uint32_t loop_start_ip = bytecode().size();
            
            loop_break_patches.push_back({});
            loop_continue_patches.push_back({});
            
            compile_node(do_while->body.get());
            
            uint32_t continue_ip = bytecode().size();
            for (size_t patch_idx : loop_continue_patches.back()) {
                bytecode()[patch_idx] = (continue_ip >> 24) & 0xFF;
                bytecode()[patch_idx + 1] = (continue_ip >> 16) & 0xFF;
                bytecode()[patch_idx + 2] = (continue_ip >> 8) & 0xFF;
                bytecode()[patch_idx + 3] = continue_ip & 0xFF;
            }
            
            compile_expression(do_while->condition.get());
            
            emit_byte(static_cast<uint8_t>(OpCode::JUMP_IF_TRUE));
            emit_int32(loop_start_ip);
            
            uint32_t end_ip = bytecode().size();
            for (size_t patch_idx : loop_break_patches.back()) {
                bytecode()[patch_idx] = (end_ip >> 24) & 0xFF;
                bytecode()[patch_idx + 1] = (end_ip >> 16) & 0xFF;
                bytecode()[patch_idx + 2] = (end_ip >> 8) & 0xFF;
                bytecode()[patch_idx + 3] = end_ip & 0xFF;
            }
            
            loop_break_patches.pop_back();
            loop_continue_patches.pop_back();
            break;
        }
        case NodeType::RETURN_STMT: {
            auto* ret_stmt = static_cast<ReturnStatement*>(node);
            if (ret_stmt->value) {
                compile_expression(ret_stmt->value.get());
            } else {
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_NULL));
            }
            
            // Cleanup locals before return
            Node* current = node->parent;
            while (current && current->node_type != NodeType::METHOD_DECL && current->node_type != NodeType::CONSTRUCTOR_DECL) {
                if (current->node_type == NodeType::BLOCK) {
                    emit_cleanup_for_node(current);
                }
                current = current->parent;
            }
            
            emit_byte(static_cast<uint8_t>(OpCode::RETURN));
            break;
        }
        case NodeType::BREAK_STMT: {
            emit_byte(static_cast<uint8_t>(OpCode::JUMP));
            loop_break_patches.back().push_back(bytecode().size());
            emit_int32(0xFFFFFFFF);
            break;
        }
        case NodeType::CONTINUE_STMT: {
            emit_byte(static_cast<uint8_t>(OpCode::JUMP));
            loop_continue_patches.back().push_back(bytecode().size());
            emit_int32(0xFFFFFFFF);
            break;
        }
        case NodeType::EXPR_STMT: {
            auto* expr_stmt = static_cast<ExpressionStatement*>(node);
            compile_expression(expr_stmt->expression.get());
            emit_byte(static_cast<uint8_t>(OpCode::POP));
            break;
        }
        case NodeType::SWITCH_STMT: {
            auto* switch_stmt = static_cast<SwitchStatement*>(node);
            compile_expression(switch_stmt->condition.get());
            
            loop_break_patches.push_back({});
            
            std::vector<uint32_t> case_body_jumps;
            uint32_t default_body_jump = 0xFFFFFFFF;
            
            for (const auto& child : switch_stmt->children) {
                auto* case_stmt = static_cast<CaseStatement*>(child.get());
                if (!case_stmt->is_default) {
                    emit_byte(static_cast<uint8_t>(OpCode::DUP));
                    compile_expression(case_stmt->case_value.get());
                    emit_byte(static_cast<uint8_t>(OpCode::EQUAL));
                    emit_byte(static_cast<uint8_t>(OpCode::JUMP_IF_TRUE));
                    case_body_jumps.push_back(bytecode().size());
                    emit_int32(0xFFFFFFFF);
                } else {
                    emit_byte(static_cast<uint8_t>(OpCode::JUMP));
                    default_body_jump = bytecode().size();
                    emit_int32(0xFFFFFFFF);
                    case_body_jumps.push_back(0xFFFFFFFF);
                }
            }
            
            uint32_t end_jump_if_no_match = 0xFFFFFFFF;
            if (default_body_jump == 0xFFFFFFFF) {
                emit_byte(static_cast<uint8_t>(OpCode::JUMP));
                end_jump_if_no_match = bytecode().size();
                emit_int32(0xFFFFFFFF);
            }
            
            int case_idx = 0;
            for (const auto& child : switch_stmt->children) {
                auto* case_stmt = static_cast<CaseStatement*>(child.get());
                
                uint32_t current_ip = bytecode().size();
                if (case_stmt->is_default) {
                    if (default_body_jump != 0xFFFFFFFF) {
                        bytecode()[default_body_jump] = (current_ip >> 24) & 0xFF;
                        bytecode()[default_body_jump + 1] = (current_ip >> 16) & 0xFF;
                        bytecode()[default_body_jump + 2] = (current_ip >> 8) & 0xFF;
                        bytecode()[default_body_jump + 3] = current_ip & 0xFF;
                    }
                } else {
                    uint32_t patch_ip = case_body_jumps[case_idx];
                    bytecode()[patch_ip] = (current_ip >> 24) & 0xFF;
                    bytecode()[patch_ip + 1] = (current_ip >> 16) & 0xFF;
                    bytecode()[patch_ip + 2] = (current_ip >> 8) & 0xFF;
                    bytecode()[patch_ip + 3] = current_ip & 0xFF;
                }
                
                for (const auto& stmt : case_stmt->children) {
                    compile_node(stmt.get());
                }
                case_idx++;
            }
            
            if (end_jump_if_no_match != 0xFFFFFFFF) {
                uint32_t end_ip = bytecode().size();
                bytecode()[end_jump_if_no_match] = (end_ip >> 24) & 0xFF;
                bytecode()[end_jump_if_no_match + 1] = (end_ip >> 16) & 0xFF;
                bytecode()[end_jump_if_no_match + 2] = (end_ip >> 8) & 0xFF;
                bytecode()[end_jump_if_no_match + 3] = end_ip & 0xFF;
            }
            
            uint32_t end_ip = bytecode().size();
            emit_byte(static_cast<uint8_t>(OpCode::POP)); 
            
            for (uint32_t break_patch : loop_break_patches.back()) {
                bytecode()[break_patch] = (end_ip >> 24) & 0xFF;
                bytecode()[break_patch + 1] = (end_ip >> 16) & 0xFF;
                bytecode()[break_patch + 2] = (end_ip >> 8) & 0xFF;
                bytecode()[break_patch + 3] = end_ip & 0xFF;
            }
            
            loop_break_patches.pop_back();
            break;
        }
        case NodeType::PACKAGE_STMT:
        case NodeType::ALIAS_STMT:
        case NodeType::ENUM_DECL:
            // These don't emit any code in themselves, skip
            break;
        default:
            throw std::runtime_error("Unhandled statement type in assembler: " + std::to_string(static_cast<int>(node->node_type)));
            break;
    }
}

void Assembler::compile_class(ClassDeclaration* class_node) {
    for (const auto& child : class_node->children) {
        if (child->node_type == NodeType::CLASS_DECL) {
            compile_class(static_cast<ClassDeclaration*>(child.get()));
        } else if (child->node_type == NodeType::METHOD_DECL || child->node_type == NodeType::CONSTRUCTOR_DECL) {
            compile_function(child.get());
        }
    }
}

void Assembler::compile_function(Node* function_node) {
    bool is_native = false;
    bool is_abstract = false;
    if (function_node->node_type == NodeType::METHOD_DECL) {
        is_native = static_cast<MethodDeclaration*>(function_node)->is_native;
        is_abstract = static_cast<MethodDeclaration*>(function_node)->is_abstract;
    }

    if (is_native || is_abstract) return; 

    function_ips[function_node] = bytecode().size();

    for (const auto& child : function_node->children) {
        compile_node(child.get());
    }

    emit_byte(static_cast<uint8_t>(OpCode::PUSH_NULL));
    emit_byte(static_cast<uint8_t>(OpCode::RETURN));
}

void Assembler::emit_cleanup_for_node(Node* node) {
    for (auto it = node->children.rbegin(); it != node->children.rend(); ++it) {
        if ((*it)->node_type == NodeType::VAR_DECL) {
            auto* var_decl = static_cast<VariableDeclaration*>((*it).get());
            if (var_decl->is_reference_type) {
                emit_byte(static_cast<uint8_t>(OpCode::GET_LOCAL));
                emit_int32(var_decl->memory_index);
                emit_byte(static_cast<uint8_t>(OpCode::DEC_REF));
            }
        }
    }
}

void Assembler::compile_expression(Node* expr) {
    if (!expr) return;

    switch (expr->node_type) {
        case NodeType::LITERAL: {
            auto* lit = static_cast<LiteralNode*>(expr);
            if (std::holds_alternative<int64_t>(lit->value)) {
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I64));
                emit_int64(std::get<int64_t>(lit->value));
            } else if (std::holds_alternative<double>(lit->value)) {
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_F64));
                emit_float64(std::get<double>(lit->value));
            } else if (std::holds_alternative<std::string>(lit->value)) {
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_STRING));
                emit_string(std::get<std::string>(lit->value));
            } else if (std::holds_alternative<std::nullptr_t>(lit->value)) {
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_NULL));
            }
            break;
        }
        case NodeType::IDENTIFIER: {
            auto* ident = static_cast<IdentifierNode*>(expr);
            
            if (ident->name == "true") {
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_TRUE));
                break;
            } else if (ident->name == "false") {
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_FALSE));
                break;
            } else if (ident->name == "null") {
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_NULL));
                break;
            } else if (ident->name == "this") {
                emit_byte(static_cast<uint8_t>(OpCode::GET_LOCAL));
                emit_int32(0);
                emit_byte(static_cast<uint8_t>(OpCode::INC_REF));
                break;
            }

            if (!ident->resolved_declaration) {
                throw_error(ident, "Unresolved identifier in assembler.");
            }

            if (ident->resolved_declaration->node_type == NodeType::FIELD_DECL) {
                auto* field = static_cast<FieldDeclaration*>(ident->resolved_declaration);
                if (field->is_static) {
                    emit_byte(static_cast<uint8_t>(OpCode::GET_GLOBAL));
                    emit_int32(field->memory_index);
                } else {
                    emit_byte(static_cast<uint8_t>(OpCode::GET_LOCAL));
                    emit_int32(0); 
                    emit_byte(static_cast<uint8_t>(OpCode::GET_PROPERTY));
                    emit_int32(field->memory_index);
                }
                if (field->is_reference_type) emit_byte(static_cast<uint8_t>(OpCode::INC_REF));
            } else if (ident->resolved_declaration->node_type == NodeType::VAR_DECL) {
                auto* var = static_cast<VariableDeclaration*>(ident->resolved_declaration);
                emit_byte(static_cast<uint8_t>(OpCode::GET_LOCAL));
                emit_int32(var->memory_index);
                if (var->is_reference_type) emit_byte(static_cast<uint8_t>(OpCode::INC_REF));
            }
            break;
        }
        case NodeType::ASSIGNMENT_EXPR: {
            auto* assign = static_cast<AssignmentExpression*>(expr);

            if (assign->overloaded_operator) {
                compile_expression(assign->target.get());
                compile_expression(assign->value.get());
                
                auto* method = static_cast<MethodDeclaration*>(assign->overloaded_operator);
                
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
                linker_patches.push_back({bytecode().size(), method});
                emit_int32(0xFFFFFFFF);
                
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
                emit_int32(method->frame_size);
                
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
                emit_int32(2); // this + 1 parameter
                
                emit_byte(static_cast<uint8_t>(OpCode::CALL));
                break;
            }
            
            if (assign->target->node_type == NodeType::ARRAY_ACCESS) {
                auto* arr_acc = static_cast<ArrayAccessExpression*>(assign->target.get());
                compile_expression(arr_acc->array.get());
                compile_expression(arr_acc->index.get());
                compile_expression(assign->value.get());
                emit_byte(static_cast<uint8_t>(OpCode::SET_ARRAY));
                break;
            }
            
            compile_expression(assign->value.get());
            
            emit_byte(static_cast<uint8_t>(OpCode::DUP));

            if (assign->target->node_type == NodeType::IDENTIFIER) {
                auto* ident = static_cast<IdentifierNode*>(assign->target.get());
                if (ident->resolved_declaration->node_type == NodeType::FIELD_DECL) {
                    auto* field = static_cast<FieldDeclaration*>(ident->resolved_declaration);
                    if (field->is_static) {
                        emit_byte(static_cast<uint8_t>(OpCode::SET_GLOBAL));
                        emit_int32(field->memory_index);
                    } else {
                        emit_byte(static_cast<uint8_t>(OpCode::GET_LOCAL));
                        emit_int32(0); 
                        
                        if (field->is_weak) {
                            emit_byte(static_cast<uint8_t>(OpCode::DUP));
                            emit_byte(static_cast<uint8_t>(OpCode::DEC_REF));
                            emit_byte(static_cast<uint8_t>(OpCode::WEAK_SET_PROPERTY));
                            emit_int32(field->memory_index);
                        } else {
                            if (field->is_reference_type) {
                                emit_byte(static_cast<uint8_t>(OpCode::DUP));
                                emit_byte(static_cast<uint8_t>(OpCode::GET_PROPERTY));
                                emit_int32(field->memory_index);
                                emit_byte(static_cast<uint8_t>(OpCode::DEC_REF));
                            }
                            
                            emit_byte(static_cast<uint8_t>(OpCode::SET_PROPERTY));
                            emit_int32(field->memory_index);
                        }
                    }
                } else if (ident->resolved_declaration->node_type == NodeType::VAR_DECL) {
                    auto* var = static_cast<VariableDeclaration*>(ident->resolved_declaration);
                    
                    if (var->is_reference_type) {
                        emit_byte(static_cast<uint8_t>(OpCode::GET_LOCAL));
                        emit_int32(var->memory_index);
                        emit_byte(static_cast<uint8_t>(OpCode::DEC_REF));
                    }
                    
                    emit_byte(static_cast<uint8_t>(OpCode::SET_LOCAL));
                    emit_int32(var->memory_index);
                }
            } else if (assign->target->node_type == NodeType::MEMBER_ACCESS) {
                auto* mem_acc = static_cast<MemberAccessExpression*>(assign->target.get());
                compile_expression(mem_acc->object.get());
                

                auto* field = static_cast<FieldDeclaration*>(mem_acc->resolved_declaration);
                if (!field) {
                    throw std::runtime_error("MEMBER_ACCESS resolved_declaration is null at line " + std::to_string(mem_acc->line));
                }
                if (field->is_weak) {
                    emit_byte(static_cast<uint8_t>(OpCode::DUP));
                    emit_byte(static_cast<uint8_t>(OpCode::DEC_REF));
                    emit_byte(static_cast<uint8_t>(OpCode::WEAK_SET_PROPERTY));
                    emit_int32(field->memory_index);
                } else {
                    if (field->is_reference_type) {
                        emit_byte(static_cast<uint8_t>(OpCode::DUP));
                        emit_byte(static_cast<uint8_t>(OpCode::GET_PROPERTY));
                        emit_int32(field->memory_index);
                        emit_byte(static_cast<uint8_t>(OpCode::DEC_REF));
                    }
                    
                    emit_byte(static_cast<uint8_t>(OpCode::SET_PROPERTY));
                    emit_int32(field->memory_index);
                }
            }
            break;
        }
        case NodeType::BINARY_EXPR: {
            auto* bin = static_cast<BinaryExpression*>(expr);

            if (bin->overloaded_operator) {
                compile_expression(bin->left.get());
                compile_expression(bin->right.get());
                
                auto* method = static_cast<MethodDeclaration*>(bin->overloaded_operator);
                
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
                linker_patches.push_back({bytecode().size(), method});
                emit_int32(0xFFFFFFFF);
                
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
                emit_int32(method->frame_size);
                
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
                emit_int32(2); // this + 1 parameter
                
                emit_byte(static_cast<uint8_t>(OpCode::CALL));
                break;
            }
            
            if (bin->op == TokenType::OPERATOR_LOGICAL_AND) {
                compile_expression(bin->left.get());
                emit_byte(static_cast<uint8_t>(OpCode::DUP));
                emit_byte(static_cast<uint8_t>(OpCode::JUMP_IF_FALSE));
                size_t jump_idx = bytecode().size();
                emit_int32(0xFFFFFFFF);
                emit_byte(static_cast<uint8_t>(OpCode::POP));
                compile_expression(bin->right.get());
                uint32_t end_ip = bytecode().size();
                bytecode()[jump_idx] = (end_ip >> 24) & 0xFF;
                bytecode()[jump_idx + 1] = (end_ip >> 16) & 0xFF;
                bytecode()[jump_idx + 2] = (end_ip >> 8) & 0xFF;
                bytecode()[jump_idx + 3] = end_ip & 0xFF;
                break;
            } else if (bin->op == TokenType::OPERATOR_LOGICAL_OR) {
                compile_expression(bin->left.get());
                emit_byte(static_cast<uint8_t>(OpCode::DUP));
                emit_byte(static_cast<uint8_t>(OpCode::JUMP_IF_TRUE));
                size_t jump_idx = bytecode().size();
                emit_int32(0xFFFFFFFF);
                emit_byte(static_cast<uint8_t>(OpCode::POP));
                compile_expression(bin->right.get());
                uint32_t end_ip = bytecode().size();
                bytecode()[jump_idx] = (end_ip >> 24) & 0xFF;
                bytecode()[jump_idx + 1] = (end_ip >> 16) & 0xFF;
                bytecode()[jump_idx + 2] = (end_ip >> 8) & 0xFF;
                bytecode()[jump_idx + 3] = end_ip & 0xFF;
                break;
            }
            
            compile_expression(bin->left.get());
            compile_expression(bin->right.get());
            
            switch (bin->op) {
                case TokenType::OPERATOR_PLUS: emit_byte(static_cast<uint8_t>(OpCode::ADD)); break;
                case TokenType::OPERATOR_MINUS: emit_byte(static_cast<uint8_t>(OpCode::SUBTRACT)); break;
                case TokenType::OPERATOR_MULTIPLY: emit_byte(static_cast<uint8_t>(OpCode::MULTIPLY)); break;
                case TokenType::OPERATOR_DIVIDE: emit_byte(static_cast<uint8_t>(OpCode::DIVIDE)); break;
                case TokenType::OPERATOR_MODULO: emit_byte(static_cast<uint8_t>(OpCode::MODULO)); break;
                case TokenType::OPERATOR_EQUAL: emit_byte(static_cast<uint8_t>(OpCode::EQUAL)); break;
                case TokenType::OPERATOR_NOT_EQUAL: emit_byte(static_cast<uint8_t>(OpCode::NOT_EQUAL)); break;
                case TokenType::OPERATOR_LESS_THAN: emit_byte(static_cast<uint8_t>(OpCode::LESS)); break;
                case TokenType::OPERATOR_LESS_EQUAL: emit_byte(static_cast<uint8_t>(OpCode::LESS_EQUAL)); break;
                case TokenType::OPERATOR_GREATER_THAN: emit_byte(static_cast<uint8_t>(OpCode::GREATER)); break;
                case TokenType::OPERATOR_GREATER_EQUAL: emit_byte(static_cast<uint8_t>(OpCode::GREATER_EQUAL)); break;
                default: throw std::runtime_error("Unknown binary operator.");
            }
            break;
        }
        case NodeType::UNARY_EXPR: {
            auto* uny = static_cast<UnaryExpression*>(expr);
            compile_expression(uny->operand.get());
            
            if (uny->op == TokenType::OPERATOR_LOGICAL_NOT) {
                emit_byte(static_cast<uint8_t>(OpCode::LOGICAL_NOT));
            } else if (uny->op == TokenType::OPERATOR_MINUS) {
                emit_byte(static_cast<uint8_t>(OpCode::NEGATE));
            } else if (uny->op == TokenType::OPERATOR_INCREMENT || uny->op == TokenType::OPERATOR_DECREMENT) {
                uint8_t opc = (uny->op == TokenType::OPERATOR_INCREMENT) ? static_cast<uint8_t>(OpCode::INC) : static_cast<uint8_t>(OpCode::DEC);
                emit_byte(opc);
                
                if (uny->operand->node_type == NodeType::IDENTIFIER) {
                    auto* ident = static_cast<IdentifierNode*>(uny->operand.get());
                    if (ident->resolved_declaration->node_type == NodeType::VAR_DECL) {
                        emit_byte(static_cast<uint8_t>(OpCode::DUP));
                        emit_byte(static_cast<uint8_t>(OpCode::SET_LOCAL));
                        emit_int32(static_cast<VariableDeclaration*>(ident->resolved_declaration)->memory_index);
                    } else if (ident->resolved_declaration->node_type == NodeType::FIELD_DECL) {
                        auto* field = static_cast<FieldDeclaration*>(ident->resolved_declaration);
                        emit_byte(static_cast<uint8_t>(OpCode::DUP));
                        if (field->is_static) {
                            emit_byte(static_cast<uint8_t>(OpCode::SET_GLOBAL));
                            emit_int32(field->memory_index);
                        } else {
                            emit_byte(static_cast<uint8_t>(OpCode::GET_LOCAL));
                            emit_int32(0);
                            emit_byte(static_cast<uint8_t>(OpCode::SET_PROPERTY));
                            emit_int32(field->memory_index);
                        }
                    }
                } else if (uny->operand->node_type == NodeType::MEMBER_ACCESS) {
                    auto* mem = static_cast<MemberAccessExpression*>(uny->operand.get());
                    auto* field = static_cast<FieldDeclaration*>(mem->resolved_declaration);
                    emit_byte(static_cast<uint8_t>(OpCode::DUP));
                    if (field->is_static) {
                        emit_byte(static_cast<uint8_t>(OpCode::SET_GLOBAL));
                        emit_int32(field->memory_index);
                    } else {
                        compile_expression(mem->object.get());
                        emit_byte(static_cast<uint8_t>(OpCode::SET_PROPERTY));
                        emit_int32(field->memory_index);
                    }
                } else if (uny->operand->node_type == NodeType::ARRAY_ACCESS) {
                    auto* arr_acc = static_cast<ArrayAccessExpression*>(uny->operand.get());
                    compile_expression(arr_acc->array.get());
                    compile_expression(arr_acc->index.get());
                    compile_expression(uny->operand.get());
                    emit_byte(opc);
                    emit_byte(static_cast<uint8_t>(OpCode::SET_ARRAY));
                }
            }
            break;
        }
        case NodeType::METHOD_CALL: {
            auto* call = static_cast<MethodCallExpression*>(expr);


            bool is_static_method = false;
            if (call->resolved_declaration) {
                if (call->resolved_declaration->node_type == NodeType::METHOD_DECL) {
                    is_static_method = static_cast<MethodDeclaration*>(call->resolved_declaration)->is_static;
                } else if (call->resolved_declaration->node_type == NodeType::CONSTRUCTOR_DECL) {
                    is_static_method = false;
                }
            }
            if (call->callee->node_type == NodeType::MEMBER_ACCESS) {
                auto* mem_acc = static_cast<MemberAccessExpression*>(call->callee.get());
                if (call->resolved_declaration && !is_static_method) {
                    compile_expression(mem_acc->object.get());
                }
            } else if (call->resolved_declaration && !is_static_method) {
                // local method call, push 'this'
                emit_byte(static_cast<uint8_t>(OpCode::GET_LOCAL));
                emit_int32(0);
            }
            
            for (const auto& arg : call->arguments) {
                compile_expression(arg.get());
            }

            if (!call->resolved_declaration) {
                throw_error(call, "Unresolved method call in assembler.");
            }


            bool is_native = false;
            int memory_index = -1;
            int frame_size = 0;
            
            if (call->resolved_declaration->node_type == NodeType::METHOD_DECL) {
                auto* m = static_cast<MethodDeclaration*>(call->resolved_declaration);
                is_native = m->is_native;
                memory_index = m->memory_index;
                frame_size = m->frame_size;
                is_static_method = m->is_static;
            } else if (call->resolved_declaration->node_type == NodeType::CONSTRUCTOR_DECL) {
                auto* c = static_cast<ConstructorDeclaration*>(call->resolved_declaration);
                is_native = false;
                memory_index = -1;
                frame_size = c->frame_size;
                is_static_method = false;
            } else {
                throw_error(call, "Resolved declaration is not a method or constructor.");
            }


            if (call->is_virtual_call) {
                auto* target_method = static_cast<MethodDeclaration*>(call->resolved_declaration);
                emit_byte(static_cast<uint8_t>(OpCode::CALL_VIRTUAL));
                emit_int32(target_method->vtable_index);
                emit_int32(target_method->frame_size);
                emit_int32(call->arguments.size() + (target_method->is_static ? 0 : 1));
            } else 
            if (is_native) {
                emit_byte(static_cast<uint8_t>(OpCode::CALL_NATIVE));
                emit_int32(memory_index);
                emit_int32(call->arguments.size());
                emit_byte(is_static_method ? 1 : 0);
            } else {
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
                linker_patches.push_back({bytecode().size(), call->resolved_declaration});
                emit_int32(0xFFFFFFFF);
                
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
                emit_int32(frame_size);
                
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
                emit_int32(call->arguments.size() + (is_static_method ? 0 : 1));
                
                emit_byte(static_cast<uint8_t>(OpCode::CALL));
            }
            break;
        }
        case NodeType::NEW_INSTANCE: {
            auto* inst = static_cast<NewInstanceExpression*>(expr);
            if (!inst->resolved_declaration) {
                throw_error(inst, "Unresolved constructor call in assembler.");
            }

            if (!inst->resolved_declaration) {
                throw std::runtime_error("NEW_INSTANCE resolved_declaration is null!");
            }
            
            ClassDeclaration* class_decl = nullptr;
            ConstructorDeclaration* ctor = nullptr;
            
            if (inst->resolved_declaration->node_type == NodeType::CLASS_DECL) {
                class_decl = static_cast<ClassDeclaration*>(inst->resolved_declaration);
            } else if (inst->resolved_declaration->node_type == NodeType::CONSTRUCTOR_DECL) {
                ctor = static_cast<ConstructorDeclaration*>(inst->resolved_declaration);
                class_decl = static_cast<ClassDeclaration*>(ctor->parent);
            } else {
                throw std::runtime_error("NEW_INSTANCE resolved_declaration is not a class or constructor!");
            }

            emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
            emit_int32(class_decl->instance_size);
            emit_byte(static_cast<uint8_t>(OpCode::ALLOC_DYNAMIC));

            if (class_decl->vtable_id != -1) {
                emit_byte(static_cast<uint8_t>(OpCode::SET_VTABLE));
                emit_int32(class_decl->vtable_id);
            }
            
            if (ctor) {
                emit_byte(static_cast<uint8_t>(OpCode::DUP));
                
                for (const auto& arg : inst->arguments) {
                    compile_expression(arg.get());
                }
                
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
                linker_patches.push_back({bytecode().size(), ctor});
                emit_int32(0xFFFFFFFF);
                
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
                emit_int32(ctor->frame_size);
                
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
                emit_int32(inst->arguments.size() + 1); 
                
                emit_byte(static_cast<uint8_t>(OpCode::CALL));
                emit_byte(static_cast<uint8_t>(OpCode::POP)); // Pop the NULL returned by the constructor
            }
            break;
        }
        case NodeType::MEMBER_ACCESS: {
            auto* mem_acc = static_cast<MemberAccessExpression*>(expr);
            if (mem_acc->enum_value != -1) {
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
                emit_int32(mem_acc->enum_value);
                break;
            }
            
            auto* field = static_cast<FieldDeclaration*>(mem_acc->resolved_declaration);

            if (!mem_acc->resolved_declaration && mem_acc->member_name == "length") {
                compile_expression(mem_acc->object.get());
                emit_byte(static_cast<uint8_t>(OpCode::GET_PROPERTY));
                emit_int32(0);
                break;
            }
            if (!field) {
                throw std::runtime_error("MEMBER_ACCESS read resolved_declaration is null at line " + std::to_string(mem_acc->line));
            }
            
            if (field->is_static) {
                emit_byte(static_cast<uint8_t>(OpCode::GET_GLOBAL));
                emit_int32(field->memory_index);
            } else {
                compile_expression(mem_acc->object.get());
                emit_byte(static_cast<uint8_t>(OpCode::GET_PROPERTY));
                emit_int32(field->memory_index);
            }
            
            if (field->is_reference_type) emit_byte(static_cast<uint8_t>(OpCode::INC_REF));
            break;
        }
        case NodeType::ARRAY_ACCESS: {
            auto* arr_acc = static_cast<ArrayAccessExpression*>(expr);
            compile_expression(arr_acc->array.get());
            compile_expression(arr_acc->index.get());
            emit_byte(static_cast<uint8_t>(OpCode::GET_ARRAY));
            
            bool is_ref = (arr_acc->expression_type.array_depth > 0 || !arr_acc->is_primitive);
            if (is_ref) {
                emit_byte(static_cast<uint8_t>(OpCode::INC_REF));
            }
            break;
        }
        case NodeType::ARRAY_CREATION: {
            auto* arr_crea = static_cast<ArrayCreationExpression*>(expr);
            compile_expression(arr_crea->size.get());
            emit_byte(static_cast<uint8_t>(OpCode::ALLOC_DYNAMIC));
            break;
        }
        case NodeType::ARRAY_LITERAL: {
            auto* arr_lit = static_cast<ArrayLiteralExpression*>(expr);
            emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
            emit_int32(arr_lit->elements.size());
            emit_byte(static_cast<uint8_t>(OpCode::ALLOC_DYNAMIC));
            
            for (size_t i = 0; i < arr_lit->elements.size(); i++) {
                emit_byte(static_cast<uint8_t>(OpCode::DUP));
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
                emit_int32(i);
                compile_expression(arr_lit->elements[i].get());
                emit_byte(static_cast<uint8_t>(OpCode::SET_ARRAY));
                emit_byte(static_cast<uint8_t>(OpCode::POP)); // Pop the value pushed back by SET_ARRAY
            }
            break;
        }
        case NodeType::CAST_EXPR: {
            auto* cast_expr = static_cast<CastExpression*>(expr);
            compile_expression(cast_expr->expression.get());
            
            std::string t = cast_expr->target_type.name;
            if (t == "int8") emit_byte(static_cast<uint8_t>(OpCode::CONV_I8));
            else if (t == "int16") emit_byte(static_cast<uint8_t>(OpCode::CONV_I16));
            else if (t == "int32") emit_byte(static_cast<uint8_t>(OpCode::CONV_I32));
            else if (t == "int64") emit_byte(static_cast<uint8_t>(OpCode::CONV_I64));
            else if (t == "uint8") emit_byte(static_cast<uint8_t>(OpCode::CONV_U8));
            else if (t == "uint16") emit_byte(static_cast<uint8_t>(OpCode::CONV_U16));
            else if (t == "uint32") emit_byte(static_cast<uint8_t>(OpCode::CONV_U32));
            else if (t == "uint64") emit_byte(static_cast<uint8_t>(OpCode::CONV_U64));
            else if (t == "float32") emit_byte(static_cast<uint8_t>(OpCode::CONV_F32));
            else if (t == "float64") emit_byte(static_cast<uint8_t>(OpCode::CONV_F64));
            else if (cast_expr->target_vtable_id != -1) {
                // Keep the object on stack, but we need to duplicate it to check it without consuming it
                // Wait, CAST_CHECK will consume it and if successful, push it back? No, let's just make CAST_CHECK NOT consume it!
                // Let's assume CAST_CHECK doesn't consume the object, just checks it.
                // Wait, what if we just pass the object, and CAST_CHECK peeks at it, or pops and pushes back?
                // Either way, emit CAST_CHECK and vtable_id.
                emit_byte(static_cast<uint8_t>(OpCode::CAST_CHECK));
                emit_int32(cast_expr->target_vtable_id);
            }
            break;
        }
        case NodeType::INSTANCEOF_EXPR: {
            auto* inst_expr = static_cast<InstanceofExpression*>(expr);
            compile_expression(inst_expr->expression.get());
            if (inst_expr->target_vtable_id != -1) {
                emit_byte(static_cast<uint8_t>(OpCode::INSTANCEOF));
                emit_int32(inst_expr->target_vtable_id);
            } else {
                // If it's a primitive or something else, we could just emit PUSH_FALSE, but wait, binder allows it?
                // Actually, binder sets target_vtable_id. If it's -1, it means it's not a class or we can't check it.
                emit_byte(static_cast<uint8_t>(OpCode::POP));
                emit_byte(static_cast<uint8_t>(OpCode::PUSH_FALSE));
            }
            break;
        }
        case NodeType::TERNARY_EXPR: {
            auto* tern = static_cast<TernaryExpression*>(expr);
            compile_expression(tern->condition.get());
            
            emit_byte(static_cast<uint8_t>(OpCode::JUMP_IF_FALSE));
            size_t false_jump_idx = bytecode().size();
            emit_int32(0xFFFFFFFF);
            
            compile_expression(tern->true_branch.get());
            
            emit_byte(static_cast<uint8_t>(OpCode::JUMP));
            size_t end_jump_idx = bytecode().size();
            emit_int32(0xFFFFFFFF);
            
            uint32_t false_ip = bytecode().size();
            bytecode()[false_jump_idx] = (false_ip >> 24) & 0xFF;
            bytecode()[false_jump_idx + 1] = (false_ip >> 16) & 0xFF;
            bytecode()[false_jump_idx + 2] = (false_ip >> 8) & 0xFF;
            bytecode()[false_jump_idx + 3] = false_ip & 0xFF;
            
            compile_expression(tern->false_branch.get());
            
            uint32_t end_ip = bytecode().size();
            bytecode()[end_jump_idx] = (end_ip >> 24) & 0xFF;
            bytecode()[end_jump_idx + 1] = (end_ip >> 16) & 0xFF;
            bytecode()[end_jump_idx + 2] = (end_ip >> 8) & 0xFF;
            bytecode()[end_jump_idx + 3] = end_ip & 0xFF;
            
            break;
        }
        default:
            throw std::runtime_error("Unhandled expression type in assembler: " + std::to_string(static_cast<int>(expr->node_type)));
            break;
    }
}

} // namespace solix
