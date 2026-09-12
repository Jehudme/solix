import sys

# 1. Update compiler.hpp
with open("language/include/solix/compiler.hpp", "r") as f:
    hpp_code = f.read()
hpp_code = hpp_code.replace("std::vector<std::vector<uint32_t>> loop_break_patches;", "std::vector<std::vector<uint32_t>> loop_break_patches;\n    std::vector<std::vector<uint32_t>> loop_continue_patches;")
with open("language/include/solix/compiler.hpp", "w") as f:
    f.write(hpp_code)


# 2. Update compiler.cpp
with open("language/src/compiler.cpp", "r") as f:
    code = f.read()

helper = """
[[noreturn]] static void throw_compile_error(parser::Node* node, const std::string& msg) {
    std::string err = "[Compiler Error] ";
    if (node) {
        if (!node->file_path.empty()) {
            err += node->file_path.string() + ":";
        }
        err += std::to_string(node->line) + ":" + std::to_string(node->column) + " - ";
    }
    err += msg;
    throw std::runtime_error(err);
}
"""
code = code.replace("namespace compiler {", "namespace compiler {\n" + helper)

code = code.replace('throw std::runtime_error("Linker Error: Unresolved function call!");', 'throw_compile_error(nullptr, "Linker Error: Unresolved function call!");')
code = code.replace('throw std::runtime_error("Entry point not found: " + std::string(entry_point));', 'throw_compile_error(nullptr, "Entry point not found: " + std::string(entry_point));')
code = code.replace('throw std::runtime_error("Entry point must be static");', 'throw_compile_error(main_method, "Entry point must be static");')
code = code.replace('throw std::runtime_error("Unsupported binary operator in compiler");', 'throw_compile_error(expr, "Unsupported binary operator in compiler");')
code = code.replace('throw std::runtime_error("Unsupported cast target type in compiler");', 'throw_compile_error(expr, "Unsupported cast target type in compiler");')


compile_node_end = """    }
    // ... Implement If, While, Do-While, etc. if required
}"""
compile_node_end_new = """    }
    else if (node->node_type == parser::NodeType::SWITCH_STATEMENT) {
        auto switch_stmt = static_cast<parser::SwitchStatement*>(node);
        compileExpression(switch_stmt->condition.get());
        
        loop_break_patches.push_back(std::vector<uint32_t>());
        
        std::vector<uint32_t> next_case_patches;
        for (const auto& child : switch_stmt->children) {
            auto case_stmt = static_cast<parser::CaseStatement*>(child.get());
            
            for (uint32_t patch_ip : next_case_patches) {
                uint32_t current_ip = bytecode.size();
                bytecode[patch_ip] = (current_ip >> 24) & 0xFF;
                bytecode[patch_ip+1] = (current_ip >> 16) & 0xFF;
                bytecode[patch_ip+2] = (current_ip >> 8) & 0xFF;
                bytecode[patch_ip+3] = current_ip & 0xFF;
            }
            next_case_patches.clear();
            
            if (!case_stmt->is_default) {
                emitByte(static_cast<uint8_t>(OpCode::DUP));
                compileExpression(case_stmt->case_value.get());
                emitByte(static_cast<uint8_t>(OpCode::EQUAL));
                emitByte(static_cast<uint8_t>(OpCode::JUMP_IF_FALSE));
                next_case_patches.push_back(bytecode.size());
                emitInt32(0xFFFFFFFF);
            }
            
            for (const auto& stmt : case_stmt->children) {
                compileNode(stmt.get());
            }
        }
        
        for (uint32_t patch_ip : next_case_patches) {
            uint32_t current_ip = bytecode.size();
            bytecode[patch_ip] = (current_ip >> 24) & 0xFF;
            bytecode[patch_ip+1] = (current_ip >> 16) & 0xFF;
            bytecode[patch_ip+2] = (current_ip >> 8) & 0xFF;
            bytecode[patch_ip+3] = current_ip & 0xFF;
        }
        
        emitByte(static_cast<uint8_t>(OpCode::POP)); 
        
        uint32_t end_ip = bytecode.size();
        for (uint32_t break_patch : loop_break_patches.back()) {
            bytecode[break_patch] = (end_ip >> 24) & 0xFF;
            bytecode[break_patch+1] = (end_ip >> 16) & 0xFF;
            bytecode[break_patch+2] = (end_ip >> 8) & 0xFF;
            bytecode[break_patch+3] = end_ip & 0xFF;
        }
        loop_break_patches.pop_back();
    }
    else if (node->node_type == parser::NodeType::BREAK_STATEMENT) {
        if (loop_break_patches.empty()) throw_compile_error(node, "Break statement outside of loop or switch");
        emitByte(static_cast<uint8_t>(OpCode::JUMP));
        loop_break_patches.back().push_back(bytecode.size());
        emitInt32(0xFFFFFFFF);
    }
    else if (node->node_type == parser::NodeType::CONTINUE_STATEMENT) {
        if (loop_continue_patches.empty()) throw_compile_error(node, "Continue statement outside of loop");
        emitByte(static_cast<uint8_t>(OpCode::JUMP));
        loop_continue_patches.back().push_back(bytecode.size());
        emitInt32(0xFFFFFFFF);
    }
    else if (node->node_type == parser::NodeType::EXPRESSION_STATEMENT) {
        auto expr_stmt = static_cast<parser::ExpressionStatement*>(node);
        if (expr_stmt->expression) compileExpression(expr_stmt->expression.get());
    }
    else {
        throw_compile_error(node, "Unhandled AST node type in compiler (compileNode): " + std::to_string(static_cast<int>(node->node_type)));
    }
}"""
if compile_node_end in code: code = code.replace(compile_node_end, compile_node_end_new)


