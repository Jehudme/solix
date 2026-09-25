# ArrayCreationExpression

## 1. Overview & Purpose

An `ArrayCreationExpression` (`new Type[size]`) allocates a contiguous array buffer on the heap. Arrays are reference types governed by ARC.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
int32[] arr = new int32[10];
```

```bytecode
// Compiled VM Bytecode
PUSH_CONST_I32 10
ALLOC_STATIC 4              // Allocates array buffer (element size 4)
SET_LOCAL 1                 // Stored into arr
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Zero-Initialized Array
```solix
int32[] data = new int32[5];
int32 first = data[0]; // 0
```
*Expected Result*: `first` is 0.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Negative Size at Runtime (Runtime Fault)
```solix
int32[] bad = new int32[-1];
```
*Expected Runtime Exception*:
```text
[FATAL VM PANIC] NegativeArraySizeException: Attempted to create array with negative size -1
```
