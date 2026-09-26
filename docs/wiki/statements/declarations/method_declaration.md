# MethodDeclaration

## 1. Overview & Purpose

A `MethodDeclaration` defines a callable member function. Methods support parameter overloading, instance dispatch (with implicit `this` at slot 0), static dispatch, and abstract signatures.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### How It Compiles
- **Static Methods**: Emits direct `CALL` with fixed symbol address.
- **Instance Methods**: Emits `CALL_VIRTUAL <slot>` using the receiver's VTable.
- **Cleanup**: Before `RETURN`, emits `DEC_REF` for reference parameters and for `this` (slot 0).
