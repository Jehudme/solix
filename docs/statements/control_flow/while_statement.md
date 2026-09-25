# WhileStatement

## 1. Overview & Purpose

A `WhileStatement` implements pre-test repetitive execution. Before every iteration, the condition is evaluated. If `true`, the loop body executes; if `false`, execution exits the loop.

The loop body supports `break` (to exit immediately) and `continue` (to jump back to the condition check). Any reference variables declared inside the loop body are cleaned up via ARC at the end of each iteration.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
int32 i = 0;
while (i < 3) {
    i++;
}
```

```bytecode
// Compiled VM Bytecode
PUSH_CONST_I32 0
SET_LOCAL 1                 // i = 0

// --- Loop Start (<loop_start_ip>) ---
GET_LOCAL 1                 // Load i
PUSH_CONST_I32 3
LESS_I64                    // i < 3
JUMP_IF_FALSE <loop_exit_ip>// Exit loop if condition is false

// --- Loop Body ---
GET_LOCAL 1
INC_I64
SET_LOCAL 1                 // i++
JUMP <loop_start_ip>        // Loop back to condition

// --- Loop Exit (<loop_exit_ip>) ---
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Standard Counted Loop
```solix
int32 total = 0;
int32 i = 1;
while (i <= 5) {
    total += i;
    i++;
}
```
*Expected Result*: `total` equals 15.

### Case 3.2: While Loop with Break and Continue
```solix
int32 sum = 0;
int32 i = 0;
while (true) {
    i++;
    if (i % 2 == 0) continue;
    if (i > 5) break;
    sum += i;
}
```
*Expected Result*: `sum` equals 1 + 3 + 5 = 9.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Non-Boolean Loop Condition
```solix
void test() {
    int32 x = 5;
    while (x) { // Error: x is not bool
        x--;
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: While condition must be of type 'bool', got 'int32'
```
