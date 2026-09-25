# ReturnStatement

## 1. Overview & Purpose

A `ReturnStatement` (`return;` or `return expr;`) exits the current subroutine, unwinds all active lexical scopes, decrements instance receiver `this` and reference parameters, and yields an optional return value to the caller.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
int32 compute() {
    String temp = new String("val");
    return 42;
}
```

```bytecode
// Compiled VM Bytecode
// 1. Evaluate Return Value
PUSH_CONST_I32 42           // Pushed to top of stack

// 2. Unwind Intermediate Blocks
GET_LOCAL 1                 // Load temp
DEC_REF                     // Destroy temp without disturbing 42!

// 3. Subroutine Frame Cleanup
// (Decrements parameters and 'this' if instance method)

// 4. Return to Caller
RETURN                      // Pops frame; 42 remains at stack top for caller
```

---

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [ReturnStatement in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#returnstatement).
