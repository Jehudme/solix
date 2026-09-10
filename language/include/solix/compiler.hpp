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
    // Constants
    PUSH_CONST,     // Pushes a value from the constant pool onto the stack
    PUSH_TRUE,      // Pushes boolean true
    PUSH_FALSE,     // Pushes boolean false
    PUSH_NULL,      // Pushes a null reference
    
    // Arithmetic
    ADD, SUBTRACT, MULTIPLY, DIVIDE, MODULO,
    
    // Comparison / Logic
    EQUAL, NOT_EQUAL, GREATER, GREATER_EQUAL, LESS, LESS_EQUAL,
    LOGICAL_NOT, NEGATE,
    
    // Variables
    GET_LOCAL, SET_LOCAL,     // Stack variables
    GET_GLOBAL, SET_GLOBAL,   // Global/Static variables
    
    // Control Flow
    JUMP,           // Unconditional jump
    JUMP_IF_FALSE,  // Jump if the top of the stack is false
    
    // Function / Methods
    CALL,           // Calls a Solix function by integer index
    CALL_NATIVE,    // Calls a C++ FFI native function by integer index
    RETURN,         // Returns from a function
    
    // Objects & Arrays
    NEW_INSTANCE,   // Creates a new class instance on the heap
    NEW_ARRAY,      // Creates a new array on the heap
    GET_PROPERTY,   // Gets a member field of a class
    SET_PROPERTY,   // Sets a member field of a class
    GET_ARRAY,      // Gets a value from an array index
    SET_ARRAY,      // Sets a value at an array index
    
    // Stack manipulation
    POP,            // Discards the top value on the stack
    
    // System
    HALT            // Ends execution
};

// ==========================================
// Constant Pool Data Types
// ==========================================
using ConstantValue = std::variant<int64_t, double, std::string, bool>;

// ==========================================
// Bytecode Chunk
// ==========================================
// A Chunk represents a single compiled method/function
struct Chunk {
    std::string name;                     // Method name for debugging
    std::vector<uint8_t> code;            // The raw bytecode instructions
    std::vector<ConstantValue> constants; // The constant pool for this chunk
    std::vector<int> lines;               // Line numbers for debugging/stack traces
    
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
// The final output of the Compiler.
struct BytecodeProgram {
    std::vector<Chunk> functions;                 // Fast O(1) jump array for methods
    std::unordered_map<std::string, int> exports; // Only used once at startup to find "main"
};

// ==========================================
// Compiler
// ==========================================
class Compiler {
public:
    Compiler();
    
    // Compiles the AST into an executable Bytecode Program
    BytecodeProgram compile(const parser::AstTree& tree);

private:
    BytecodeProgram program;
    Chunk* current_chunk;
    
    // AST Visitors
    void compileNode(parser::Node* node);
    void compileExpression(parser::Node* expr);
    void compileStatement(parser::Node* stmt);
};

} // namespace solix::compiler
