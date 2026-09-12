import sys

with open("language/src/compiler.cpp", "r") as f:
    code = f.read()

for_old = """    else if (node->node_type == parser::NodeType::FOR_STATEMENT) {
        auto for_stmt = static_cast<parser::ForStatement*>(node);
        if (for_stmt->initialization) {
            compileNode(for_stmt->initialization.get());
        }
        
        uint32_t start_ip = bytecode.size();
        
        uint32_t patch_ip = 0;
        bool has_condition = for_stmt->condition != nullptr;
        if (has_condition) {
            compileExpression(for_stmt->condition.get());
            emitByte(static_cast<uint8_t>(OpCode::JUMP_IF_FALSE));
            patch_ip = bytecode.size();
            emitInt32(0xFFFFFFFF);
        }
        
        compileNode(for_stmt->body.get());
        
        if (for_stmt->iteration) {
            compileExpression(for_stmt->iteration.get());
            emitByte(static_cast<uint8_t>(OpCode::POP));
        }
        
        emitByte(static_cast<uint8_t>(OpCode::JUMP));
        emitInt32(start_ip);
        
        if (has_condition) {
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
        bool has_condition = for_stmt->condition != nullptr;
        if (has_condition) {
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
            emitByte(static_cast<uint8_t>(OpCode::POP));
        }
        
        emitByte(static_cast<uint8_t>(OpCode::JUMP));
        emitInt32(start_ip);
        
        uint32_t end_ip = bytecode.size();
        if (has_condition) {
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
if for_old in code:
    code = code.replace(for_old, for_new)
    with open("language/src/compiler.cpp", "w") as f:
        f.write(code)
    print("Patched FOR loop")
else:
    print("Failed to find FOR loop")
