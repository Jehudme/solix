# §33 ArrayLiteralExpression

## 1. Overview & Scope

An `ArrayLiteralExpression` constructs and populates an array inline using bracket notation (`[elem1, elem2]`) or brace notation (`{elem1, elem2}`). The compiler infers element type from element unification and allocates the heap array.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
ArrayLiteralExpression ::= '[' ElementList? ']'
                         | '{' ElementList? '}'
ElementList            ::= Expression (',' Expression)* ','?
```

---

## 3. Scope & Declaration Space (Static Semantics)

Element expressions must unify to a common type.

---

## 4. Operational Semantics (Dynamic Execution)

1. Computes literal length $N$.
2. Allocates heap array buffer of size $N$.
3. Evaluates and stores each element in sequential index order.
4. Pushes array reference to stack top.

---

## 5. Memory Model & ARC Invariants

Creates array reference with `ref_count = 1`.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Incompatible Element Types
```solix
var arr = [10, "string"]; // Error: incompatible elements
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Incompatible types in array literal
```

---

## 7. Runtime Fault Conditions

None.

---

## 8. Conformance & Verification Examples

### Example 8.1: Inline Array Instantiation
```solix
int32[] primes = [2, 3, 5, 7, 11];
```
*Verification Invariant*: Array allocated with length 5 containing prime numbers.
