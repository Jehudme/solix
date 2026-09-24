#pragma once
#include <stdint.h>
#include <string>

namespace solix {

enum class OpCode : uint8_t {
  // Stack & Constants
  PUSH_CONST_I8,
  PUSH_CONST_I16,
  PUSH_CONST_I32,
  PUSH_CONST_I64,
  PUSH_CONST_U8,
  PUSH_CONST_U16,
  PUSH_CONST_U32,
  PUSH_CONST_U64,
  PUSH_CONST_F32,
  PUSH_CONST_F64,
  PUSH_CONST_STRING,
  PUSH_TRUE,
  PUSH_FALSE,
  PUSH_NULL,
  POP,
  DUP,
  DUP2,

  // Arithmetic & Logic
  ADD_I64,
  ADD_F64,
  SUB_I64,
  SUB_F64,
  MUL_I64,
  MUL_F64,
  DIV_I64,
  DIV_F64,
  MOD_I64,
  EQ_I64,
  EQ_F64,
  NEQ_I64,
  NEQ_F64,
  GREATER_I64,
  GREATER_F64,
  GREATER_EQ_I64,
  GREATER_EQ_F64,
  LESS_I64,
  LESS_F64,
  LESS_EQ_I64,
  LESS_EQ_F64,
  LOGICAL_NOT,
  NEGATE,
  INC_I64,
  INC_F64,
  DEC_I64,
  DEC_F64,

  // Variables
  GET_LOCAL,
  SET_LOCAL,
  GET_GLOBAL,
  SET_GLOBAL,

  // Control Flow
  JUMP,
  JUMP_IF_FALSE,
  JUMP_IF_TRUE,

  // Heap Memory & Arrays
  ALLOC_STATIC,
  ALLOC_DYNAMIC,
  GET_PROPERTY,
  SET_PROPERTY,
  WEAK_SET_PROPERTY,
  GET_ARRAY,
  SET_ARRAY,
  ARRAY_LENGTH,

  // GC (ARC)
  INC_REF,
  DEC_REF,

  // Type Conversions
  CONV_I8,
  CONV_I16,
  CONV_I32,
  CONV_I64,
  CONV_U8,
  CONV_U16,
  CONV_U32,
  CONV_U64,
  CONV_F32,
  CONV_F64,

  // Functions
  CALL,
  CALL_NATIVE,
  DEFINE_NATIVE,
  CALL_VIRTUAL,
  DEFINE_VTABLE,
  SET_VTABLE,
  CAST_CHECK,
  INSTANCEOF,
  RETURN,

  // End
  HALT,
  THROW_ABSTRACT,
  
