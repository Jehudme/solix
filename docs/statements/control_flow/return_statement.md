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

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Early Return from Nested Scopes
```solix
int32 find(bool fast) {
    String a = new String("a");
    {
        String b = new String("b");
        if (fast) return 1;
    }
    return 0;
}
```
*Expected Result*: Returns 1; both `b` and `a` are deallocated.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Missing Return Value in Non-Void Method
```solix
int32 get_val() {
    return; // Error: must return a value
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Must return a value from non-void method
```

### Case 4.2: Return Type Mismatch
```solix
int32 get_num() {
    return "text"; // Error: String cannot convert to int32
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Return type mismatch: expected 'int32', got 'String'
```
