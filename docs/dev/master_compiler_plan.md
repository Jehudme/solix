# Solix Master Compiler Refactor Plan

This document outlines the final, exhaustive set of modifications required for the Frontend (`parser.hpp`, `parser.cpp`, and `semantic.cpp`) to perfectly support the flattened, IP-based, ARC-managed Bytecode Virtual Machine.

---

## 1. Abstract Syntax Tree Enhancements (`parser.hpp`)
To guarantee the compiler is a pure, lightning-fast translator, the AST nodes must be enriched to store all memory and routing decisions calculated by the Semantic Analyzer.

1. **`Node::memory_index`**: (Already added). Stores the Call Frame slot, or the Global Memory Pool index, or the Class Field offset.
2. **`MethodDeclaration::frame_size`**: (Already added). The total number of local variables + arguments the method requires.
3. **`MemberAccessExpression::enum_value`**: Add an integer to store the raw value of an enum member (e.g. `ApplicationState.INITIALIZING` = 0) so the Compiler can just emit `PUSH_CONST 0`.
4. **`ClassDeclaration::instance_size`**: Add an integer counting the number of non-static fields. The Compiler needs this for `ALLOC_DYNAMIC <size>`.
5. **`NewInstanceExpression::resolved_constructor`**: Add a `Node*` pointer to the exact `ConstructorDeclaration` being called, so the Compiler can extract its destination IP for the `CALL` instruction.
6. **`VariableDeclaration::is_reference_type`**: Add a boolean. If true, the variable holds a memory pool address (Class, Array, String). This tells the Compiler to emit `REMOVE_REF` when the variable's scope ends.

---

## 2. Global Variable Mapping (Semantic Analyzer: Pass 1.5)
The VM carves out the bottom of the memory pool for static data.
*   **Action**: Create a new pass between Pass 1 and Pass 2.
*   **Logic**: Keep an `int staticVariableIndex = 1;` counter. Loop through the `tree.symbols` map. For every package-level field or `static` class field, assign `field->memory_index = staticVariableIndex++;`.
*   **Result**: The Compiler now knows exactly which `SET_GLOBAL <index>` to emit without doing string lookups.

---

## 3. Local Frame Mapping (Semantic Analyzer: Pass 2)
The VM's Call Frames are pre-allocated fixed-size arrays.
*   **Action**: Inside `resolveAndCheck`, keep an `int localVariableIndex = 0;` tracker.
*   **Logic**: 
    *   When entering a `MethodDeclaration`, reset to `0`.
    *   For every parameter and `VariableDeclaration` inside the method, assign `node->memory_index = localVariableIndex++;`.
    *   When exiting the method, save `method->frame_size = localVariableIndex;`.
*   **Result**: `x = y + z` trivially compiles to `GET_LOCAL 2; GET_LOCAL 3; ADD; SET_LOCAL 1`.

---

## 4. Deep Type Corrections & Edge Cases (Crucial Findings)

After reading through the entire `.cpp` and `.slx` codebase, here are the absolute final tweaks required to make the language completely memory-safe:

1. **String is NOT a Primitive**: In `semantic.cpp`, `type_str == "string"` is currently marked as `is_primitive = true`. As you brillianty pointed out, strings vary in size and must live in the Memory Pool. 
   *   **Fix**: Remove `"string"` from the primitive list. Strings must evaluate to `is_primitive = false` so they trigger `ADD_REF` and `REMOVE_REF` garbage collection!
2. **Array Reference Safety**: Arrays (`array_depth > 0`) must also trigger `is_reference_type = true` so the GC tracks them.
3. **The `null` Keyword**: 
   *   **Fix**: Update the Lexer/Parser to recognize `null` as a literal. The Semantic Analyzer will give it a generic reference type. The Compiler will translate it directly to `PUSH_CONST 0`, which safely points to the restricted Index 0 of the memory pool.
4. **Array Literal Type Safety (`{1, 2, 3}`)**: 
   *   **Fix**: In `semantic.cpp`, `ArrayLiteralExpression` currently only checks the type of the *first* element `elements[0]`. It needs a loop to verify that all elements match, preventing crashes from `int32[] = {1, "Hello"};`.
5. **The `DUP` OpCode**:
   *   **Fix**: For expressions like `numbers[0]++`, the compiler must duplicate the array address on the stack. Add `DUP` to `compiler.hpp`.
6. **The `JUMP_IF_TRUE` OpCode**:
   *   **Fix**: Specifically for `do-while` loops which evaluate at the bottom, jumping back to the top only if the condition is true. Add to `compiler.hpp`.

---

## 5. The Linker Design (`compiler.cpp`)
We throw away the "Vector of Chunks" architecture completely in favor of raw Instruction Pointers (IP).
*   **Action**: `compiler.cpp` outputs a single `std::vector<uint8_t> flat_bytecode;`.
*   **The CALL Sequence**: 
    *   Push Arguments.
    *   `PUSH_CONST 0xFFFFFFFF` (A temporary placeholder for the Destination IP).
    *   `PUSH_CONST <Frame Size>`.
    *   `PUSH_CONST <Arg Count>`.
    *   Emit `CALL`.
*   **The Linking Phase**: At the very end of compilation, the Compiler loops over the 0xFFFFFFFF holes it left and replaces them with the actual byte index of where the target function begins.