while_old = """    else if (node->node_type == parser::NodeType::WHILE_STATEMENT) {
        auto while_stmt = static_cast<parser::WhileStatement*>(node);
        uint32_t start_ip = bytecode.size();
        compileExpression(while_stmt->condition.get());
        emitByte(static_cast<uint8_t>(OpCode::JUMP_IF_FALSE));
        uint32_t patch_ip = bytecode.size();
        emitInt32(0xFFFFFFFF);
        
        compileNode(while_stmt->body.get());
        
        emitByte(static_cast<uint8_t>(OpCode::JUMP));
        emitInt32(start_ip);
        
        uint32_t end_ip = bytecode.size();
        bytecode[patch_ip] = (end_ip >> 24) & 0xFF;
            bytecode[patch_ip+1] = (end_ip >> 16) & 0xFF;
            bytecode[patch_ip+2] = (end_ip >> 8) & 0xFF;
            bytecode[patch_ip+3] = end_ip & 0xFF;
    }"""
while_new = """    else if (node->node_type == parser::NodeType::WHILE_STATEMENT) {
        auto while_stmt = static_cast<parser::WhileStatement*>(node);
        uint32_t start_ip = bytecode.size();
        compileExpression(while_stmt->condition.get());
        emitByte(static_cast<uint8_t>(OpCode::JUMP_IF_FALSE));
        uint32_t patch_ip = bytecode.size();
        emitInt32(0xFFFFFFFF);
        
        loop_break_patches.push_back(std::vector<uint32_t>());
        loop_continue_patches.push_back(std::vector<uint32_t>());
        
        compileNode(while_stmt->body.get());
        
        uint32_t continue_ip = start_ip;
        for (uint32_t cont_patch : loop_continue_patches.back()) {
            bytecode[cont_patch] = (continue_ip >> 24) & 0xFF;
            bytecode[cont_patch+1] = (continue_ip >> 16) & 0xFF;
            bytecode[cont_patch+2] = (continue_ip >> 8) & 0xFF;
            bytecode[cont_patch+3] = continue_ip & 0xFF;
        }
        loop_continue_patches.pop_back();
        
        emitByte(static_cast<uint8_t>(OpCode::JUMP));
        emitInt32(start_ip);
        
        uint32_t end_ip = bytecode.size();
        bytecode[patch_ip] = (end_ip >> 24) & 0xFF;
        bytecode[patch_ip+1] = (end_ip >> 16) & 0xFF;
        bytecode[patch_ip+2] = (end_ip >> 8) & 0xFF;
        bytecode[patch_ip+3] = end_ip & 0xFF;
        
        for (uint32_t break_patch : loop_break_patches.back()) {
            bytecode[break_patch] = (end_ip >> 24) & 0xFF;
            bytecode[break_patch+1] = (end_ip >> 16) & 0xFF;
            bytecode[break_patch+2] = (end_ip >> 8) & 0xFF;
            bytecode[break_patch+3] = end_ip & 0xFF;
        }
        loop_break_patches.pop_back();
    }"""
