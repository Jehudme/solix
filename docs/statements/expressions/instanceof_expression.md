# §29 InstanceOfExpression

## 1. Overview & Scope

An `InstanceOfExpression` (`expr instanceof TargetType`) evaluates whether an object instance conforms at runtime to a specified class or interface type. It yields a boolean `true` if the object inherits from or implements the target type, and `false` otherwise (or if the evaluated reference is `null`).

---

## 2. Syntax & Production Rules

### Production Rules
```solix
InstanceOfExpression ::= Expression 'instanceof' QualifiedType
```

---

## 3. Scope & Declaration Space (Static Semantics)

The LHS must be a reference type; the RHS must be a declared class or interface.

---

## 4. Operational Semantics (Dynamic Execution)

1. Evaluates LHS expression onto operand stack.
2. Emits `OpCode::INSTANCEOF <target_vtable_id>`.
3. If reference is `null`, returns `false`.
4. Otherwise, checks target type against instance VTable hierarchy table. Pushes `bool`.

---

## 5. Memory Model & ARC Invariants

Pushes scalar `bool`. Incurs 0 ARC refcount changes.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: InstanceOf with Primitive Type
```solix
bool b = 10 instanceof int32; // Error
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: 'instanceof' cannot be applied to primitive types
```

---

## 7. Runtime Fault Conditions

None; null references safely evaluate to `false`.

---

## 8. Conformance & Verification Examples

### Example 8.1: Safe Hierarchy Query
```solix
Animal a = null;
bool is_dog = a instanceof Dog; // Evaluates to false safely
```
*Verification Invariant*: `is_dog` is `false`.
