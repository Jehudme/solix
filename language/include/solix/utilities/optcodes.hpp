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

  // Arithmetic & Logic
  ADD,
  SUBTRACT,
  MULTIPLY,
  DIVIDE,
  MODULO,
  EQUAL,
  NOT_EQUAL,
  GREATER,
  GREATER_EQUAL,
  LESS,
  LESS_EQUAL,
  LOGICAL_NOT,
  NEGATE,
  INC,
  DEC,

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
  RETURN,

  // End
  HALT
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
        case OpCode::ADD: return "ADD";
        case OpCode::SUBTRACT: return "SUBTRACT";
        case OpCode::MULTIPLY: return "MULTIPLY";
        case OpCode::DIVIDE: return "DIVIDE";
        case OpCode::MODULO: return "MODULO";
        case OpCode::EQUAL: return "EQUAL";
        case OpCode::NOT_EQUAL: return "NOT_EQUAL";
        case OpCode::GREATER: return "GREATER";
        case OpCode::GREATER_EQUAL: return "GREATER_EQUAL";
        case OpCode::LESS: return "LESS";
        case OpCode::LESS_EQUAL: return "LESS_EQUAL";
        case OpCode::LOGICAL_NOT: return "LOGICAL_NOT";
        case OpCode::NEGATE: return "NEGATE";
        case OpCode::INC: return "INC";
        case OpCode::DEC: return "DEC";
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
        case OpCode::RETURN: return "RETURN";
        case OpCode::HALT: return "HALT";
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
    if (str == "ADD") return OpCode::ADD;
    if (str == "SUBTRACT") return OpCode::SUBTRACT;
    if (str == "MULTIPLY") return OpCode::MULTIPLY;
    if (str == "DIVIDE") return OpCode::DIVIDE;
    if (str == "MODULO") return OpCode::MODULO;
    if (str == "EQUAL") return OpCode::EQUAL;
    if (str == "NOT_EQUAL") return OpCode::NOT_EQUAL;
    if (str == "GREATER") return OpCode::GREATER;
    if (str == "GREATER_EQUAL") return OpCode::GREATER_EQUAL;
    if (str == "LESS") return OpCode::LESS;
    if (str == "LESS_EQUAL") return OpCode::LESS_EQUAL;
    if (str == "LOGICAL_NOT") return OpCode::LOGICAL_NOT;
    if (str == "NEGATE") return OpCode::NEGATE;
    if (str == "INC") return OpCode::INC;
    if (str == "DEC") return OpCode::DEC;
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
    if (str == "RETURN") return OpCode::RETURN;
    if (str == "HALT") return OpCode::HALT;
    return OpCode::HALT; // fallback
}

} // namespace solix