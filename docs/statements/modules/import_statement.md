# ImportStatement

## 1. Overview & Purpose

An `ImportStatement` brings external types or entire package namespaces into the current file's scope so you can refer to them without typing verbose qualified names:
- **Selective Import**: `import std.collections.List;` (imports only `List`).
- **Wildcard Import**: `import std.io.*;` (imports all symbols in `std.io`).

Imports are processed purely at compile time during symbol resolution and produce **zero runtime bytecode**.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### How It Compiles
1. In Pass 1a, the compiler parses import directives and populates the translation unit's import table.
2. When the binder encounters an unqualified identifier (e.g. `List`), it checks:
   - Current lexical block & class members.
   - Declarations within the current package.
   - Selective imports.
   - Wildcard imports.
3. If an identifier matches symbols in multiple wildcard-imported packages, the compiler flags an ambiguity error.

---

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [ImportStatement in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#importstatement).
