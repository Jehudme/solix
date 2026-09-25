# §31 ArrayCreationExpression

## 1. Overview & Scope

An `ArrayCreationExpression` (`new ElementType[size]`) dynamically allocates a contiguous array buffer on the heap. Arrays are first-class reference types governed by ARC.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
ArrayCreationExpression ::= 'new' TypeSpecifier '[' Expression ']'
```

---

## 3. Scope & Declaration Space (Static Semantics)

The size expression must evaluate to an integral type (`int32`, `int64`, etc.).

---

## 4. Operational Semantics (Dynamic Execution)

1. Evaluates size expression onto operand stack.
2. Checks size >= 0.
3. Allocates array header (element count, element size) and buffer.
4. Clears memory to default zero/null values.
5. Pushes array reference onto stack.

---

## 5. Memory Model & ARC Invariants

Array buffer is managed as an ARC reference object.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Non-Integral Size
```solix
int32[] arr = new int32["ten"]; // Error
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Array size must be an integer, got 'String'
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Negative Array Size
Raises `NegativeArraySizeException` if size < 0.

---

## 8. Conformance & Verification Examples

### Example 8.1: Array Buffer Allocation
```solix
int32[] numbers = new int32[100];
```
*Verification Invariant*: `numbers.length` equals 100; memory allocated and initialized to 0.
