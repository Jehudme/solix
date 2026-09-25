# InterfaceDeclaration

## 1. Overview & Purpose

An `InterfaceDeclaration` defines an abstract behavioral contract that implementing classes must satisfy. Interfaces declare method signatures without bodies. Classes can implement multiple interfaces.

Interface variables are reference types managed by ARC. Methods called on interface references are dispatched dynamically at runtime.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### How It Compiles
1. The compiler verifies in Pass 2 that every implementing class provides concrete implementations for all interface signatures.
2. Invocations on interface references emit `CALL_VIRTUAL` referencing the interface method slot.

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Multiple Interface Conformance
```solix
interface Printable { void print(); }
interface Serializable { void save(); }

class Doc implements Printable, Serializable {
    void print() { Console.println("print"); }
    void save() { Console.println("save"); }
}
```
*Expected Result*: Compiles cleanly; instances can be passed to functions taking either `Printable` or `Serializable`.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Interface Method with Body
```solix
interface Reader {
    int32 read() { return 0; } // Error: method cannot have body
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Interface methods cannot have a body
```