if while_old in code: code = code.replace(while_old, while_new)

do_while_old = """    else if (node->node_type == parser::NodeType::DO_WHILE_STATEMENT) {
        auto do_stmt = static_cast<parser::DoWhileStatement*>(node);
        uint32_t start_ip = bytecode.size();
        
        compileNode(do_stmt->body.get());
        
        compileExpression(do_stmt->condition.get());
        
        emitByte(static_cast<uint8_t>(OpCode::JUMP_IF_TRUE));
        emitInt32(start_ip);
    }"""
do_while_new = """    else if (node->node_type == parser::NodeType::DO_WHILE_STATEMENT) {
        auto do_stmt = static_cast<parser::DoWhileStatement*>(node);
        uint32_t start_ip = bytecode.size();
        
        loop_break_patches.push_back(std::vector<uint32_t>());
        loop_continue_patches.push_back(std::vector<uint32_t>());
        
        compileNode(do_stmt->body.get());
        
        uint32_t continue_ip = bytecode.size();
        for (uint32_t cont_patch : loop_continue_patches.back()) {
            bytecode[cont_patch] = (continue_ip >> 24) & 0xFF;
            bytecode[cont_patch+1] = (continue_ip >> 16) & 0xFF;
            bytecode[cont_patch+2] = (continue_ip >> 8) & 0xFF;
            bytecode[cont_patch+3] = continue_ip & 0xFF;
        }
        loop_continue_patches.pop_back();
        
        compileExpression(do_stmt->condition.get());
        
        emitByte(static_cast<uint8_t>(OpCode::JUMP_IF_TRUE));
        emitInt32(start_ip);
        
        uint32_t end_ip = bytecode.size();
        for (uint32_t break_patch : loop_break_patches.back()) {
            bytecode[break_patch] = (end_ip >> 24) & 0xFF;
            bytecode[break_patch+1] = (end_ip >> 16) & 0xFF;
            bytecode[break_patch+2] = (end_ip >> 8) & 0xFF;
            bytecode[break_patch+3] = end_ip & 0xFF;
        }
        loop_break_patches.pop_back();
    }"""
if do_while_old in code: code = code.replace(do_while_old, do_while_new)

for_old = """    else if (node->node_type == parser::NodeType::FOR_STATEMENT) {
        auto for_stmt = static_cast<parser::ForStatement*>(node);
        if (for_stmt->initialization) {
            compileNode(for_stmt->initialization.get());
        }
        
        uint32_t start_ip = bytecode.size();
        uint32_t patch_ip = 0;
        
        if (for_stmt->condition) {
            compileExpression(for_stmt->condition.get());
            emitByte(static_cast<uint8_t>(OpCode::JUMP_IF_FALSE));
            patch_ip = bytecode.size();
            emitInt32(0xFFFFFFFF);
        }
        
        compileNode(for_stmt->body.get());
        
        if (for_stmt->iteration) {
            compileExpression(for_stmt->iteration.get());
        }
        
        emitByte(static_cast<uint8_t>(OpCode::JUMP));
        emitInt32(start_ip);
        
        if (for_stmt->condition) {
            uint32_t end_ip = bytecode.size();
            bytecode[patch_ip] = (end_ip >> 24) & 0xFF;
            bytecode[patch_ip+1] = (end_ip >> 16) & 0xFF;
            bytecode[patch_ip+2] = (end_ip >> 8) & 0xFF;
            bytecode[patch_ip+3] = end_ip & 0xFF;
        }
    }"""
