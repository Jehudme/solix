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

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [InterfaceDeclaration in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#interfacedeclaration).
