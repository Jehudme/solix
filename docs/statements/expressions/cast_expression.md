# §28 CastExpression

## 1. Overview & Scope

A `CastExpression` (`(TargetType)expr`) explicitly converts a value from one type to another. Solix supports numeric primitive conversions, identity casts, and reference type upcasting and downcasting.

For reference types, downcasting emits a runtime `CAST_CHECK` opcode that validates class hierarchy conformance, throwing a `TypeCastException` if the instance does not conform.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
CastExpression ::= '(' TypeSpecifier ')' Expression
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Cast Validity
- Casting between unrelated class hierarchies (where neither extends the other) is rejected at compile time.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Execution
- **Primitive Conversion**: Emits conversion opcode (e.g. `I32_TO_F64`).
- **Reference Downcast**: Emits `OpCode::CAST_CHECK <vtable_id>`. The VM inspects the instance header; if incompatible, throws `TypeCastException`.

---

## 5. Memory Model & ARC Invariants

Reference casting preserves the underlying object pointer; refcount is unchanged.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Unrelated Hierarchy Cast
```solix
class A {} class B {}
void test() { A a = new A(); B b = (B)a; }
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Cannot cast from 'A' to unrelated type 'B'
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Invalid Dynamic Downcast
```solix
Animal a = new Animal();
Dog d = (Dog)a; // Throws at runtime
```
*Runtime Fault*:
```text
[FATAL VM PANIC] TypeCastException: Cannot cast 'Animal' to 'Dog'
```

---

## 8. Conformance & Verification Examples

### Example 8.1: Polymorphic Downcast
```solix
Animal a = new Dog();
Dog d = (Dog)a; // Succeeds
```
*Verification Invariant*: `d` holds valid reference to `Dog`.