for_new = """    else if (node->node_type == parser::NodeType::FOR_STATEMENT) {
        auto for_stmt = static_cast<parser::ForStatement*>(node);
        if (for_stmt->initialization) {
            compileNode(for_stmt->initialization.get());
        }
        
        uint32_t start_ip = bytecode.size();
        uint32_t patch_ip = 0;
        
        if (for_stmt->condition) {
            compileExpression(for_stmt->condition.get());
            emitByte(static_cast<uint8_t>(OpCode::JUMP_IF_FALSE));
            patch_ip = bytecode.size();
            emitInt32(0xFFFFFFFF);
        }
        
        loop_break_patches.push_back(std::vector<uint32_t>());
        loop_continue_patches.push_back(std::vector<uint32_t>());
        
        compileNode(for_stmt->body.get());
        
        uint32_t continue_ip = bytecode.size();
        for (uint32_t cont_patch : loop_continue_patches.back()) {
            bytecode[cont_patch] = (continue_ip >> 24) & 0xFF;
            bytecode[cont_patch+1] = (continue_ip >> 16) & 0xFF;
            bytecode[cont_patch+2] = (continue_ip >> 8) & 0xFF;
            bytecode[cont_patch+3] = continue_ip & 0xFF;
        }
        loop_continue_patches.pop_back();
        
        if (for_stmt->iteration) {
            compileExpression(for_stmt->iteration.get());
        }
        
        emitByte(static_cast<uint8_t>(OpCode::JUMP));
        emitInt32(start_ip);
        
        uint32_t end_ip = bytecode.size();
        if (for_stmt->condition) {
            bytecode[patch_ip] = (end_ip >> 24) & 0xFF;
            bytecode[patch_ip+1] = (end_ip >> 16) & 0xFF;
            bytecode[patch_ip+2] = (end_ip >> 8) & 0xFF;
            bytecode[patch_ip+3] = end_ip & 0xFF;
        }
        
        for (uint32_t break_patch : loop_break_patches.back()) {
            bytecode[break_patch] = (end_ip >> 24) & 0xFF;
            bytecode[break_patch+1] = (end_ip >> 16) & 0xFF;
            bytecode[break_patch+2] = (end_ip >> 8) & 0xFF;
            bytecode[break_patch+3] = end_ip & 0xFF;
        }
        loop_break_patches.pop_back();
    }"""
if for_old in code: code = code.replace(for_old, for_new)

# Assign Expr
assign_old = """    else if (expr->node_type == parser::NodeType::ASSIGNMENT_EXPRESSION) {
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
    }"""
assign_new = """    else if (expr->node_type == parser::NodeType::ASSIGNMENT_EXPRESSION) {
        auto assign = static_cast<parser::AssignmentExpression*>(expr);
        
        if (assign->target->node_type == parser::NodeType::IDENTIFIER_EXPRESSION) {
            compileExpression(assign->value.get());
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
            } else if (ident->resolved_declaration->node_type == parser::NodeType::FIELD_DECLARATION) {
                auto field = static_cast<parser::FieldDeclaration*>(ident->resolved_declaration);
                if (field->is_static) {
                    emitByte(static_cast<uint8_t>(OpCode::SET_GLOBAL));
                    emitInt32(field->memory_index);
                } else {
                    emitByte(static_cast<uint8_t>(OpCode::GET_LOCAL));
                    emitInt32(0);
                    emitByte(static_cast<uint8_t>(OpCode::SET_PROPERTY));
                    emitInt32(field->memory_index);
                }
            }
        } else if (assign->target->node_type == parser::NodeType::MEMBER_ACCESS_EXPRESSION) {
            auto mem_acc = static_cast<parser::MemberAccessExpression*>(assign->target.get());
            compileExpression(assign->value.get());
            compileExpression(mem_acc->object.get());
            auto field = static_cast<parser::FieldDeclaration*>(mem_acc->resolved_declaration);
            emitByte(static_cast<uint8_t>(OpCode::SET_PROPERTY));
            emitInt32(field->memory_index);
        } else if (assign->target->node_type == parser::NodeType::ARRAY_ACCESS_EXPRESSION) {
            auto arr_acc = static_cast<parser::ArrayAccessExpression*>(assign->target.get());
            compileExpression(assign->value.get());
            compileExpression(arr_acc->array.get());
            compileExpression(arr_acc->index.get());
            emitByte(static_cast<uint8_t>(OpCode::SET_ARRAY));
        } else {
            throw_compile_error(expr, "Invalid assignment target");
        }
    }"""
