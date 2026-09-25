# UnaryExpression

## 1. Overview & Purpose

A `UnaryExpression` operates on a single operand (`!`, `-`, `+`, `~`, prefix/postfix `++`, `--`). Prefix operations mutate and yield the new value; postfix operations yield the original value before mutating.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
int32 b = a++;
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 1                 // Push original 'a' (to yield as result)
DUP
INC_I64
SET_LOCAL 1                 // 'a' incremented in storage
SET_LOCAL 2                 // 'b' receives original value
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Postfix vs Prefix
```solix
int32 x = 5;
int32 post = x++; // post = 5, x = 6
int32 pre = ++x;  // pre = 7, x = 7
```
*Expected Result*: `post` is 5, `pre` is 7.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Increment on Constant
```solix
++10; // Error
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Invalid operand for increment operator: expected lvalue
```
