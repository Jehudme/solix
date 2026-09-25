# ForStatement

## 1. Overview & Purpose

A `ForStatement` provides 3-clause iteration: an initialization clause (creating loop-scoped variables), a termination condition, and a step update expression.

The initialization variable is encapsulated inside an internal loop scope, meaning it does not leak into the outer function. `continue` jumps directly to the step expression, and ARC cleans up induction variables when the loop finishes.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
for (int32 i = 0; i < 2; i++) {
    Console.println(i);
}
```

```bytecode
// Compiled VM Bytecode
// --- Init (Loop Scope) ---
PUSH_CONST_I32 0
SET_LOCAL 1                 // i = 0

// --- Condition (<loop_start_ip>) ---
GET_LOCAL 1
PUSH_CONST_I32 2
LESS_I64
JUMP_IF_FALSE <loop_exit_ip>

// --- Body ---
GET_LOCAL 1
CALL_NATIVE Console.println

// --- Step Update (<step_ip>) ---
GET_LOCAL 1
INC_I64
SET_LOCAL 1                 // i++
JUMP <loop_start_ip>

// --- Exit (<loop_exit_ip>) ---
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Standard For Loop with Continue
```solix
int32 evens = 0;
for (int32 i = 0; i < 10; i++) {
    if (i % 2 != 0) continue; // Jumps to i++
    evens++;
}
```
*Expected Result*: `evens` equals 5.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Induction Variable Leakage
```solix
void test() {
    for (int32 i = 0; i < 5; i++) {}
    Console.println(i); // Error: i is out of scope
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Undefined identifier: i
```
