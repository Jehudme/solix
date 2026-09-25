# MethodDeclaration

## 1. Overview & Purpose

A `MethodDeclaration` defines a callable member function. Methods support parameter overloading, instance dispatch (with implicit `this` at slot 0), static dispatch, and abstract signatures.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### How It Compiles
- **Static Methods**: Emits direct `CALL` with fixed symbol address.
- **Instance Methods**: Emits `CALL_VIRTUAL <slot>` using the receiver's VTable.
- **Cleanup**: Before `RETURN`, emits `DEC_REF` for reference parameters and for `this` (slot 0).

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Virtual Method Overriding
```solix
class Parent { String name() { return "parent"; } }
class Child extends Parent { String name() { return "child"; } }

void test() {
    Parent p = new Child();
    Console.println(p.name()); // Prints: child
}
```
*Expected Result*: Dispatches dynamically to `Child.name()`.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Abstract Method with Body
```solix
abstract class Base {
    abstract void run() {} // Error: cannot have body
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Abstract method 'run' cannot have a body
```
