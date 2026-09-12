import sys

with open("language/src/compiler.cpp", "r") as f:
    code = f.read()

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
                    emitByte(static_cast<uint8_t>(OpCode::GET_LOCAL)); // 'this' is at index 0
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

# Find the end of compileExpression
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
        compileExpression(arr_crea->length.get());
        emitByte(static_cast<uint8_t>(OpCode::ALLOC_DYNAMIC));
    }
    else if (expr->node_type == parser::NodeType::ARRAY_LITERAL_EXPRESSION) {
        auto arr_lit = static_cast<parser::ArrayLiteralExpression*>(expr);
        emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
        emitInt32(arr_lit->elements.size());
        emitByte(static_cast<uint8_t>(OpCode::ALLOC_DYNAMIC));
        for (size_t i = 0; i < arr_lit->elements.size(); i++) {
            emitByte(static_cast<uint8_t>(OpCode::DUP)); // Array reference
            emitByte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
            emitInt32(i); // Index
            compileExpression(arr_lit->elements[i].get()); // Value
            emitByte(static_cast<uint8_t>(OpCode::SET_ARRAY));
            emitByte(static_cast<uint8_t>(OpCode::POP)); // Set_array doesn't pop array ref? Wait, does it?
            // Wait, VM SET_ARRAY pops value, index, array ref. We need to leave the array ref on stack for the final result.
            // If SET_ARRAY pops all 3, then DUP before index and value is correct:
            // Stack: arr_ref, arr_ref(duped), index, value -> SET_ARRAY pops 3 -> Stack: arr_ref
        }
    }
    else if (expr->node_type == parser::NodeType::TERNARY_EXPRESSION) {
        auto tern = static_cast<parser::TernaryExpression*>(expr);
        compileExpression(tern->condition.get());
        emitByte(static_cast<uint8_t>(OpCode::JUMP_IF_FALSE));
        uint32_t patch_ip_false = bytecode.size();
        emitInt32(0xFFFFFFFF);
        
        compileExpression(tern->true_expression.get());
        emitByte(static_cast<uint8_t>(OpCode::JUMP));
        uint32_t patch_ip_end = bytecode.size();
        emitInt32(0xFFFFFFFF);
        
        uint32_t false_ip = bytecode.size();
        bytecode[patch_ip_false] = (false_ip >> 24) & 0xFF;
        bytecode[patch_ip_false+1] = (false_ip >> 16) & 0xFF;
        bytecode[patch_ip_false+2] = (false_ip >> 8) & 0xFF;
        bytecode[patch_ip_false+3] = false_ip & 0xFF;
        
        compileExpression(tern->false_expression.get());
        
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

print("Phase 5 - Part 2 applied")
