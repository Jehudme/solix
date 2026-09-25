# SwitchStatement

## 1. Overview & Purpose

A `SwitchStatement` evaluates an integral or enum selector expression and jumps to matching `case` constant labels, with an optional `default` fallback. Solix supports fall-through semantics unless interrupted by a `break`.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
switch (cmd) {
    case 1:
        run();
        break;
    default:
        stop();
        break;
}
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 1                 // Load cmd onto stack
DUP
PUSH_CONST_I32 1
EQ_I64
JUMP_IF_TRUE <case_1_ip>

// Default Jump
JUMP <default_ip>

// Case 1 Body
<case_1_ip>:
POP                         // Pop duplicated selector
CALL run()
JUMP <switch_exit_ip>

// Default Body
<default_ip>:
POP
CALL stop()
JUMP <switch_exit_ip>

<switch_exit_ip>:
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Switch with Explicit Break
```solix
int32 value = 2;
String name = "";
switch (value) {
    case 1: name = "one"; break;
    case 2: name = "two"; break;
    default: name = "other"; break;
}
```
*Expected Result*: `name` equals `"two"`.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Duplicate Case Constant
```solix
void test(int32 x) {
    switch (x) {
        case 1: break;
        case 1: break; // Error: duplicate
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Duplicate case value '1' in switch statement
```
