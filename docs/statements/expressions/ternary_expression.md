# TernaryExpression

## 1. Overview & Purpose

A `TernaryExpression` (`cond ? true_expr : false_expr`) is an inline conditional operator. It is strictly short-circuiting: only the branch matching the evaluated condition is executed.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
int32 val = flag ? 10 : 20;
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 1                 // Load flag
JUMP_IF_FALSE <false_branch>
PUSH_CONST_I32 10           // True branch
JUMP <end>
<false_branch>:
PUSH_CONST_I32 20           // False branch
<end>:
SET_LOCAL 2                 // Stored into val
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Safe Guard with Null Check
```solix
String s = null;
int32 len = (s != null) ? s.length() : 0; // Short-circuits; does not call s.length()!
```
*Expected Result*: `len` equals 0; no null pointer exception occurs.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Non-Boolean Condition
```solix
int32 res = 5 ? 1 : 2; // Error
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Ternary condition must be of type 'bool', got 'int32'
```
