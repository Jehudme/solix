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

### 2.3 Entry Point Resolution & Ambiguity Detection
- The compiler performs an exhaustive scan across all compilation units to identify all candidate functions matching the entry point name configured in `CompilationOptions::entry_point` (defaults to `main`).
- If multiple candidates matching the entry point name exist across modules or classes (for example, two package-level `main()` functions in distinct packages), compilation is rejected with diagnostic:
  `[E_ASM] Ambiguous entry point '<name>': multiple candidates found:` listing the source path, line, and column for every candidate.
- If exactly one candidate is identified, it is selected as the program's boot entry point.
- If no candidate is identified, compilation succeeds in library mode without emitting a startup call.

---

## 3. Compilation & Runtime Mechanics (With Real Bytecode)

### How It Compiles
- **Static Methods & Free Functions**: Emits direct `CALL` with fixed symbol address.
- **Instance Methods**: Emits `CALL_VIRTUAL <slot>` using the receiver's VTable.
- **Cleanup**: Before `RETURN`, emits `DEC_REF` for reference parameters and for `this` (slot 0).

