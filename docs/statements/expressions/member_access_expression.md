# §34 MemberAccessExpression

## 1. Overview & Scope

A `MemberAccessExpression` (`receiver.member`) accesses a field, nested member, or enum value on an object instance or type namespace. For instance fields, the compiler resolves the byte offset within the instance memory layout.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
MemberAccessExpression ::= Expression '.' Identifier
```

---

## 3. Scope & Declaration Space (Static Semantics)

Resolves member against receiver type's symbol table, verifying access modifiers (`public`, `private`, `protected`).

---

## 4. Operational Semantics (Dynamic Execution)

1. Evaluates receiver expression.
2. Checks for `null` receiver.
3. Emits `OpCode::GET_PROPERTY <offset>` or `OpCode::SET_PROPERTY <offset>`.

---

## 5. Memory Model & ARC Invariants

Reading a reference field onto the stack does not increment refcount until stored in an owning lvalue.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Inaccessible Private Field
```solix
class A { private int32 x; }
void test() { A a = new A(); a.x = 5; } // Error: private
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Cannot access private field 'x' of class 'A'
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Null Receiver Dereference
Accessing a member on `null` triggers `NullReferenceException`.

---

## 8. Conformance & Verification Examples

### Example 8.1: Instance Field Access
```solix
Point p = new Point(10, 20);
int32 x_coord = p.x;
```
*Verification Invariant*: `x_coord` equals 10.
