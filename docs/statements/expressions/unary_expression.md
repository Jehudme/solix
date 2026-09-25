# §27 UnaryExpression

## 1. Overview & Scope

A `UnaryExpression` applies an operation to a single operand. Solix supports prefix logical negation (`!`), bitwise complement (`~`), arithmetic negation (`-`), unary plus (`+`), as well as prefix and postfix increment (`++`) and decrement (`--`).

---

## 2. Syntax & Production Rules

### Production Rules
```solix
UnaryExpression ::= PrefixOperator Expression
                  | LValue PostfixOperator
PrefixOperator  ::= '!' | '~' | '-' | '+' | '++' | '--'
PostfixOperator ::= '++' | '--'
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Mutability Requirement for Increment/Decrement
- Prefix and postfix `++`/`--` require an assignable lvalue.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Prefix vs Postfix
- **Prefix (`++x`)**: Increments value in storage and pushes the new value.
- **Postfix (`x++`)**: Pushes original value to stack, then increments storage.

---

## 5. Memory Model & ARC Invariants

Direct scalar register mutation. Zero heap allocations.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Increment on Constant/Literal
```solix
++10; // Error: lvalue required
```
*Diagnostic Message*:
```text
[ERROR] parser.cpp: Invalid operand for increment operator: expected lvalue
```

---

## 7. Runtime Fault Conditions

None under normal operation.

---

## 8. Conformance & Verification Examples

### Example 8.1: Postfix vs Prefix Evaluation
```solix
int32 a = 5;
int32 b = a++; // b = 5, a = 6
int32 c = ++a; // c = 7, a = 7
```
*Verification Invariant*: `b` is 5, `c` is 7, `a` is 7.
