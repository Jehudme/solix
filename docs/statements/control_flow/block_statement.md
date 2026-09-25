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

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Empty and Nested Empty Blocks
Empty blocks compile to zero runtime instructions and produce zero stack delta.
```solix
void test() {
    {}
    {
        {}
        { {} }
    }
}
```
*Expected Result*: Compiles and runs cleanly; stack remains balanced.

### Case 3.2: Lexical Variable Shadowing
An inner block can shadow an outer variable with a different type.
```solix
int32 value = 10;
{
    String value = new String("shadow");
    Console.println(value); // Prints: shadow
}
Console.println(value);     // Prints: 10
```
*Expected Result*: Outputs `"shadow"` then `10`. The inner `String` is freed at the inner closing brace; outer `int32` is unaffected.

### Case 3.3: Strict LIFO Destruction of Multiple Reference Objects
Multiple objects in a block are decremented in reverse declaration order.
```solix
{
    String first = new String("first");
    String second = new String("second");
    String third = new String("third");
}
```
*Expected Result*: Bytecode executes `DEC_REF third`, then `DEC_REF second`, then `DEC_REF first`.

### Case 3.4: Early Return from Nested Blocks
Returning from inside deep blocks unwinds all intermediate scopes.
```solix
int32 compute(bool early) {
    String a = new String("a");
    {
        String b = new String("b");
        if (early) {
            return 100; // Must clean up b and a before RETURN!
        }
    }
    return 0;
}
```
*Expected Result*: Calling `compute(true)` frees `b` and `a` with 0 memory leaks.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Accessing Block-Scoped Variable Outside Its Block
```solix
void test() {
    {
        int32 temp = 100;
    }
    int32 leak = temp; // Error: temp does not exist
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Undefined identifier: temp
```

### Case 4.2: Duplicate Variable in Same Immediate Scope
```solix
void test() {
    {
        int32 score = 10;
        float64 score = 20.0; // Error: duplicate declaration
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Variable 'score' is already defined in the current scope
```

### Case 4.3: Unclosed Block (Missing Brace)
```solix
void test() {
    {
        int32 x = 1;
// Missing '}'
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Syntax error: expected '}' before end of file
```

### Case 4.4: Stray Extra Closing Brace
```solix
void test() {
    { int32 x = 1; }
    } // Extra brace
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Syntax error: unexpected token '}'
```
