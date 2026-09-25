# ContinueStatement

## 1. Overview & Purpose

A `ContinueStatement` (`continue;`) skips the remainder of the current loop iteration and immediately jumps to the loop's continuation point (the condition check in `while`/`do-while`, or the step expression in `for`).

Intermediate reference variables declared inside the loop body prior to the `continue` statement are cleaned up via `DEC_REF` before the jump occurs.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
while (i < 5) {
    String s = new String("skip");
    i++;
    continue; // Jump to condition
}
```

```bytecode
// Compiled VM Bytecode
// --- Continue Execution ---
GET_LOCAL 2                 // Clean up s
DEC_REF
JUMP <loop_condition_ip>    // Jump to condition check
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Continue Advances Loop Variable
```solix
int32 hits = 0;
for (int32 i = 0; i < 6; i++) {
    if (i % 2 == 0) continue;
    hits++;
}
```
*Expected Result*: `hits` equals 3 (processed for 1, 3, 5).

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Continue Outside Loop
```solix
void test() {
    continue; // Error: not in loop
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: 'continue' statement not allowed outside of loop
```
