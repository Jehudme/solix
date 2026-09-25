# BlockStatement

## 1. Overview & Purpose

A `BlockStatement` groups zero or more statements inside a pair of curly braces `{ ... }`. It is the fundamental building block of program structure and variable lifetime in Solix.

In Solix, a block does three critical jobs:
1. **Creates a Lexical Scope**: Any variable declared inside the block exists only until the closing brace `}`. Outer scopes cannot see it.
2. **Enables Variable Shadowing**: An inner block can declare a variable with the same name as an outer variable, temporarily masking the outer one until the inner block ends.
3. **Guarantees ARC Cleanup & Exception Safety**: Solix uses Automatic Reference Counting (ARC). When a block ends—whether by falling through normally, hitting an early `return`/`break`, or throwing an exception—the compiler automatically emits `DEC_REF` cleanup for every local reference object declared in that block in reverse declaration order.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### How It Compiles
When the compiler encounters `{ ... }`, it pushes a new `SymbolTable` scope. At the end of the block, it emits:
1. **Normal Cleanup**: Emits `GET_LOCAL <slot>` + `DEC_REF` in reverse order for all reference variables.
2. **Skip Jump**: An unconditional `JUMP` past the exception cleanup segment.
3. **Exception Cleanup Segment**: An unwinding landing pad that inner throws jump to, which also decrements the same variables and then branches to the enclosing cleanup or outer frame (`JMP_TO_OUTER_CLEANUP`).

### Bytecode Disassembly Example
```solix
// Solix Code
{
    int32 x = 42;
    String msg = new String("hello");
}
```

```bytecode
// Compiled VM Bytecode
PUSH_CONST_I32 42
SET_LOCAL 1                 // Slot 1: x = 42

PUSH_CONST_STRING 0         // "hello"
CALL String.new(String)     // Slot 2: msg (ref_count = 1)
SET_LOCAL 2

// --- Normal Scope Exit ---
GET_LOCAL 2                 // Load msg
DEC_REF                     // Decrement msg (ref_count 1 -> 0, freed!)
JUMP <skip_exception_ip>

// --- Exception Cleanup Segment (<cleanup_ip>) ---
GET_LOCAL 2                 // If exception occurred, clean up msg here too
DEC_REF
JUMP <outer_cleanup_ip>     // Unwind to parent block

// --- Normal Continuation (<skip_exception_ip>) ---
```

---

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [BlockStatement in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#blockstatement).
