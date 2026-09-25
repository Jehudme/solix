# §7 FieldDeclaration

## 1. Overview & Scope

A `FieldDeclaration` defines state storage either within a class instance (instance fields) or globally across a class/package (static fields). Fields support access modifiers (`public`, `private`, `protected`), type specifiers, and optional initializers.

Solix provides special support for the `weak` ownership modifier on reference fields. Marking a field as `weak` prevents Automatic Reference Counting (ARC) reference cycles (such as parent-child object graphs) from leaking memory.

### Syntactic Placement
A `FieldDeclaration` is permitted inside class bodies or at global package scope (for top-level global variables).

---

## 2. Syntax & Production Rules

### Production Rules
```solix
FieldDeclaration ::= Modifier* 'weak'? TypeSpecifier Identifier ('=' Expression)? ';'
Modifier         ::= 'public' | 'private' | 'protected' | 'static'
```

### Canonical Code Patterns
```solix
class Node {
    public int32 id;
    public Node next;      // Strong reference (increments ref_count)
    public weak Node prev; // Weak reference (breaks cyclic dependency!)
}
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Field Visibility & Access Control
- Evaluated during Pass 1b (`REGISTER_MEMBERS`).
- Member access checks enforce `private` and `protected` boundaries during semantic analysis.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Instance Field Offsets
- Each instance field is assigned a memory byte offset within the class layout.
- Access emits `GET_PROPERTY <offset>` or `SET_PROPERTY <offset>`.
- For `weak` fields, writing emits `WEAK_SET_PROPERTY <offset>`, which records the pointer without incrementing the target object's `ref_count`.

---

## 5. Memory Model & ARC Invariants

### 5.1 Strong vs Weak Reference Fields
- **Strong Reference Field**: Writing an object pointer increments `ref_count` via `INC_REF`; overwriting or destroying decrements old value via `DEC_REF`.
- **Weak Reference Field**: Writing does not increment `ref_count`, preventing cyclic leaks.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Duplicate Field Identifier
```solix
class Test {
    int32 count;
    float64 count; // Error: duplicate
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Field 'count' is already declared in class 'Test'
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Field Access on Null Receiver
Reading or writing a field on a null reference triggers an immediate `NullReferenceException`.

---

## 8. Conformance & Verification Examples

### Example 8.1: Cyclic Memory Reclamation with Weak Field
```solix
class Parent {
    Child child;
}
class Child {
    weak Parent parent; // Weak back-pointer
}
```
*Verification Invariant*: When `Parent` reference count drops to 0, both `Parent` and `Child` are deallocated cleanly.
