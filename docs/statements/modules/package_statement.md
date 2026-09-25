# PackageStatement

## 1. Overview & Purpose

A `PackageStatement` (`package name.subname;`) defines the namespace boundary for all declarations in a source file. It prevents global name collisions between libraries and structures the codebase into hierarchical modules.

In Solix:
- A file may declare at most one package statement, and it **must be the very first non-comment statement**.
- All top-level classes, functions, and global variables declared in the file are prefixed with the package name in the compiler's symbol table (e.g. `std.collections.List`).
- A package statement is a compile-time directive: it emits **zero bytecode instructions** and incurs zero runtime memory overhead.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### How It Compiles
During Pass 1a (`REGISTER_GLOBALS`) and Pass 1b (`REGISTER_MEMBERS`):
1. The compiler registers the package name in its `known_packages` table.
2. The package string is stamped onto AST child nodes via `node->package_context`.
3. When types are registered, their symbol keys are recorded as `package_name + "." + type_name`.
4. **Bytecode Output**: None. Packages exist strictly as compiler metadata.

---

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [PackageStatement in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#packagestatement).
