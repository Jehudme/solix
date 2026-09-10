#pragma once

#include "solix/parser.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include <variant>

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
    CALL,           // Calls a Solix function
    CALL_NATIVE,    // Calls a C++ FFI native function
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
// The constant pool stores literals that appear in the source code.
// Strings, big numbers, etc., live here.
using ConstantValue = std::variant<int64_t, double, std::string, bool>;

// ==========================================
// Bytecode Chunk
// ==========================================
// A "Chunk" represents a sequence of bytecode. Usually, every method
// has its own chunk. The main global scope will also have a chunk.
struct Chunk {
    std::vector<uint8_t> code;            // The raw bytecode instructions
    std::vector<ConstantValue> constants; // The constant pool
    std::vector<int> lines;               // Line numbers for debugging/stack traces
    
    // Appends a constant to the pool and returns its index
    int addConstant(ConstantValue value) {
        constants.push_back(value);
        return constants.size() - 1;
    }
    
    // Writes a raw byte to the chunk
    void writeByte(uint8_t byte, int line) {
        code.push_back(byte);
        lines.push_back(line);
    }
    
    // Writes an OpCode to the chunk
    void writeOp(OpCode op, int line) {
        writeByte(static_cast<uint8_t>(op), line);
    }
};

// ==========================================
// Compiler
// ==========================================
// Translates the parsed and semantically-checked AstTree into bytecode Chunks.
class Compiler {
public:
    Compiler();
    
    // Compiles the AST into an executable Bytecode Chunk
    Chunk compile(const parser::AstTree& tree);

private:
    Chunk current_chunk;
    
    // AST Visitors
    void compileNode(parser::Node* node);
    void compileExpression(parser::Node* expr);
    void compileStatement(parser::Node* stmt);
};

} // namespace solix::compiler
