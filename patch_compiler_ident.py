import sys

with open("language/src/compiler.cpp", "r") as f:
    code = f.read()

ident_old = """                } else {
                    emitByte(static_cast<uint8_t>(OpCode::GET_PROPERTY));
                    emitInt32(field->memory_index);
                }"""
ident_new = """                } else {
                    emitByte(static_cast<uint8_t>(OpCode::GET_LOCAL));
                    emitInt32(0);
                    emitByte(static_cast<uint8_t>(OpCode::GET_PROPERTY));
                    emitInt32(field->memory_index);
                }"""
if ident_old in code: code = code.replace(ident_old, ident_new)

arr_lit_old = """            emitByte(static_cast<uint8_t>(OpCode::SET_ARRAY));
            emitByte(static_cast<uint8_t>(OpCode::POP)); // Set_array doesn't pop array ref? Wait, does it?
            // Wait, VM SET_ARRAY pops value, index, array ref. We need to leave the array ref on stack for the final result.
            // If SET_ARRAY pops all 3, then DUP before index and value is correct:
            // Stack: arr_ref, arr_ref(duped), index, value -> SET_ARRAY pops 3 -> Stack: arr_ref"""
arr_lit_new = """            emitByte(static_cast<uint8_t>(OpCode::SET_ARRAY));"""
if arr_lit_old in code: code = code.replace(arr_lit_old, arr_lit_new)

with open("language/src/compiler.cpp", "w") as f:
    f.write(code)

print("Patched IDENTIFIER_EXPRESSION and ARRAY_LITERAL_EXPRESSION")
