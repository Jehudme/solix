# §9 MethodDeclaration

## 1. Overview & Scope

A `MethodDeclaration` defines a callable member function within a class or interface, or a top-level function at global package scope. Methods encapsulate executable logic, accept typed parameters, and yield an optional return value.

Solix supports instance methods (possessing an implicit `this` pointer), static methods (called directly on the type without an instance), and abstract methods (requiring concrete subclass implementation). Instance methods participate in virtual dynamic dispatch via Virtual Method Tables (VTables).

### Syntactic Placement
A `MethodDeclaration` is permitted inside class and interface bodies, or at global translation-unit scope.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
MethodDeclaration ::= MethodModifier* TypeSpecifier Identifier '(' ParameterList? ')' (BlockStatement | ';')
MethodModifier    ::= 'public' | 'private' | 'protected' | 'static' | 'abstract' | 'final'
```

### Canonical Code Patterns
```solix
class Calculator {
    public static int32 add(int32 a, int32 b) {
        return a + b;
    }

    public int32 multiply(int32 a, int32 b) {
        return a * b;
    }
}
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Parameter Scope & Monotonic Frame Slots
- Parameters are allocated frame registers sequentially starting from slot 1 (or slot 0 for static methods).
- Instance methods reserve slot 0 strictly for `this`.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Dispatch Lowering
- **Static Methods**: Emits direct function call instruction with fixed jump address.
- **Instance Methods**: Emits indirect virtual call through the receiver's VTable slot ID.

---

## 5. Memory Model & ARC Invariants

### 5.1 Parameter ARC Cleanup
- Any reference parameters passed into a method are decremented via `DEC_REF` prior to `OpCode::RETURN`.
- Slot 0 (`this`) is decremented on method exit for instance methods.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Abstract Method with Body
```solix
abstract class Base {
    abstract void run() {} // Error: abstract method cannot have body
}
```
*Diagnostic Message*:
```text
[ERROR] parser.cpp: Abstract method 'run' cannot have a body
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Instance Method Call on Null
Calling an instance method on a null reference raises `NullReferenceException`.

---

## 8. Conformance & Verification Examples

### Example 8.1: Overriding Method Dispatch
```solix
class Parent { String tag() { return new String("Parent"); } }
class Child extends Parent { String tag() { return new String("Child"); } }
```
*Verification Invariant*: Calling `tag()` on a `Parent` reference holding `Child` dynamically invokes `Child.tag`.
