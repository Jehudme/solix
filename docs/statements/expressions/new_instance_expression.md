# §30 NewInstanceExpression

## 1. Overview & Scope

A `NewInstanceExpression` (`new ClassName(arguments)`) allocates a new instance of a class on the heap, initializes its internal ARC header and VTable metadata, and invokes the matching constructor.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
NewInstanceExpression ::= 'new' QualifiedType '(' ArgumentList? ')'
```

---

## 3. Scope & Declaration Space (Static Semantics)

The target class must be concrete (non-abstract). Constructor overload matching resolves arguments.

---

## 4. Operational Semantics (Dynamic Execution)

1. Allocates heap buffer (`OpCode::ALLOC <size>`).
2. Initializes object header (`ref_count = 1`, `vtable_id`).
3. Invokes constructor with instance at slot 0 (`this`).
4. Pushes newly created reference pointer onto stack top.

---

## 5. Memory Model & ARC Invariants

Newly created object begins with `ref_count = 1`.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Instantiating Abstract Class
```solix
abstract class Base {}
Base b = new Base(); // Error
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Cannot instantiate abstract class 'Base'
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Heap Memory Exhaustion
Raises `OutOfMemoryException` if heap allocation fails.

---

## 8. Conformance & Verification Examples

### Example 8.1: Instantiation Lifecycle
```solix
Widget w = new Widget(100);
```
*Verification Invariant*: `w` reference count is 1. Destructor runs when `w` exits scope.