  // Exception Handling
  REGISTER_RETURN_CLEANUP,
  JMP_TO_OUTER_CLEANUP,
  THROW_EXCEPTION,
  GET_EXCEPTION,
  CLEAR_EXCEPTION
};

inline const char* opcode_to_string(uint8_t op) {
    switch (static_cast<OpCode>(op)) {
        case OpCode::PUSH_CONST_I8: return "PUSH_CONST_I8";
        case OpCode::PUSH_CONST_I16: return "PUSH_CONST_I16";
        case OpCode::PUSH_CONST_I32: return "PUSH_CONST_I32";
        case OpCode::PUSH_CONST_I64: return "PUSH_CONST_I64";
        case OpCode::PUSH_CONST_U8: return "PUSH_CONST_U8";
        case OpCode::PUSH_CONST_U16: return "PUSH_CONST_U16";
        case OpCode::PUSH_CONST_U32: return "PUSH_CONST_U32";
        case OpCode::PUSH_CONST_U64: return "PUSH_CONST_U64";
        case OpCode::PUSH_CONST_F32: return "PUSH_CONST_F32";
        case OpCode::PUSH_CONST_F64: return "PUSH_CONST_F64";
        case OpCode::PUSH_CONST_STRING: return "PUSH_CONST_STRING";
        case OpCode::PUSH_TRUE: return "PUSH_TRUE";
        case OpCode::PUSH_FALSE: return "PUSH_FALSE";
        case OpCode::PUSH_NULL: return "PUSH_NULL";
        case OpCode::POP: return "POP";
        case OpCode::DUP: return "DUP";
        case OpCode::DUP2: return "DUP2";
        case OpCode::ADD_I64: return "ADD_I64";
        case OpCode::ADD_F64: return "ADD_F64";
        case OpCode::SUB_I64: return "SUB_I64";
        case OpCode::SUB_F64: return "SUB_F64";
        case OpCode::MUL_I64: return "MUL_I64";
        case OpCode::MUL_F64: return "MUL_F64";
        case OpCode::DIV_I64: return "DIV_I64";
        case OpCode::DIV_F64: return "DIV_F64";
        case OpCode::MOD_I64: return "MOD_I64";
        case OpCode::EQ_I64: return "EQ_I64";
        case OpCode::EQ_F64: return "EQ_F64";
        case OpCode::NEQ_I64: return "NEQ_I64";
        case OpCode::NEQ_F64: return "NEQ_F64";
        case OpCode::GREATER_I64: return "GREATER_I64";
        case OpCode::GREATER_F64: return "GREATER_F64";
        case OpCode::GREATER_EQ_I64: return "GREATER_EQ_I64";
        case OpCode::GREATER_EQ_F64: return "GREATER_EQ_F64";
        case OpCode::LESS_I64: return "LESS_I64";
        case OpCode::LESS_F64: return "LESS_F64";
        case OpCode::LESS_EQ_I64: return "LESS_EQ_I64";
        case OpCode::LESS_EQ_F64: return "LESS_EQ_F64";
        case OpCode::LOGICAL_NOT: return "LOGICAL_NOT";
        case OpCode::NEGATE: return "NEGATE";
        case OpCode::INC_I64: return "INC_I64";
        case OpCode::INC_F64: return "INC_F64";
        case OpCode::DEC_I64: return "DEC_I64";
        case OpCode::DEC_F64: return "DEC_F64";
        case OpCode::GET_LOCAL: return "GET_LOCAL";
        case OpCode::SET_LOCAL: return "SET_LOCAL";
        case OpCode::GET_GLOBAL: return "GET_GLOBAL";
        case OpCode::SET_GLOBAL: return "SET_GLOBAL";
        case OpCode::JUMP: return "JUMP";
        case OpCode::JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case OpCode::JUMP_IF_TRUE: return "JUMP_IF_TRUE";
        case OpCode::ALLOC_STATIC: return "ALLOC_STATIC";
        case OpCode::ALLOC_DYNAMIC: return "ALLOC_DYNAMIC";
        case OpCode::GET_PROPERTY: return "GET_PROPERTY";
        case OpCode::SET_PROPERTY: return "SET_PROPERTY";
        case OpCode::WEAK_SET_PROPERTY: return "WEAK_SET_PROPERTY";
        case OpCode::GET_ARRAY: return "GET_ARRAY";
        case OpCode::SET_ARRAY: return "SET_ARRAY";
        case OpCode::ARRAY_LENGTH: return "ARRAY_LENGTH";
        case OpCode::INC_REF: return "INC_REF";
        case OpCode::DEC_REF: return "DEC_REF";
        case OpCode::CONV_I8: return "CONV_I8";
        case OpCode::CONV_I16: return "CONV_I16";
        case OpCode::CONV_I32: return "CONV_I32";
        case OpCode::CONV_I64: return "CONV_I64";
        case OpCode::CONV_U8: return "CONV_U8";
        case OpCode::CONV_U16: return "CONV_U16";
        case OpCode::CONV_U32: return "CONV_U32";
        case OpCode::CONV_U64: return "CONV_U64";
        case OpCode::CONV_F32: return "CONV_F32";
        case OpCode::CONV_F64: return "CONV_F64";
        case OpCode::CALL: return "CALL";
        case OpCode::CALL_NATIVE: return "CALL_NATIVE";
        case OpCode::DEFINE_NATIVE: return "DEFINE_NATIVE";
        case OpCode::CALL_VIRTUAL: return "CALL_VIRTUAL";
        case OpCode::DEFINE_VTABLE: return "DEFINE_VTABLE";
        case OpCode::SET_VTABLE: return "SET_VTABLE";
        case OpCode::CAST_CHECK: return "CAST_CHECK";
        case OpCode::INSTANCEOF: return "INSTANCEOF";
        case OpCode::RETURN: return "RETURN";
        case OpCode::HALT: return "HALT";
        case OpCode::THROW_ABSTRACT: return "THROW_ABSTRACT";
        case OpCode::REGISTER_RETURN_CLEANUP: return "REGISTER_RETURN_CLEANUP";
        case OpCode::JMP_TO_OUTER_CLEANUP: return "JMP_TO_OUTER_CLEANUP";
        case OpCode::THROW_EXCEPTION: return "THROW_EXCEPTION";
        case OpCode::GET_EXCEPTION: return "GET_EXCEPTION";
        case OpCode::CLEAR_EXCEPTION: return "CLEAR_EXCEPTION";
        default: return "UNKNOWN";
    }
}

inline OpCode string_to_opcode(const std::string& str) {
    if (str == "PUSH_CONST_I8") return OpCode::PUSH_CONST_I8;
    if (str == "PUSH_CONST_I16") return OpCode::PUSH_CONST_I16;
    if (str == "PUSH_CONST_I32") return OpCode::PUSH_CONST_I32;
    if (str == "PUSH_CONST_I64") return OpCode::PUSH_CONST_I64;
    if (str == "PUSH_CONST_U8") return OpCode::PUSH_CONST_U8;
    if (str == "PUSH_CONST_U16") return OpCode::PUSH_CONST_U16;
    if (str == "PUSH_CONST_U32") return OpCode::PUSH_CONST_U32;
    if (str == "PUSH_CONST_U64") return OpCode::PUSH_CONST_U64;
    if (str == "PUSH_CONST_F32") return OpCode::PUSH_CONST_F32;
    if (str == "PUSH_CONST_F64") return OpCode::PUSH_CONST_F64;
    if (str == "PUSH_CONST_STRING") return OpCode::PUSH_CONST_STRING;
    if (str == "PUSH_TRUE") return OpCode::PUSH_TRUE;
    if (str == "PUSH_FALSE") return OpCode::PUSH_FALSE;
    if (str == "PUSH_NULL") return OpCode::PUSH_NULL;
    if (str == "POP") return OpCode::POP;
    if (str == "DUP") return OpCode::DUP;
    if (str == "DUP2") return OpCode::DUP2;
    if (str == "ADD_I64") return OpCode::ADD_I64;
    if (str == "ADD_F64") return OpCode::ADD_F64;
    if (str == "SUB_I64") return OpCode::SUB_I64;
    if (str == "SUB_F64") return OpCode::SUB_F64;
    if (str == "MUL_I64") return OpCode::MUL_I64;
    if (str == "MUL_F64") return OpCode::MUL_F64;
    if (str == "DIV_I64") return OpCode::DIV_I64;
    if (str == "DIV_F64") return OpCode::DIV_F64;
    if (str == "MOD_I64") return OpCode::MOD_I64;
    if (str == "EQ_I64") return OpCode::EQ_I64;
    if (str == "EQ_F64") return OpCode::EQ_F64;
    if (str == "NEQ_I64") return OpCode::NEQ_I64;
    if (str == "NEQ_F64") return OpCode::NEQ_F64;
    if (str == "GREATER_I64") return OpCode::GREATER_I64;
    if (str == "GREATER_F64") return OpCode::GREATER_F64;
    if (str == "GREATER_EQ_I64") return OpCode::GREATER_EQ_I64;
    if (str == "GREATER_EQ_F64") return OpCode::GREATER_EQ_F64;
    if (str == "LESS_I64") return OpCode::LESS_I64;
    if (str == "LESS_F64") return OpCode::LESS_F64;
    if (str == "LESS_EQ_I64") return OpCode::LESS_EQ_I64;
    if (str == "LESS_EQ_F64") return OpCode::LESS_EQ_F64;
    if (str == "LOGICAL_NOT") return OpCode::LOGICAL_NOT;
    if (str == "NEGATE") return OpCode::NEGATE;
    if (str == "INC_I64") return OpCode::INC_I64;
    if (str == "INC_F64") return OpCode::INC_F64;
    if (str == "DEC_I64") return OpCode::DEC_I64;
    if (str == "DEC_F64") return OpCode::DEC_F64;
    if (str == "GET_LOCAL") return OpCode::GET_LOCAL;
    if (str == "SET_LOCAL") return OpCode::SET_LOCAL;
    if (str == "GET_GLOBAL") return OpCode::GET_GLOBAL;
    if (str == "SET_GLOBAL") return OpCode::SET_GLOBAL;
    if (str == "JUMP") return OpCode::JUMP;
    if (str == "JUMP_IF_FALSE") return OpCode::JUMP_IF_FALSE;
    if (str == "JUMP_IF_TRUE") return OpCode::JUMP_IF_TRUE;
    if (str == "ALLOC_STATIC") return OpCode::ALLOC_STATIC;
    if (str == "ALLOC_DYNAMIC") return OpCode::ALLOC_DYNAMIC;
    if (str == "GET_PROPERTY") return OpCode::GET_PROPERTY;
    if (str == "SET_PROPERTY") return OpCode::SET_PROPERTY;
    if (str == "WEAK_SET_PROPERTY") return OpCode::WEAK_SET_PROPERTY;
    if (str == "GET_ARRAY") return OpCode::GET_ARRAY;
    if (str == "SET_ARRAY") return OpCode::SET_ARRAY;
    if (str == "ARRAY_LENGTH") return OpCode::ARRAY_LENGTH;
    if (str == "INC_REF") return OpCode::INC_REF;
    if (str == "DEC_REF") return OpCode::DEC_REF;
    if (str == "CONV_I8") return OpCode::CONV_I8;
    if (str == "CONV_I16") return OpCode::CONV_I16;
    if (str == "CONV_I32") return OpCode::CONV_I32;
    if (str == "CONV_I64") return OpCode::CONV_I64;
    if (str == "CONV_U8") return OpCode::CONV_U8;
    if (str == "CONV_U16") return OpCode::CONV_U16;
    if (str == "CONV_U32") return OpCode::CONV_U32;
    if (str == "CONV_U64") return OpCode::CONV_U64;
    if (str == "CONV_F32") return OpCode::CONV_F32;
    if (str == "CONV_F64") return OpCode::CONV_F64;
    if (str == "CALL") return OpCode::CALL;
    if (str == "CALL_NATIVE") return OpCode::CALL_NATIVE;
    if (str == "DEFINE_NATIVE") return OpCode::DEFINE_NATIVE;
    if (str == "CALL_VIRTUAL") return OpCode::CALL_VIRTUAL;
    if (str == "DEFINE_VTABLE") return OpCode::DEFINE_VTABLE;
    if (str == "SET_VTABLE") return OpCode::SET_VTABLE;
    if (str == "CAST_CHECK") return OpCode::CAST_CHECK;
    if (str == "INSTANCEOF") return OpCode::INSTANCEOF;
    if (str == "RETURN") return OpCode::RETURN;
    if (str == "HALT") return OpCode::HALT;
    if (str == "THROW_ABSTRACT") return OpCode::THROW_ABSTRACT;
    if (str == "REGISTER_RETURN_CLEANUP") return OpCode::REGISTER_RETURN_CLEANUP;
    if (str == "JMP_TO_OUTER_CLEANUP") return OpCode::JMP_TO_OUTER_CLEANUP;
    if (str == "THROW_EXCEPTION") return OpCode::THROW_EXCEPTION;
    if (str == "GET_EXCEPTION") return OpCode::GET_EXCEPTION;
    if (str == "CLEAR_EXCEPTION") return OpCode::CLEAR_EXCEPTION;
    return OpCode::HALT; // fallback
}

} // namespace solix