#include <iostream>
#include <fstream>
#include <sstream>

int main() {
    std::ifstream file("language/src/compiler.cpp");
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    std::string search = "    else if (node->node_type == parser::NodeType::RETURN_STATEMENT) {";
    size_t pos = source.find(search);
    if (pos == std::string::npos) {
        std::cout << "Not found\n";
        return 1;
    }
    
    std::string insertion = R"(    else if (node->node_type == parser::NodeType::IF_STATEMENT) {
        auto if_stmt = static_cast<parser::IfStatement*>(node);
        compileExpression(if_stmt->condition.get());
        emitByte(static_cast<uint8_t>(OpCode::JUMP_IF_FALSE));
        uint32_t patch_ip = bytecode.size();
        emitInt32(0xFFFFFFFF);
        
        compileNode(if_stmt->if_block.get());
        
        if (if_stmt->else_block) {
            emitByte(static_cast<uint8_t>(OpCode::JUMP));
            uint32_t jump_end_ip = bytecode.size();
            emitInt32(0xFFFFFFFF);
            
            uint32_t else_start = bytecode.size();
            uint32_t* patch_ptr = reinterpret_cast<uint32_t*>(&bytecode[patch_ip]);
            *patch_ptr = else_start;
            
            compileNode(if_stmt->else_block.get());
            
            uint32_t end_ip = bytecode.size();
            uint32_t* end_patch_ptr = reinterpret_cast<uint32_t*>(&bytecode[jump_end_ip]);
            *end_patch_ptr = end_ip;
        } else {
            uint32_t end_ip = bytecode.size();
            uint32_t* patch_ptr = reinterpret_cast<uint32_t*>(&bytecode[patch_ip]);
            *patch_ptr = end_ip;
        }
    }
    else if (node->node_type == parser::NodeType::WHILE_STATEMENT) {
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
        uint32_t* patch_ptr = reinterpret_cast<uint32_t*>(&bytecode[patch_ip]);
        *patch_ptr = end_ip;
    }
    else if (node->node_type == parser::NodeType::DO_WHILE_STATEMENT) {
        auto do_while_stmt = static_cast<parser::DoWhileStatement*>(node);
        uint32_t start_ip = bytecode.size();
        
        compileNode(do_while_stmt->body.get());
        compileExpression(do_while_stmt->condition.get());
        
        emitByte(static_cast<uint8_t>(OpCode::JUMP_IF_FALSE));
        uint32_t patch_ip = bytecode.size();
        emitInt32(0xFFFFFFFF);
        
        emitByte(static_cast<uint8_t>(OpCode::JUMP));
        emitInt32(start_ip);
        
        uint32_t end_ip = bytecode.size();
        uint32_t* patch_ptr = reinterpret_cast<uint32_t*>(&bytecode[patch_ip]);
        *patch_ptr = end_ip;
    }
    else if (node->node_type == parser::NodeType::FOR_STATEMENT) {
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
        
        if (for_stmt->update) {
            compileExpression(for_stmt->update.get());
            emitByte(static_cast<uint8_t>(OpCode::POP));
        }
        
        emitByte(static_cast<uint8_t>(OpCode::JUMP));
        emitInt32(start_ip);
        
        if (has_condition) {
            uint32_t end_ip = bytecode.size();
            uint32_t* patch_ptr = reinterpret_cast<uint32_t*>(&bytecode[patch_ip]);
            *patch_ptr = end_ip;
        }
    }
)";
    source.insert(pos, insertion);
    std::ofstream out("language/src/compiler.cpp");
    out << source;
    std::cout << "Done insertion\n";
    return 0;
}
