# EnumDeclaration

## 1. Overview & Purpose

An `EnumDeclaration` defines a discrete set of named constant values backed by 64-bit integer values (`int64`). Enums are value types: they incur **zero ARC overhead** and require no heap allocations.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### How It Compiles
Members evaluate to constant integer push instructions:
```solix
// Solix Code
enum Status { PENDING, ACTIVE, DONE }
Status s = Status.ACTIVE;
```

```bytecode
// Compiled VM Bytecode
PUSH_CONST_I64 1            // Status.ACTIVE is ordinal 1
SET_LOCAL 1                 // Stored into 's' (value type, 0 ARC)
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Enum Equality and Switch
```solix
enum Color { RED, GREEN, BLUE }

void test(Color c) {
    if (c == Color.RED) {
        Console.println("Red");
    }
}
```
*Expected Result*: Compiles and compares correctly via `EQ_I64`.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Duplicate Enum Member
```solix
enum State { READY, READY }
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Duplicate enum member 'READY' in enum 'State'
```
