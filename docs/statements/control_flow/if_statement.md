# IfStatement

## 1. Overview & Scope

An `IfStatement` provides conditional branching based on the evaluation of a boolean predicate expression. If the condition evaluates to `true`, the `then` branch executes. If `false`, control transfers to the optional `else` branch (or bypasses the statement).

In Solix:
- The condition must be strictly of type `bool`. Numeric zero/non-zero truthiness is rejected.
- The compiler lowers the if-statement using conditional jump instructions (`JUMP_IF_FALSE`) with forward jump target patching.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
if (score >= 50) {
    Console.println("Pass");
} else {
    Console.println("Fail");
}
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 1                 // Load score
PUSH_CONST_I32 50
GREATER_EQ_I64              // Pushes bool (true/false)
JUMP_IF_FALSE <else_ip>     // Jump to else branch if false

// --- Then Branch ---
PUSH_CONST_STRING 0         // "Pass"
CALL_NATIVE Console.println
JUMP <end_ip>               // Jump over else branch

// --- Else Branch (<else_ip>) ---
PUSH_CONST_STRING 1         // "Fail"
CALL_NATIVE Console.println

// --- End (<end_ip>) ---
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Single Branch If
```solix
int32 x = 10;
if (x > 5) {
    x = 20;
}
```
*Expected Result*: `x` becomes 20.

### Case 3.2: If-Else Chain
```solix
int32 score = 85;
char grade = 'F';
if (score >= 90) {
    grade = 'A';
} else if (score >= 80) {
    grade = 'B';
} else {
    grade = 'C';
}
```
*Expected Result*: `grade` evaluates to `'B'`.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Non-Boolean Condition Type
```solix
void test() {
    int32 count = 1;
    if (count) { // Error: int32 not allowed as condition
        Console.println("yes");
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: If condition must be of type 'bool', got 'int32'
```
