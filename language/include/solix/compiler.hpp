#pragma once

#include "solix/parser.hpp"
#include <string_view>
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
    LOGICAL_NOT, NEGATE, INC, DEC,
    
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
    INC_REF, DEC_REF,
    
    // Type Conversions
    CONV_I8, CONV_I16, CONV_I32, CONV_I64,
    CONV_U8, CONV_U16, CONV_U32, CONV_U64,
    CONV_F32, CONV_F64,
    
    // Functions
    CALL, CALL_NATIVE, DEFINE_NATIVE, RETURN,
    
    // End
    HALT
};

// ==========================================
// The Core Compiler
// ==========================================
class Compiler {
public:
  // TODO: Include the source code in the ast tree for compilation.
  void include(std::string_view source_code, std::optional<std::filesystem::path> path = std::nullopt);
  // TODO: Extract the source code from the file then include it in the ast tree for compilation.
  void include(std::filesystem::path path);

  // TODO: Update this function, we should run the semantic analysis before compiling the AST to bytecode.
  std::vector<uint8_t> compile(std::string_view entry_point);
  std::string disassemble(const std::vector<uint8_t>& bytecode) const; // Write the assembly representation of the bytecode for debugging purposes

private:
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
    void compileBootSequence(std::string_view entry_point);
    void compileClass(parser::ClassDeclaration* class_node);
    void compileFunction(parser::Node* function_node);

    void compileNode(parser::Node* node);
    void compileExpression(parser::Node* expr);
    void emitCleanupForNode(parser::Node* node);

    // Current bytecode being generated
    std::vector<uint8_t> bytecode;
    
    // Current AST being compiled
    parser::AstTree ast_tree;
    std::vector<std::vector<uint32_t>> loop_break_patches;
    std::vector<std::vector<uint32_t>> loop_continue_patches;
};

} // namespace compiler
} // namespace solix
