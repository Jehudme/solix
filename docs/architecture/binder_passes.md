# Semantic Binder Passes

## 1. Overview

The `Binder` stage (`language/src/processes/binder.cpp`) is the core semantic engine of the Solix compiler. Because Solix supports out-of-order type references, cyclic type relationships, class inheritance, method overloading, and template monomorphization, binding cannot occur in a single top-to-bottom AST traversal. Instead, the `Binder` executes **four sequential passes**.

---

## 2. The Four Passes

### Pass 1a: Symbol Discovery & Package Registration
- **Goal**: Register the names of all top-level types across all compilation units before their bodies are inspected.
- **Operations**:
  - Traverses `ProgramNode` across all source files.
  - Sets active `current_package` from `PackageStatementNode`.
  - Registers all `ClassDeclaration`, `InterfaceDeclaration`, and `EnumDeclaration` symbols in the global scope table `global_scope` using their fully qualified names (e.g., `solix.core.String`).
  - Registers `alias` definitions and collects `import` statements into pending import queues.
  - Resolves package imports (both explicit like `import solix.core.String;` and wildcards like `import solix.collections.*;`).

### Pass 1b: Type Hierarchy & Member Registration
- **Goal**: Establish inheritance relationships and populate class member signatures without type-checking method bodies.
- **Operations**:
  - Resolves `extends` base class and `implements` interface types using `resolve_symbol`.
  - Validates that base classes exist and checks for cyclic inheritance loops (`A extends B extends A`).
  - Registers all instance and static fields, calculating memory offset words for class instance layout.
  - Registers constructor signatures and method signatures (return type, parameter types).
  - Assigns unique sequential `vtable_id` numbers to all non-primitive, non-interface classes.
  - Constructs class virtual tables, resolving virtual method overrides and calculating VTable slot indices.

### Pass 2: Type Checking, Scoping & ARC Hook Insertion
- **Goal**: Full type validation of statements and expressions, frame layout determination, and insertion of memory management operations.
- **Operations**:
  - Validates expression types, binary operations, method argument matching, and explicit casts.
  - Manages lexical scopes (`Scope` stack) for local variables, tracking `slot_index` offsets within the call frame.
  - Enforces `const` variable immutability.
  - Tracks reference type variables leaving scope:
    - Automatically synthesizes and inserts `DEC_REF` cleanup nodes for all active reference variables when exiting blocks, breaking, continuing, or returning.
  - For method calls, performs overload resolution by mangling call argument types (`ClassName.method(Type1,Type2)`).

### Pass 3: Template Monomorphization
- **Goal**: Instantiate generic blueprint classes into concrete AST nodes.
- **Operations**:
  - Inspects all instantiated generic types (e.g., `List<int32>`, `HashMap<String, User>`).
  - Clones the AST template definition registered in `template_registry`.
  - Recursively substitutes generic type parameters (e.g. `T` $\to$ `int32`) with their concrete type arguments.
  - Re-runs Passes 1a, 1b, and 2 over the monomorphized class AST subtree to validate types, assign dedicated VTable IDs, and bind methods.
