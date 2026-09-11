#pragma once

#include "solix/parser.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include <variant>
#include <unordered_map>

namespace solix::compiler {

// ==========================================
// Instruction Set Architecture (Bytecode)
// ==========================================
enum class OpCode : uint8_t {
    PUSH_CONST, PUSH_TRUE, PUSH_FALSE, PUSH_NULL,
    ADD, SUBTRACT, MULTIPLY, DIVIDE, MODULO,
    EQUAL, NOT_EQUAL, GREATER, GREATER_EQUAL, LESS, LESS_EQUAL,
    LOGICAL_NOT, NEGATE,
    GET_LOCAL, SET_LOCAL,
    GET_GLOBAL, SET_GLOBAL,
    JUMP, JUMP_IF_FALSE,
    CALL, CALL_NATIVE, RETURN,
    NEW_INSTANCE, NEW_ARRAY,
    GET_PROPERTY, SET_PROPERTY,
    GET_ARRAY, SET_ARRAY,
    POP, HALT
};

// ==========================================
// Constant Pool Data Types
// ==========================================
using ConstantValue = std::variant<int64_t, double, std::string, bool>;

// ==========================================
// Bytecode Chunk
// ==========================================
struct Chunk {
    std::string name;                     // Method name for debugging
    int max_local_slots = 0;              // Size of the Call Frame for this function
    std::vector<uint8_t> code;            // The raw bytecode instructions
    std::vector<ConstantValue> constants; // The constant pool for this chunk
    std::vector<int> lines;               // Line numbers for debugging
    
    int addConstant(ConstantValue value) {
        constants.push_back(value);
        return constants.size() - 1;
    }
    
    void writeByte(uint8_t byte, int line) {
        code.push_back(byte);
        lines.push_back(line);
    }
    
    void writeOp(OpCode op, int line) {
        writeByte(static_cast<uint8_t>(op), line);
    }
    
    void writeInt(int32_t value, int line) {
        writeByte((value >> 24) & 0xFF, line);
        writeByte((value >> 16) & 0xFF, line);
        writeByte((value >> 8) & 0xFF, line);
        writeByte(value & 0xFF, line);
    }
};

// ==========================================
// Compiled Program
// ==========================================
struct BytecodeProgram {
    std::vector<Chunk> functions;                 // Fast O(1) jump array for methods
    std::unordered_map<std::string, int> exports; // Only used once at startup to find "main"
    int global_variable_count = 0;                // How many slots to reserve at Memory Pool indices 1 to N
};

// ==========================================
// Compiler
// ==========================================
class Compiler {
public:
    Compiler();
    BytecodeProgram compile(const parser::AstTree& tree);

private:
    BytecodeProgram program;
    Chunk* current_chunk;
    
    // Global Tracker
    std::unordered_map<std::string, int> global_variables;
    
    // Local Scope Tracker for the current function
    struct Local {
        std::string name;
        int depth;
    };
    std::vector<Local> locals;
    int scope_depth;
    
    void beginScope();
    void endScope();
    int addLocal(const std::string& name);
    int resolveLocal(const std::string& name);
    
    int registerGlobal(const std::string& name);
    int resolveGlobal(const std::string& name);

    // AST Visitors
    void compileNode(parser::Node* node);
    void compileExpression(parser::Node* expr);
    void compileStatement(parser::Node* stmt);
};

} // namespace solix::compiler
