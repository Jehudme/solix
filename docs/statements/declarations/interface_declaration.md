# §5 InterfaceDeclaration

## 1. Overview & Scope

An `InterfaceDeclaration` defines an abstract behavioral contract that implementing classes must satisfy. Interfaces declare method signatures without bodies. In Solix, classes may implement multiple interfaces, enabling flexible polymorphism without multiple class inheritance.

Interface references are first-class types managed by ARC. Methods invoked on interface references are dispatched dynamically via Interface Dispatch Tables (ITables) or polymorphic VTable slot lookups.

### Syntactic Placement
An `InterfaceDeclaration` is legally permitted at global package scope or nested directly inside a class.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
InterfaceDeclaration ::= 'interface' Identifier SuperInterfaces? InterfaceBody
SuperInterfaces      ::= 'extends' QualifiedType (',' QualifiedType)*
InterfaceBody        ::= '{' InterfaceMember* '}'
InterfaceMember      ::= MethodSignature ';'
MethodSignature      ::= TypeSpecifier Identifier '(' ParameterList? ')'
```

### Canonical Code Patterns
```solix
interface Serializable {
    String serialize();
}

interface Drawable {
    void draw(int32 x, int32 y);
}
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Contract Obligations
- All methods declared in an interface are implicitly `public` and `abstract`.
- Any concrete class implementing an interface must provide a matching implementation for every method signature in the interface and its ancestor interfaces.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Compile-Time Conformance Checking
- During Pass 2 (`BIND_EXECUTION`), the binder verifies that every implementing class defines all required method signatures with exact parameter and return type matches.

### 4.2 Dynamic Dispatch
- When invoking a method on an interface reference (`p.print()`), the VM resolves the target method using the concrete instance's VTable/ITable map.

---

## 5. Memory Model & ARC Invariants

### 5.1 Reference Pointer Lifetime
- Variables declared with an interface type hold an 8-byte heap object reference and participate in ARC identically to class references (`INC_REF` on assign, `DEC_REF` on scope exit).

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Interface Method with Body
Interface methods cannot have bodies.
```solix
interface Reader {
    int32 read() { return 0; } // Error: method cannot have body
}
```
*Diagnostic Message*:
```text
[ERROR] parser.cpp: Interface methods cannot have a body
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Interface Method Call on Null
Invoking an interface method on a null reference raises a `NullReferenceException`.

---

## 8. Conformance & Verification Examples

### Example 8.1: Multiple Interface Conformance
```solix
interface A { void a(); }
interface B { void b(); }

class Target implements A, B {
    void a() {}
    void b() {}
}

void verify() {
    A refA = new Target();
    B refB = (B)refA; // Valid cross-interface cast
}
```
*Verification Invariant*: Static analysis passes; runtime `CAST_CHECK` validates interface conformance.