if assign_old in code: code = code.replace(assign_old, assign_new)

# Expression end
expr_end = """        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
        emitInt32(inst->arguments.size() + 1);
        
        emitByte(static_cast<uint8_t>(OpCode::CALL));
    }
}"""
expr_end_new = """        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
        emitInt32(inst->arguments.size() + 1);
        
        emitByte(static_cast<uint8_t>(OpCode::CALL));
    }
    else if (expr->node_type == parser::NodeType::MEMBER_ACCESS_EXPRESSION) {
        auto mem_acc = static_cast<parser::MemberAccessExpression*>(expr);
        if (mem_acc->enum_value != -1) {
            emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
            emitInt32(mem_acc->enum_value);
        } else {
            compileExpression(mem_acc->object.get());
            auto field = static_cast<parser::FieldDeclaration*>(mem_acc->resolved_declaration);
            emitByte(static_cast<uint8_t>(OpCode::GET_PROPERTY));
            emitInt32(field->memory_index);
        }
    }
    else if (expr->node_type == parser::NodeType::ARRAY_ACCESS_EXPRESSION) {
        auto arr_acc = static_cast<parser::ArrayAccessExpression*>(expr);
        compileExpression(arr_acc->array.get());
        compileExpression(arr_acc->index.get());
        emitByte(static_cast<uint8_t>(OpCode::GET_ARRAY));
    }
    else if (expr->node_type == parser::NodeType::ARRAY_CREATION_EXPRESSION) {
        auto arr_crea = static_cast<parser::ArrayCreationExpression*>(expr);
        compileExpression(arr_crea->size.get());
        emitByte(static_cast<uint8_t>(OpCode::ALLOC_DYNAMIC));
    }
    else if (expr->node_type == parser::NodeType::ARRAY_LITERAL_EXPRESSION) {
        auto arr_lit = static_cast<parser::ArrayLiteralExpression*>(expr);
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
        emitInt32(arr_lit->elements.size());
        emitByte(static_cast<uint8_t>(OpCode::ALLOC_DYNAMIC));
        for (size_t i = 0; i < arr_lit->elements.size(); i++) {
            emitByte(static_cast<uint8_t>(OpCode::DUP)); 
            emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
            emitInt32(i); 
            compileExpression(arr_lit->elements[i].get());
            emitByte(static_cast<uint8_t>(OpCode::SET_ARRAY));
        }
    }
    else if (expr->node_type == parser::NodeType::TERNARY_EXPRESSION) {
        auto tern = static_cast<parser::TernaryExpression*>(expr);
        compileExpression(tern->condition.get());
        emitByte(static_cast<uint8_t>(OpCode::JUMP_IF_FALSE));
        uint32_t patch_ip_false = bytecode.size();
        emitInt32(0xFFFFFFFF);
        
        compileExpression(tern->true_branch.get());
        emitByte(static_cast<uint8_t>(OpCode::JUMP));
        uint32_t patch_ip_end = bytecode.size();
        emitInt32(0xFFFFFFFF);
        
        uint32_t false_ip = bytecode.size();
        bytecode[patch_ip_false] = (false_ip >> 24) & 0xFF;
        bytecode[patch_ip_false+1] = (false_ip >> 16) & 0xFF;
        bytecode[patch_ip_false+2] = (false_ip >> 8) & 0xFF;
        bytecode[patch_ip_false+3] = false_ip & 0xFF;
        
        compileExpression(tern->false_branch.get());
        
        uint32_t end_ip = bytecode.size();
        bytecode[patch_ip_end] = (end_ip >> 24) & 0xFF;
        bytecode[patch_ip_end+1] = (end_ip >> 16) & 0xFF;
        bytecode[patch_ip_end+2] = (end_ip >> 8) & 0xFF;
        bytecode[patch_ip_end+3] = end_ip & 0xFF;
    }
    else {
        throw_compile_error(expr, "Unhandled AST node type in compiler (compileExpression): " + std::to_string(static_cast<int>(expr->node_type)));
    }
}"""
if expr_end in code: code = code.replace(expr_end, expr_end_new)

with open("language/src/compiler.cpp", "w") as f:
    f.write(code)

print("Patched compiler.cpp safely")
