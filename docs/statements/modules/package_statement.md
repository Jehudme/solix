# PackageStatement (`NodeType::PACKAGE_STMT`)

## 1. Description & Purpose

The `package` statement establishes the compilation unit's logical namespace boundary. It defines the root organizational unit in Solix, preventing global symbol collisions across disparate libraries and modules. All top-level declarations (classes, interfaces, enums, global variables, and top-level functions) following a `package` statement are scoped within that package namespace. Packages are hierarchical, separated by dot delimiters (e.g., `std.collections`).

## 2. Syntax & Grammar

```solix
package <identifier> ('.' <identifier>)* ';'
```

## 3. Underlying Systems & Mechanics

- Evaluated during Pass 1a (`REGISTER_GLOBALS`) and Pass 1b (`REGISTER_MEMBERS`).
- Stamped onto AST child nodes via `node->package_context` to guarantee namespace context preservation across flat multi-file passes.
- Prefixes all top-level symbols (classes, enums, global aliases) with `package_name + "."`.
- Registers the package into the compiler's `known_packages` table for cross-file resolution.
- Emits zero runtime bytecode; purely compile-time namespace partitioning.

## 4. Positive Test Scenarios (Valid Variations)

1. **Single-Level Package**: `package solix;`
2. **Deep Hierarchical Package**: `package com.solix.advanced.render.vulkan;`
3. **Implicit Root Package**: Omitting `package` places all declarations in the global root namespace.
4. **Multiple Files Sharing One Package**: Multiple distinct `.slx` source files declaring the identical package name merge symbols into the same package namespace.

## 5. Negative Test Scenarios (Invalid Variations)

1. **Multiple Package Declarations in One File**:
   - `package a; package b;`  
     *Error*: `Multiple package declarations in single compilation unit`
2. **Package Declaration After Top-Level Code**:
   - `class Item {} package my_pkg;`  
     *Error*: `Package statement must be the first statement in the file`
3. **Invalid Characters or Numerics in Package Path**:
   - `package 123.foo;`  
     *Error*: `Expected identifier in package declaration`
4. **Missing Semicolon**:
   - `package com.foo`  
     *Error*: `Expected ';' after package name`
