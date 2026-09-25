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

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Selective Import
```solix
import std.collections.List;

void test() {
    List items = new List();
}
```
*Expected Result*: Compiles cleanly; `List` resolves to `std.collections.List`.

### Case 3.2: Wildcard Import
```solix
import std.io.*;

void test() {
    Console.println("Hello");
}
```
*Expected Result*: `Console` resolves to `std.io.Console`.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Importing Non-Existent Package
```solix
import invalid.pkg.Foo;
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot resolve import 'invalid.pkg.Foo': package or symbol not found
```

### Case 4.2: Ambiguous Symbol Collision
```solix
import pkg_a.*; // defines Token
import pkg_b.*; // also defines Token

void test() {
    Token t; // Error: ambiguous
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Reference to 'Token' is ambiguous: matches 'pkg_a.Token' and 'pkg_b.Token'
```
