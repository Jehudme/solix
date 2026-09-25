# BreakStatement

## 1. Overview & Purpose

A `BreakStatement` (`break;`) immediately terminates the innermost enclosing loop (`while`, `do-while`, `for`) or `switch` construct.

Before jumping to the construct's exit address, the compiler traverses the AST parent hierarchy and emits inline `DEC_REF` instructions for all active reference variables in intermediate blocks, guaranteeing leak-free escapes.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
while (true) {
    String temp = new String("leak_test");
    break; // Exits loop
}
```

```bytecode
// Compiled VM Bytecode
<loop_head>:
PUSH_CONST_STRING 0
CALL String.new(String)
SET_LOCAL 1                 // Slot 1: temp

// --- Break Execution ---
GET_LOCAL 1                 // Intermediate block cleanup
DEC_REF                     // Clean up temp before jumping!
JUMP <loop_exit>            // Jump out of while loop

<loop_exit>:
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Breaking Out of Deep Nested Blocks
```solix
while (true) {
    String s1 = new String("s1");
    {
        String s2 = new String("s2");
        break; // Cleans up s2 and s1
    }
}
```
*Expected Result*: `s2` and `s1` are freed cleanly; 0 memory leaks.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Break Outside Loop or Switch
```solix
void test() {
    int32 x = 10;
    break; // Error: break not inside loop/switch
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: 'break' statement not allowed outside of loop or switch
```
