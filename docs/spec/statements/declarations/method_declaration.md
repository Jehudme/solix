# MethodDeclaration

## 1. Overview & Purpose

A `MethodDeclaration` defines a callable member function or package-level free function. Methods support parameter overloading, instance dispatch (with implicit `this` at slot 0), static dispatch, and abstract signatures.

---

## 2. Declaration Scope Rules

### 2.1 Free / Top-Level Functions
- Functions declared outside of any enclosing class (at compilation unit or package level) are inherently static.
- Specifying the `static` modifier on a function outside of a class is prohibited and raises compile-time diagnostic:
  `[E_BIND] 'static' modifier is not allowed on functions outside of a class`.
- Free functions can serve as application entry points (e.g., `main()`) without needing any modifier keyword.

### 2.2 Class Member Methods
- Functions declared inside a class may be instance methods or marked with `static` to denote static member functions.
- When an entry point function is defined inside a class, it must be declared `public static`.

---

## 3. Compilation & Runtime Mechanics (With Real Bytecode)

### How It Compiles
- **Static Methods & Free Functions**: Emits direct `CALL` with fixed symbol address.
- **Instance Methods**: Emits `CALL_VIRTUAL <slot>` using the receiver's VTable.
- **Cleanup**: Before `RETURN`, emits `DEC_REF` for reference parameters and for `this` (slot 0).

