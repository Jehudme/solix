# ArrayAccessExpression

## 1. Overview & Purpose

An `ArrayAccessExpression` (`arr[index]`) accesses or modifies an element in an array. The VM enforces runtime bounds checking, raising `IndexOutOfBoundsException` on violation.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
int32 val = arr[2];
arr[2] = 50;
```

```bytecode
// Compiled VM Bytecode
// Read
GET_LOCAL 1                 // arr
PUSH_CONST_I32 2            // index
GET_ARRAY                   // Bounds checks & reads element
SET_LOCAL 2                 // val

// Write
PUSH_CONST_I32 50           // value
GET_LOCAL 1                 // arr
PUSH_CONST_I32 2            // index
SET_ARRAY                   // Bounds checks & writes element
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: In-Bounds Read and Write
```solix
int32[] buffer = new int32[3];
buffer[0] = 100;
buffer[1] = 200;
int32 sum = buffer[0] + buffer[1]; // 300
```
*Expected Result*: `sum` equals 300.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Index Out of Bounds (Runtime Fault)
```solix
int32[] data = new int32[2];
int32 fail = data[5]; // Out of bounds
```
*Expected Runtime Exception*:
```text
[FATAL VM PANIC] IndexOutOfBoundsException: Index 5 out of bounds for array length 2
```
