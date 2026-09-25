# §37 LiteralNode

## 1. Overview & Scope

A `LiteralNode` represents a constant scalar or string value embedded directly in program source text. Solix supports integer, floating-point, boolean, character, string, and `null` literals.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
Literal ::= IntegerLiteral
          | FloatLiteral
          | BooleanLiteral
          | CharacterLiteral
          | StringLiteral
          | 'null'
```

---

## 3. Scope & Declaration Space (Static Semantics)

Literals carry intrinsic types:
- `10`: `int32`
- `10L`: `int64`
- `3.14`: `float64`
- `true`, `false`: `bool`
- `'a'`: `char`
- `"text"`: `String`
- `null`: `null` (assignable to any reference type)

---

## 4. Operational Semantics (Dynamic Execution)

Emits immediate push instructions: `PUSH_INT`, `PUSH_FLOAT`, `PUSH_STRING`, `PUSH_NULL`.

---

## 5. Memory Model & ARC Invariants

- String literals are pooled in the static string pool.
- `null` is represented by pointer `0x0`.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Numeric Overflow in Literal
```solix
int32 x = 99999999999999999999; // Overflow
```
*Diagnostic Message*:
```text
[ERROR] lexer.cpp: Integer literal out of range for type 'int32'
```

---

## 7. Runtime Fault Conditions

None.

---

## 8. Conformance & Verification Examples

### Example 8.1: Null Reference Assignment
```solix
String s = null;
if (s == null) {
    Console.println("is null");
}
```
*Verification Invariant*: `s == null` evaluates to `true`.
