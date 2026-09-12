#pragma once

#include "solix/parser.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <variant>
#include <cstdint>

namespace solix {
namespace compiler {

// ==========================================
// Virtual Machine Instruction Set
// ==========================================
enum class OpCode : uint8_t {
    // Stack & Constants
    PUSH_CONST, PUSH_TRUE, PUSH_FALSE, PUSH_NULL, POP, DUP,
    
    // Arithmetic & Logic
    ADD, SUBTRACT, MULTIPLY, DIVIDE, MODULO,
    EQUAL, NOT_EQUAL, GREATER, GREATER_EQUAL, LESS, LESS_EQUAL,
    LOGICAL_NOT, NEGATE,
    
    // Variables
    GET_LOCAL, SET_LOCAL,
    GET_GLOBAL, SET_GLOBAL,
    
    // Control Flow
    JUMP, JUMP_IF_FALSE, JUMP_IF_TRUE,
    
    // Heap Memory & Arrays
    ALLOC_STATIC, ALLOC_DYNAMIC,
    GET_PROPERTY, SET_PROPERTY,
    GET_ARRAY, SET_ARRAY,
    
    // GC (ARC)
    ADD_REF, REMOVE_REF,
    
    // Type Conversion
    CONVERT_F64,
    
    // Functions
    CALL, CALL_NATIVE, RETURN,
    
    // End
    HALT
};

// ==========================================
// Constant Pool Data Types
// ==========================================
using ConstantValue = std::variant<int64_t, double, std::string, bool>;

// ==========================================
// The Flattened Executable Program
// ==========================================
struct BytecodeProgram {
    std::vector<uint8_t> flat_bytecode;
    std::vector<ConstantValue> constants;
};

// ==========================================
// The Core Compiler
// ==========================================
class Compiler {
public:
    BytecodeProgram compile(parser::AstTree& ast);

private:
    BytecodeProgram program;
    
    // Track where functions start in the flat bytecode array
    std::unordered_map<parser::Node*, uint32_t> function_ips;
    
    // Linker Phase patches: Map from <Byte_Index_Of_0xFFFFFFFF_Hole> to <Function_Node>
    std::vector<std::pair<size_t, parser::Node*>> linker_patches;

    // Helper to add a constant and return its index
    uint32_t emitConstant(const ConstantValue& value);
    
    // Helper to emit a raw byte
    void emitByte(uint8_t byte);
    
    // Helper to emit an integer (like 32-bit offset)
    void emitInt(uint32_t value);
    
    // Linker
    void applyLinkerPatches();
    
    // Compilation passes
    void compileNode(parser::Node* node);
    void compileExpression(parser::Node* expr);
};

} // namespace compiler
} // namespace solix
