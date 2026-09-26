# VariableDeclarationStatement

## 1. Overview & Purpose

A `VariableDeclarationStatement` introduces one or more named variables into the current scope. It binds an identifier to a static type, allocates a register slot in the function's activation frame, and optionally assigns an initial value.

In Solix:
- **Primitives (Value Types)**: Numbers, booleans, and characters (`int32`, `float64`, `bool`, `char`) hold raw bits directly in the stack register and require no reference counting.
- **Reference Types**: Classes, interfaces, strings, and **all arrays** (including `int32[]`) hold heap pointers. Initializing a reference variable increments its reference counter (`INC_REF`), and leaving the variable's scope decrements it (`DEC_REF`).

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### How It Compiles
1. The binder resolves the type and checks if `array_depth > 0 || !is_primitive`. If so, it flags `is_reference_type = true`.
2. The binder assigns `memory_index = local_variable_index++`.
3. If an initializer expression is present:
   - Compiles the initializer expression onto the stack top.
   - If `is_reference_type == true`, emits `INC_REF`.
   - Emits `SET_LOCAL <memory_index>`.

### Bytecode Disassembly Example
```solix
// Solix Code
int32 count = 10;
String name = new String("Solix");
```

```bytecode
// Compiled VM Bytecode
PUSH_CONST_I32 10
SET_LOCAL 1                 // Slot 1: count = 10 (value type, no INC_REF)

PUSH_CONST_STRING 0         // "Solix"
CALL String.new(String)     // Returns heap pointer
INC_REF                     // Claim ownership (ref_count = 2)
SET_LOCAL 2                 // Slot 2: name = pointer
```
