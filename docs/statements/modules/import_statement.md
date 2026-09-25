# §2 ImportStatement

## 1. Overview & Scope

An `ImportStatement` introduces external symbols or entire package namespaces into the current compilation unit's lexical scope. Imports allow programmers to refer to types, functions, and global constants defined in other files or libraries without using verbose, fully qualified package paths.

Solix supports two forms of importation:
1. **Selective Imports**: Imports a specific named type or symbol (e.g. `import std.collections.List;`), preventing namespace pollution.
2. **Wildcard Imports**: Imports all publicly exported symbols from a package (e.g. `import std.io.*;`), enabling bulk symbol availability.

### Syntactic Placement
An `ImportStatement` is legally permitted only at the top of a compilation unit, occurring immediately after the optional `PackageStatement` and before any type, variable, or function declarations.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
ImportStatement     ::= 'import' QualifiedIdentifier ('.' '*' | '.' Identifier)? ';'
QualifiedIdentifier ::= Identifier ('.' Identifier)*
```

### Canonical Code Patterns
```solix
// 1. Selective Single-Type Import
import std.collections.List;

// 2. Wildcard Package Import
import std.io.*;

// 3. Importing Custom Application Subsystems
import my_project.services.AuthenticationService;
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Unqualified Symbol Lookup Resolution
- When an unqualified identifier (e.g. `List`) is resolved by the binder, the lookup resolution order is:
  1. Local scope / enclosing block declarations.
  2. Member declarations of current class and inherited ancestors.
  3. Declarations within the current package.
  4. Explicitly imported selective symbols (`import a.b.Type;`).
  5. Wildcard-imported package symbols (`import a.b.*;`).
- If an identifier cannot be found across these stages, compilation halts with an `Undefined identifier` error.

### 3.2 Collision & Disambiguation Rules
- If two wildcard imports supply an identical unqualified type name (e.g. `import pkgA.*; import pkgB.*;` where both define `Widget`):
  - Referencing `Widget` unqualified produces an `Ambiguous symbol reference` compile-time error.
  - The programmer must disambiguate by using the fully qualified name `pkgA.Widget` or adding an explicit selective import.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Normal Completion
- An `ImportStatement` is a purely static compile-time directive.
- It produces **zero bytecode opcodes** and emits no runtime structures.
- During semantic analysis (Pass 1a and 1b), the binder populates the file's import table and completes processing immediately.

### 4.2 Abrupt Completion
- An `ImportStatement` cannot complete abruptly at runtime.

---

## 5. Memory Model & ARC Invariants

### 5.1 Zero Runtime Footprint
- Imports exist exclusively within the compiler's symbol table lookup pipeline and incur 0 bytes of memory or execution time at runtime.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Non-Existent Package Import
Attempting to import a package that is not present in the compilation project is rejected.
```solix
import non_existent_library.Module;
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Cannot resolve import 'non_existent_library.Module': package or symbol not found
```

### Rule 6.2: Ambiguous Symbol Collision
Using an unqualified type that exists in multiple wildcard-imported packages is rejected.
```solix
import pkg_a.*; // defines Token
import pkg_b.*; // also defines Token

void test() {
    Token t; // Error: ambiguous
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Reference to 'Token' is ambiguous: matches 'pkg_a.Token' and 'pkg_b.Token'
```

---

## 7. Runtime Fault Conditions

An `ImportStatement` generates no dynamic runtime faults; validation is entirely static.

---

## 8. Conformance & Verification Examples

### Example 8.1: Disambiguation via Explicit Qualification
```solix
import pkg_a.*;
import pkg_b.*;

void verify_qualification() {
    pkg_a.Token t1 = new pkg_a.Token(); // Fully qualified: succeeds
    pkg_b.Token t2 = new pkg_b.Token(); // Fully qualified: succeeds
}
```
*Verification Invariant*: Static analysis succeeds without ambiguity errors.
