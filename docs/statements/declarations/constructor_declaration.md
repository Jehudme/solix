# §8 ConstructorDeclaration

## 1. Overview & Scope

A `ConstructorDeclaration` defines the initialization routine invoked immediately after an object instance is allocated in heap memory. Constructors share the exact name of their enclosing class, support parameter overloading, superclass constructor chaining, and member initializer lists (`: super(args), field(val)`).

In Solix, constructors allocate local register slot 0 to the implicit `this` instance pointer. The constructor is responsible for initializing all fields before the newly allocated object reference is returned to the caller.

### Syntactic Placement
A `ConstructorDeclaration` is permitted only directly within class declaration bodies.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
ConstructorDeclaration ::= Identifier '(' ParameterList? ')' MemberInitList? BlockStatement
MemberInitList         ::= ':' MemberInitializer (',' MemberInitializer)*
MemberInitializer      ::= Identifier '(' ArgumentList? ')'
```

### Canonical Code Patterns
```solix
class Point3D extends Point2D {
    int32 z;

    Point3D(int32 x, int32 y, int32 z) : super(x, y), z(z) {
        Console.println("Point3D initialized");
    }
}
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Initializer List Scope
- The member initializer list evaluates expressions in the scope of constructor parameters.
- `super(...)` must appear as the very first initializer if the class extends a base class.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Execution Sequence
1. Heap memory is allocated (`OpCode::ALLOC <size>`).
2. Object header is initialized (`ref_count = 1`, `vtable_id`).
3. Instance pointer stored in slot 0 (`this`).
4. Superclass constructor executes.
5. Member initializers execute, populating field offsets.
6. Constructor block statements execute.
7. Instance reference returned to stack top.

---

## 5. Memory Model & ARC Invariants

### 5.1 Constructor Abort Deallocation
If an exception is thrown inside a constructor body, the partially initialized instance in slot 0 is decremented and deallocated, ensuring no orphan heap memory is leaked.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Constructor Name Mismatch
A constructor must share the exact name of the enclosing class.
```solix
class Item {
    Widget() {} // Error: constructor name mismatch
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Constructor name 'Widget' does not match enclosing class 'Item'
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Super Constructor Invocation Failure
Runtime faults inside chained constructors unwind and abort object creation.

---

## 8. Conformance & Verification Examples

### Example 8.1: Member Initializer List Execution
```solix
class Vector {
    int32 x;
    int32 y;
    Vector(int32 x, int32 y) : x(x), y(y) {}
}
```
*Verification Invariant*: Field offsets for `x` and `y` are populated prior to entering the body.
