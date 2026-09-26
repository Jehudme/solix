# AssignmentExpression

## 1. Overview & Purpose

An `AssignmentExpression` evaluates a right-hand side (RHS) expression and writes the value to an lvalue (variable, field, or array subscript). It supports simple assignment (`=`) and compound operators (`+=`, `-=`, `*=`, `/=`).

When assigning a reference type in Solix:
- The incoming object's reference counter is incremented (`INC_REF`).
- The displaced previous object's counter is decremented (`DEC_REF`). If it reaches 0, the previous object is freed immediately.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
user.name = new_name;
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 2                 // Load new_name
INC_REF                     // Claim ownership of incoming reference
GET_LOCAL 1                 // Load user instance
SET_PROPERTY 0              // Stores pointer (and decrements old value internally)
```
