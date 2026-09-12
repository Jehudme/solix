#pragma once

#include "solix/parser.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <cstring>

namespace solix {
namespace compiler {

// ==========================================
// Virtual Machine Instruction Set
// ==========================================
enum class OpCode : uint8_t {
    // Stack & Constants
    PUSH_CONST_I8, PUSH_CONST_I16, PUSH_CONST_I32, PUSH_CONST_I64,
    PUSH_CONST_U8, PUSH_CONST_U16, PUSH_CONST_U32, PUSH_CONST_U64,
    PUSH_CONST_F32, PUSH_CONST_F64,
    PUSH_CONST_STRING,
    PUSH_TRUE, PUSH_FALSE, PUSH_NULL, POP, DUP,
    
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
    
    // Type Conversions
    CONV_I8, CONV_I16, CONV_I32, CONV_I64,
    CONV_U8, CONV_U16, CONV_U32, CONV_U64,
    CONV_F32, CONV_F64,
    
    // Functions
    CALL, CALL_NATIVE, RETURN,
    
    // End
    HALT
};

// ==========================================
// The Flattened Executable Program
// ==========================================
struct BytecodeProgram {
    std::vector<uint8_t> flat_bytecode;
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

    // Helper to emit a raw byte
    void emitByte(uint8_t byte);
    
    // Helpers to emit raw data values into the flat bytecode
    void emitInt32(uint32_t value);
    void emitInt64(uint64_t value);
    void emitFloat32(float value);
    void emitFloat64(double value);
    void emitString(const std::string& value);
    
    // Linker
    void applyLinkerPatches();
    
    // Compilation passes
    void compileNode(parser::Node* node);
    void compileExpression(parser::Node* expr);
};

} // namespace compiler
} // namespace solix
