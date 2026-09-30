# Exception Handling & Stack Unwinding

## 1. Overview

Solix provides structured exception handling through `try-catch-finally` blocks and `throw` statements. The exception system guarantees that active references are decremented and `finally` blocks are executed during stack unwinding, even when exceptions propagate across multiple nested call frames.

---

## 2. VM Exception State

The virtual machine maintains explicit exception dispatch state in `RuntimeContext`:

```cpp
struct RuntimeContext {
    Address active_exception = 0; // Heap address of active thrown object
    std::unordered_map<Address, Address> return_to_cleanup; // Trampoline map
    ...
};
```

---

## 3. Exception Opcodes & Flow

### 3.1. `THROW_EXCEPTION` (Opcode 84)
- Pops the thrown exception object address from the operand stack and writes it to `context.active_exception`.
- Initiates unwinding of the local call stack or jumps to the nearest registered catch block.

### 3.2. `REGISTER_RETURN_CLEANUP` (Opcode 82) & Trampolines
- When entering a guarded block with cleanup requirements (`finally` block or local ARC destruction), the compiler emits `REGISTER_RETURN_CLEANUP` to record the jump target of the cleanup block.
- Upon an early return or exception throw, `JMP_TO_OUTER_CLEANUP` (Opcode 83) executes the registered cleanup block before re-raising or continuing the exit.

### 3.3. Catch Matching via VTable Hierarchy
- In a `catch (ExpectedType ex)` block:
  - The runtime uses `context.active_exception` to check type compatibility using the same VTable hierarchy traversal as `instanceof`.
  - If the active exception matches `ExpectedType` or inherits from it:
    - The VM executes `GET_EXCEPTION` (Opcode 85) to push the exception reference into the catch parameter's local variable slot.
    - Executes `CLEAR_EXCEPTION` (Opcode 86) to clear `active_exception = 0`.
    - Enters the catch body.
  - If no catch block in the current function matches, the current call frame is unwound (running LIFO local variable destructors), and the search continues in the caller's frame.

### 3.4. Guaranteed `finally` Execution
- `finally` blocks are compiled as inline trampolines.
- Whether a `try` block terminates normally, throws an exception, or hits a `return` statement, control flow transfers through the `finally` block before proceeding to the final destination.
