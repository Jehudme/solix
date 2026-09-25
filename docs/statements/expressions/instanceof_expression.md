# InstanceOfExpression

## 1. Overview & Purpose

An `InstanceOfExpression` (`obj instanceof TargetType`) queries whether an instance inherits from or implements a type. Returns `true` on match, and `false` otherwise (or if `obj == null`).

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
bool is_dog = a instanceof Dog;
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 1
INSTANCEOF <Dog_vtable_id>  // Pushes bool to stack
SET_LOCAL 2
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Safe Null Evaluation
```solix
Animal a = null;
bool check = a instanceof Dog; // false, no panic!
```
*Expected Result*: `check` is `false`.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Primitive Target
```solix
bool b = 10 instanceof int32; // Error
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: 'instanceof' cannot be applied to primitive types
```
