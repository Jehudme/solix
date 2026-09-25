# §1 PackageStatement

## 1. Overview & Scope

A `PackageStatement` defines the logical namespace boundary for a Solix compilation unit (source file). It partitions top-level declarations (classes, interfaces, enums, global variables, and top-level functions) into structured, dot-delimited namespaces, preventing identifier collisions across distinct libraries and modular subsystems.

In Solix's compiler pipeline, the package statement is evaluated during Pass 1a (`REGISTER_GLOBALS`) and Pass 1b (`REGISTER_MEMBERS`). The compiler stamps the package identity onto all child AST nodes via `node->package_context`, guaranteeing that multi-file compilation correctly isolates types even when ASTs from different source files are combined into flat compilation lists.

### Syntactic Placement
A `PackageStatement` is legally permitted **only as the very first non-comment statement** of a compilation unit. At most one `PackageStatement` is permitted per source file.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
PackageStatement    ::= 'package' QualifiedIdentifier ';'
QualifiedIdentifier ::= Identifier ('.' Identifier)*
```

### Canonical Code Patterns
```solix
// 1. Single-Level Package
package network;

// 2. Hierarchical Nested Package
package std.collections.generic;

// 3. Application Domain Package
package com.company.billing.engine;
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Namespace Prefixing & Symbol Table Registration
- When a package statement `package P;` is encountered, all top-level symbols declared within that file have their canonical names prefixed with `P + "."`.
- For example, a class `List` declared in `package std.collections;` is registered in the compiler's symbol table as `std.collections.List`.
- The compiler registers `P` into the `known_packages` table to facilitate cross-file import discovery.

### 3.2 Intra-Package Visibility
- Declarations within the same package share implicit namespace visibility. A class in `package std.io;` can reference another class in `package std.io;` without requiring an explicit `import` statement.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Normal Completion
- A `PackageStatement` is a purely static, compile-time construct.
- It produces **zero bytecode opcodes** and generates no runtime memory artifacts.
- During compilation, the binder processes the package statement during Pass 1a, stamps child nodes, and completes compilation of the statement immediately.

### 4.2 Abrupt Completion
- A `PackageStatement` cannot complete abruptly at runtime.

---

## 5. Memory Model & ARC Invariants

### 5.1 Zero Runtime Memory Footprint
- Packages exist solely within the compiler's symbol table and debug metadata tables.
- They incur zero heap allocations, zero stack frames, and zero reference counting operations at runtime.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Package Statement Position
A `PackageStatement` must be the first executable declaration in the file.
```solix
import std.io;
package my_app; // Error: package must appear before imports and declarations
```
*Diagnostic Message*:
```text
[ERROR] parser.cpp: 'package' statement must be the first statement in the file
```

### Rule 6.2: Multiple Package Statements
A single file cannot declare more than one package.
```solix
package alpha;
package beta; // Error: duplicate package declaration
```
*Diagnostic Message*:
```text
[ERROR] parser.cpp: Only one 'package' statement is allowed per file
```

---

## 7. Runtime Fault Conditions

A `PackageStatement` generates no dynamic runtime faults; verification is performed statically during compilation.

---

## 8. Conformance & Verification Examples

### Example 8.1: Multi-Package Type Resolution
```solix
// File 1: MathA.slx
package math.algebra;
class Vector2 { int32 x; int32 y; }

// File 2: MathB.slx
package math.geometry;
class Vector2 { float64 x; float64 y; }
```
*Verification Invariant*: Both classes compile cleanly without collision; the compiler distinguishes `math.algebra.Vector2` from `math.geometry.Vector2`.
