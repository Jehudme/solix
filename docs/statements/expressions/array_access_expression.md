# §32 ArrayAccessExpression

## 1. Overview & Scope

An `ArrayAccessExpression` (`array[index]`) accesses or modifies an element in a contiguous array. It functions as an rvalue (reading an element value) and as an lvalue (target of assignment).

The VM performs mandatory runtime bounds checking against the array length, throwing `IndexOutOfBoundsException` on violation.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
ArrayAccessExpression ::= Expression '[' Expression ']'
```

---

## 3. Scope & Declaration Space (Static Semantics)

LHS must be an array type (`array_depth > 0`). Index must be integral.

---

## 4. Operational Semantics (Dynamic Execution)

1. Evaluates array reference. If `null`, raises `NullReferenceException`.
2. Evaluates index. If `index < 0 || index >= length`, raises `IndexOutOfBoundsException`.
3. Reads or writes target memory offset.

---

## 5. Memory Model & ARC Invariants

Reading reference element increments refcount if stored in local variable.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Subscript on Non-Array
```solix
int32 x = 10;
x[0] = 5; // Error: x is not array
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Array subscript cannot be applied to non-array type 'int32'
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Index Out of Bounds
```solix
int32[] arr = new int32[5];
int32 bad = arr[10]; // Runtime fault
```
*Runtime Fault*:
```text
[FATAL VM PANIC] IndexOutOfBoundsException: Index 10 out of bounds for array length 5
```

---

## 8. Conformance & Verification Examples

### Example 8.1: Array Mutation
```solix
int32[] data = new int32[2];
data[0] = 10;
data[1] = 20;
```
*Verification Invariant*: `data[0]` is 10 and `data[1]` is 20.
