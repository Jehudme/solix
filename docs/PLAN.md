# Solix Master Development Plan & Architectural Roadmap

> **Engineering Workflow**: Refer to [WORKFLOW.md](WORKFLOW.md) for the development lifecycle, branching, and testing protocol.

---

# Part I: Fixes & Stabilization

---

## Phase 1: Compiler Pipeline Integrity & Strict Stage Halting [COMPLETED]

### Status: COMPLETED & MERGED TO MASTER

### Issue
When the Solix compiler runs into syntax or binding errors (such as `Syntax Error in file.slx: Expected expression`), the error message is printed to the diagnostic sink, but the compilation pipeline blindly proceeds into the subsequent stages (`Binder`, `Assembler`). As a result, the assembler generates incomplete or corrupted bytecode, writes out `out.slxb`, and the CLI tool announces:
```
Successfully compiled to out.slxb
```
The CLI command exits with return code `0`. This breaks CI/CD pipelines, shell scripts, and build orchestrators that depend on exit codes to detect compilation failures, and creates corrupted binaries that crash unpredictably at runtime.

### Solution
1. **Pipeline Stage Status Checks**:
   - In [`language/src/compilation.cpp`](language/src/compilation.cpp):
     - After `lexer.execute()`: inspect `context.diagnostic->has_errors()`. If true, immediately abort compilation without executing `Parser`.
     - After `parser.execute()`: inspect `context.diagnostic->has_errors()`. If true, abort immediately without executing `Binder`.
     - After `binder.execute()`: inspect `context.diagnostic->has_errors()`. If true, abort immediately without executing `Assembler`.
     - Throw a dedicated `CompilationFailedException` containing stage failure statistics.
2. **Bytecode Emission Suppression**:
   - In [`launcher/src/commands/compile.cpp`](launcher/src/commands/compile.cpp), wrap `compile(options)` in a robust `try-catch` block.
   - If `CompilationFailedException` or any fatal diagnostic error is caught:
     - Do not serialize or create the output `.slxb` bytecode file. If an existing binary exists at the destination, remove it to prevent stale execution.
     - Print a clean summary of error counts.
     - Call `std::exit(1)`.
3. **Verification**:
   - Write Catch2 tests passing invalid syntax, undeclared types, and type mismatches. Verify that `compile()` throws `CompilationFailedException`, stops before downstream stages, and outputs no file.

---

## Phase 2: Compiler Logging Demotion & Diagnostics Context Overhaul [COMPLETED]

### Status: COMPLETED & MERGED TO MASTER

### Issue
1. **Noisy Info-Level Logging**:
   - The compiler currently floods `stdout` with fine-grained internal tracing under the `[info]` log level during normal runs:
     - `[info] [Parser] Parsing source file: ...`
     - `[info] [Parser] Declared package: ...`
     - `[info] [Parser] Parsing class 'X' at line Y`
     - `[info] [Parser] Completed parsing ... generated N AST nodes`
     - `[info] [Binder] Pass 1a: Registering package and top-level symbols...`
     - `[info] [Binder] Configured active package: ...`
     - `[info] [Binder] Starting Pass 2: Type and Memory Binding...`
   - Real compilation errors and warnings get buried under hundreds of lines of internal debug messages.
2. **Inadequate Error Context**:
   - Errors only report line numbers without visual source context or column carets.
3. **Absence of a Compiler Warning Subsystem**:
   - The compiler does not emit warnings for unused variables, shadowed variables, unused imports, implicit truncations, or dead code.

### Solution
1. **Log Level Demotion**:
   - Audit all log invocations in `lexer.cpp`, `parser.cpp`, `binder.cpp`, `assembler.cpp`, and `compilation.cpp`.
   - Demote all AST node traversals, token processing, pass transitions, and symbol lookups from `INFO` to `DEBUG` or `TRACE`.
   - Restrict `INFO` to high-level user-facing milestones (e.g., `Compiling 12 files...`, `Compilation completed in 42 ms`).
   - Default the CLI log level to quiet/minimal mode (`WARN` or clean console), requiring `-v` / `--log-level DEBUG` for granular inspection.
2. **Rich Diagnostic Formatting**:
   - In `Diagnostic::report_error` and `Diagnostic::report_warning`:
     - Format: `<file>:<line>:<col>: error: <message>`.
     - Read the source line from `CompilationContext::sources`, format the offending line, and print a caret indicator:
       ```
       src/main.slx:14:18: error: undeclared identifier 'count'
           int32 total = count + 1;
                         ^~~~~
       ```
3. **Compiler Warnings Engine**:
   - Add `Diagnostic::report_warning()` methods.
   - Implement initial warning passes in `Binder`:
     - Unused local variables and function parameters.
     - Shadowed declarations in nested scopes.
     - Unused aliases and imports.
     - Unreachable code after `return` or `throw`.

---

## Phase 3: CLI Binary Ergonomics & Program Exit Code Propagation [COMPLETED]

### Status: COMPLETED & MERGED TO MASTER

### Issue
1. **Binary Name**:
   - The CLI executable is currently built as `solix_launcher`. Idiomatic modern languages use the language name directly (e.g., `solix compile`, `solix run`).
2. **Program Return Code Discarded**:
   - In [`launcher/src/commands/execute.cpp`](launcher/src/commands/execute.cpp), `solix_launcher run` runs the VM, but unconditionally terminates with `0` regardless of what `main()` returned. If a user writes `public static int32 main(char[][] args) { return 42; }`, the shell `$?` is still `0`.

### Solution
1. **Executable Renaming in CMake**:
   - In [`launcher/CMakeLists.txt`](launcher/CMakeLists.txt):
     - Update executable target name to `solix` (or set `OUTPUT_NAME "solix"`).
     - Update standard library copy commands, Catch2 test dependencies, and launcher target links.
     - Provide backward-compatible symbolic link or alias for `solix_launcher` during transition.
2. **Exit Code Propagation from VM to Shell**:
   - In [`language/include/solix/runtime.hpp`](language/include/solix/runtime.hpp) and [`language/src/runtime.cpp`](language/src/runtime.cpp):
     - Modify `run(RuntimeOptions &options)` to return `int32_t`.
     - In `op_HALT`:
       - If `main()` returns an integer value, pop the top operand from the stack and store it in `context.exit_code`.
       - If `main()` has a `void` return signature or the stack is empty, default `context.exit_code = 0`.
   - In [`launcher/src/commands/execute.cpp`](launcher/src/commands/execute.cpp):
     - Capture `int exit_code = run(*opts);`.
     - Propagate this code via `std::exit(exit_code)` or return it from the CLI handler.
     - Retain error exit code `1` when an unhandled exception bubbles up.
3. **Verification**:
   - Write integration tests verifying that `main()` returning `0`, `1`, `42`, and `-1` correctly yields `$? = 0`, `$? = 1`, `$? = 42`, and `$? = 255`.

---

## Phase 4: Lexer Scalar Character Literal & Type Discrimination [COMPLETED]

### Status: COMPLETED & MERGED TO MASTER

### Issue
In [`language/src/processes/lexer.cpp`](language/src/processes/lexer.cpp), single-quoted characters (`'a'`) are handled by `handle_string()` and emitted as `TokenType::STRING` with a `char[]` string payload. There is no scalar `char` literal token in the lexer.
Consequently:
- Declaring `char c = 'a';` triggers a compiler type mismatch error (`Expected char, got char[]`).
- Overloaded methods such as `fill(char[] target, char val)` cannot be resolved with literal arguments.
- Character escape sequences like `'\n'`, `'\t'`, and `'\0'` cannot be used as primitive char constants.

### Solution
1. **Lexer Character Literal Handling**:
   - In `lexer.cpp`, decouple single quotes from `handle_string()`.
   - Implement `handle_character()`:
     - Expect an opening `'`.
     - Parse the character or escape sequence (`\n`, `\t`, `\r`, `\\`, `\'`, `\0`, `\xHH`).
     - Require a closing `'`.
     - Emit `TokenType::CHAR` with primitive `uint64_t` scalar value storing the ASCII/UTF-8 codepoint.
2. **AST & Binder Integration**:
   - In `parser.cpp`, parse `TokenType::CHAR` into a `LiteralExpressionNode` tagged with primitive scalar type `char`.
   - In `binder.cpp`, register `char` literals as direct value matches for `char` assignments, comparisons, and method parameters.
3. **Verification**:
   - Unit tests covering `char c = 'x';`, escape sequences, comparisons (`c == '\n'`), and overloaded method calls distinguishing `print(char)` from `print(char[])`.

---

## Phase 5: Standard Library Architecture, Directory & Packaging Cleanup [COMPLETED]

### Status: COMPLETED & MERGED TO MASTER

### Issue
1. **Vague Directory Naming & Package Mismatch**:
   - `launcher/rsc/lib/solix/utilities/` contains core classes like `Exceptions.slx`, `Objects.slx`, `Arrays.slx`, `Optional.slx`, and `Result.slx`. However, these files declare `package solix;`, creating a mismatch between the filesystem path and package identifier.
2. **Directory Naming & Typo Inconsistencies**:
   - `launcher/rsc/lib/solix/Maths/` uses uppercase and plural naming, unlike all other packages.
   - `launcher/rsc/lib/solix/systems/Environement.slx` contains a misspelling (`Environement` instead of `Environment`).
3. **Zombie 0-Byte Stub Files**:
   - Seven standard library files are completely empty (0 bytes):
     - `systems/Environment.slx`, `systems/FileSystem.slx`, `systems/Process.slx`, `systems/Time.slx`
     - `math/Conversions.slx`, `math/Operations.slx`, `math/Random.slx`
   - The compiler's standard library importer recursively iterates through all `.slx` files, wasting cycles opening and tokenizing empty files.

### Solution
1. **Directory Restructuring**:
   - Rename `launcher/rsc/lib/solix/utilities/` to `launcher/rsc/lib/solix/core/`.
   - Rename `launcher/rsc/lib/solix/Maths/` to `launcher/rsc/lib/solix/math/`.
   - Rename `launcher/rsc/lib/solix/systems/Environement.slx` to `Environment.slx`.
2. **Package Naming Standardization**:
   - All classes in `launcher/rsc/lib/solix/core/` declare `package solix.core;` (with alias or root exposure in `solix;`).
   - All classes in `launcher/rsc/lib/solix/collections/` declare `package solix.collections;`.
   - All classes in `launcher/rsc/lib/solix/systems/` declare `package solix.systems;`.
   - All classes in `launcher/rsc/lib/solix/math/` declare `package solix.math;`.
3. **Clean Up Empty Stubs**:
   - Remove 0-byte stub files until their complete implementations are ready (in Phase 13), ensuring clean compiler stdlib discovery.
   - Update CMake `copy_stdlib` targets and compiler stdlib path search logic.

---

## Phase 6: Standard Library API Modernization & Algorithm Refactoring [COMPLETED]

### Status: COMPLETED & MERGED TO MASTER

### Issue
1. **Hardcoded Non-Generic `Objects.slx`**:
   - `Objects.require_non_null()` and `Objects.is_null()` only accept `String` and `char[]`. Passing user class instances or collections fails type checking.
2. **Cluttered Suffix Names in `Arrays.slx`**:
   - Methods use explicit type suffixes (`fill_i32`, `fill_char`, `swap_i32`, `sort_i32`). Sorting and binary search only exist for `int32[]`.
3. **$O(N)$ Collections Inefficiency**:
   - `Map<K, V>` and `HashSet<T>` perform linear scans over arrays using `==` pointer equality instead of hashing and bucketing.
4. **Lack of `import` Keyword**:
   - Every collection file must repeat multiple alias statements (`alias Exception = solix.Exception;`, etc.).

### Solution
1. **Generic `Objects.slx`**:
   - Refactor `Objects` to provide template methods:
     ```slx
     public static bool is_null<T>(T item);
     public static bool non_null<T>(T item);
     public static T require_non_null<T>(T item);
     public static T require_non_null_message<T>(T item, String message);
     public static bool equals<T>(T first, T second);
     public static int32 hash_code<T>(T item);
     ```
   - In `binder.cpp`, support member method template deduction on static class calls (`Objects.require_non_null(my_obj)`).
2. **Clean Overloaded & Generic `Arrays.slx`**:
   - Replace typed suffixes with method overloads:
     - `Arrays.fill(int32[] target, int32 val)`, `Arrays.fill(float64[] target, float64 val)`, etc.
   - Implement generic algorithms:
     - `Arrays.swap<T>(T[] array, int32 i, int32 j)`
     - `Arrays.reverse<T>(T[] array)`
     - `Arrays.copy<T>(T[] src, int32 src_off, T[] dst, int32 dst_off, int32 len)`
     - `Arrays.equals<T>(T[] a, T[] b)`
   - Implement sorting and binary search for `int64[]`, `float64[]`, and `char[]`.
3. **Bucketed `HashMap<K, V>` and `HashSet<T>`**:
   - Implement true bucketed hash tables using array-of-buckets with collision chaining.
   - Compute bucket indexes using `Objects.hash_code<K>(key) % capacity`.
   - Retain linear array implementations as `ArrayMap<K, V>` and `ArraySet<T>` for small memory-sensitive sets.
4. **Package Import Syntax**:
   - In `parser.cpp` and `binder.cpp`, support `import package.Symbol;` and `import package.*;` to eliminate redundant alias boilerplate across standard library files.

---

## Phase 7: Full & Partial Symbol Path Resolution & Flexible Namespace Disambiguation [COMPLETED]

### Status: COMPLETED & MERGED TO MASTER

### Issue
1. **Dotted Identifiers Rejected as Runtime Expressions**:
   - When referencing static methods or static properties via qualified paths (e.g. `solix.core.Objects.is_null(...)` or `core.Objects.is_null(...)`), the parser structures the callee as nested `MemberAccessExpression` chains. During semantic analysis in `Binder`, the root identifier (`"solix"` or `"core"`) is looked up as a variable in the local/class scope, failing with `Invalid identifier usage: solix` or `Undefined identifier: core`.
2. **Lack of Sub-Namespace Suffix Matching for Types**:
   - In `resolve_type()`, types are matched either by exact mangled name (`global_scope.resolve(raw_type.name)`) or by prefixing known root packages (`pkg + raw_type.name`). If a package is deep (e.g. `com.mycompany.service`) and the user writes a partial sub-namespace (e.g. `service.User` or `mycompany.service.User`), lookup fails because prepending `com.mycompany.service.` yields `com.mycompany.service.service.User`.
3. **Overly Eager Package Prepending on Base Classes (`extends`)**:
   - In `Binder::visit(ClassDeclaration &n)` during Pass 1a, `cls->base_class_name` was unconditionally prepended with `current_prefix`. Extending a class from another package using an unqualified or partial name (e.g. `class MyEx extends Exception` or `extends core.Exception`) mangled the name to `my_pkg.Exception` or `my_pkg.core.Exception`.
4. **Imports Order Sensitivity and Partial Package Imports**:
   - `import core.String;` or `import core.*;` failed to resolve `"core"` to the full package `"solix.core."` if the imported package was defined in a file parsed after the importing file, or if only a package suffix was specified.

### Solution
1. **Static Symbol Path Extraction (`extract_symbol_path`)**:
   - In `binder.cpp`, implement `extract_symbol_path(Node* node, std::string& path)` to reconstruct dotted/scoped paths (`a.b.c` or `a::b::c`) from nested `MemberAccessExpression` and `IdentifierNode` trees.
   - In `MethodCallExpression` and `MemberAccessExpression`, if the root identifier is not a local variable/parameter in `current_scope` and the extracted path resolves to a `ClassDeclaration` or `EnumDeclaration`, treat the target as a static class/enum access without attempting runtime expression evaluation of the package prefixes.
2. **Suffix-Based Symbol & Type Resolution (`resolve_symbol` & `resolve_template_name`)**:
   - Search order:
     1. Local variables, parameters, and current class members (for simple unqualified names).
     2. Explicit imports in `imported_symbols`.
     3. Active package (`node_pkg`).
     4. Exact match in `global_scope`.
     5. Registered `known_packages` prefixes.
     6. Flexible sub-namespace suffix matching across all registered classes, enums, and aliases (`cand_name.ends_with("." + name)`).
     7. Ambiguity detection: if multiple candidates from different packages match without an explicit import or local package priority, emit a clear diagnostic detailing all candidates and prompting disambiguation.
3. **Template Blueprint Suffix Resolution**:
   - Extend `resolve_template_name` to support suffix lookups in `template_registry`, enabling `collections.List<T>`, `solix.collections.List<T>`, and `List<T>`.
4. **Deferred Global Import Binding**:
   - Collect import statements during Pass 1a and bind them after all top-level symbols and packages are registered, supporting full paths, partial package prefixes, and wildcards reliably across compilation units.
5. **Class Inheritance Resolution**:
   - Defer base class resolution to `resolve_base_class` in Pass 2 using `resolve_symbol`, resolving `Exception`, `core.Exception`, and `solix.core.Exception` to canonical mangled names.
6. **Parser Support for Scope Resolution Operator `::`**:
   - Allow `::` in `parse_type_info()` and `parse_import_statement()` to seamlessly interoperate with C++ style namespaces (`solix::core::String`, `core::Objects::is_null`).

---

## Phase 8: Modernized Statement & Construct Test Suite Architecture [COMPLETED]

### Status: COMPLETED & MERGED TO MASTER

### Issue
The initial testing suite relied on legacy monolithic test files (`test_phase*.cpp`) and an ad-hoc integration file (`test.slx`), which lacked systematic, construct-by-construct coverage, granular diagnostic assertions, and isolated negative verification. A robust compiler requires a fine-grained, professional test framework where each language statement, declaration, module directive, and expression is tested for both positive execution guarantees and negative compile-time/runtime diagnostics.

### Solution
1. **Modernized Test Architecture**:
   - Reconfigure `tests/CMakeLists.txt` using recursive source discovery (`file(GLOB_RECURSE)`) and Catch2 test discovery.
   - Build a shared test harness in `tests/include/test_helper.hpp` providing in-memory compilation (`compile_source`), diagnostic inspection (`assert_compile_error`), and runtime execution (`run_source`).
2. **Granular Categorization**:
   - Organize test files matching construct specifications:
     - `tests/statements/modules/` (Alias, Import, Package)
     - `tests/statements/declarations/` (Class, Constructor, Enum, Field, Interface, Method, Operator)
     - `tests/statements/control_flow/` (Block, Break, Continue, DoWhile, ExpressionStatement, For, If, Return, Switch, Throw, TryCatchFinally, VariableDeclaration, While)
     - `tests/statements/expressions/` (ArrayAccess, ArrayCreation, ArrayLiteral, Assignment, Binary, Cast, Identifier, InstanceOf, Literal, MemberAccess, MethodCall, NewInstance, Ternary, Unary)
3. **Spec-Driven Traceability**:
   - Maintain `tests/statements/TESTS.md` with explicit status tags (`[NOT IMPLEMENTED]` vs `[IMPLEMENTED]`) mapped to each test case.
   - Implemented all 37 construct test suites covering all 99 scenarios (36 passed, 2 interface test cases failed as expected via `[!mayfail]`).

---

# Part II: Documentation & Knowledge Base

---

## Phase 9: Root Project Gateway & Onboarding (`README.md`)

### Issue
The repository currently lacks a top-level `README.md`. New contributors or developers exploring the project have no structured overview of language design principles, memory model, compilation pipeline, build prerequisites, or CLI quickstart commands.

### Solution
Author an industry-standard `README.md` at the project root containing:
1. **Language Overview & Philosophy**:
   - Statically typed, object-oriented language with generics and templates.
   - Deterministic Automatic Reference Counting (ARC) memory management without stop-the-world GC pauses.
   - High-performance register/stack hybrid bytecode virtual machine implemented in C++20.
2. **Architecture Pipeline Diagram**:
   - Visual Mermaid/ASCII flow: Source Code (`.slx`) $\to$ Lexer $\to$ Parser $\to$ AST $\to$ Binder (Passes 1–3) $\to$ Assembler $\to$ Bytecode (`.slxb`) $\to$ Runtime VM.
3. **Prerequisites & Toolchain Setup**:
   - Supported toolchains: Clang 14+, GCC 11+, MSVC 2022.
   - Build commands with CMake and Ninja (`cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`).
4. **CLI Quickstart Tutorial**:
   - Hello World example, compiling with `solix compile`, running with `solix run`.
   - Passing command-line arguments and verifying program exit code propagation.
5. **Key Language Features Summary**:
   - Representative code snippets: classes, single inheritance, `weak` reference fields, template methods, `try-catch-finally`, and standard `Console.println`.
6. **Project Directory Tour & Documentation Map**:
   - Guided map linking to `language/`, `launcher/`, `tests/`, `docs/spec/`, `docs/wiki/`, and `docs/guide/`.

---

## Phase 10: Formal Language & VM Lowering Specification (`docs/spec/`)

### Issue
The formal specification of the language is currently incomplete and partially misplaced in `docs/wiki/statements/`. A production language specification must rigorously define the lexical grammar, static semantics/type system, virtual machine instruction set (ISA), and AST-to-bytecode lowering rules for all grammar constructs.

### Solution
1. **Specification Directory Reorganization**:
   - Move `docs/wiki/statements/` $\to$ `docs/spec/statements/` to place the 37 compiler lowering and VM opcode specifications in their canonical formal location.
2. **`docs/spec/lexical.md` (Lexical Grammar Specification)**:
   - Character encoding: UTF-8 source representation.
   - Whitespace, newlines, and comment conventions (single-line `//`, block `/* ... */`).
   - Identifiers: naming rules, Unicode identifier boundaries, reserved keyword isolation.
   - Numeric Literals: decimal, hexadecimal (`0x`), binary (`0b`), integer width inference (`int8` through `uint64`), floating-point notation (`float32`, `float64`).
   - Character & String Literals: single-quoted scalar `char` (`'a'`, `'\n'`, `'\uXXXX'`), double-quoted `String` literals with escape sequences.
   - Full 39-keyword reserved terminal table.
3. **`docs/spec/types.md` (Type System & Static Semantics)**:
   - Primitive type hierarchy: numeric widening, narrowing conversions, explicit cast requirements.
   - Reference type semantics: classes, arrays (`T[]`), nullability, and reference equality.
   - Nominal subtyping rules, single inheritance DAG, and method override covariance/contravariance rules.
   - Template monomorphization: type substitution, deduction rules, and specialization precedence.
4. **`docs/spec/vm_isa.md` (Virtual Machine Instruction Set Architecture)**:
   - Complete opcode dictionary: opcode byte values, mnemonic names, immediate operand encoding.
   - Stack frame transitions: operand stack consumption (`pop`), operand production (`push`), and local register indexing.
   - Memory opcodes: `ALLOC_DYNAMIC`, `INC_REF`, `DEC_REF`, `WEAK_STORE`, `WEAK_LOAD`.
   - Control flow opcodes: `JUMP`, `JUMP_IF_FALSE`, `CALL`, `INVOKE_VIRTUAL`, `RETURN`.
   - Exception opcodes: `SETUP_TRY_BLOCK`, `TEARDOWN_TRY_BLOCK`, `THROW_EXCEPTION`.

---

## Phase 11: Developer Keyword Reference Wiki (`docs/wiki/keywords/`)

### Issue
Application developers need a quick-lookup lexicon explaining language keywords in plain English, providing syntax patterns, permitted declaration contexts, common pitfalls, and code examples without wading through compiler opcode lowering tables.

### Solution
Create `docs/wiki/keywords/` containing dedicated reference pages for all 39 reserved keywords:
1. **Master Keyword Index (`docs/wiki/keywords/README.md`)**:
   - Alphabetical index table with keyword, functional category, contextual flags, and 1-sentence summaries.
2. **Control Flow Keywords (`docs/wiki/keywords/control_flow/`)**:
   - `if.md`, `else.md`, `while.md`, `do.md`, `for.md`, `switch.md`, `case.md`, `default.md`, `break.md`, `continue.md`, `return.md`, `try.md`, `catch.md`, `finally.md`, `throw.md`.
3. **Type & Declaration Keywords (`docs/wiki/keywords/declarations/`)**:
   - `class.md`, `interface.md`, `enum.md`, `package.md`, `import.md`, `alias.md`.
4. **Access & Storage Modifiers (`docs/wiki/keywords/modifiers/`)**:
   - `public.md`, `private.md`, `protected.md`, `internal.md`, `static.md`, `inline.md`, `native.md`, `const.md`, `virtual.md`, `override.md`, `weak.md`, `abstract.md`.
5. **Expression & Operator Keywords (`docs/wiki/keywords/expressions/`)**:
   - `new.md`, `super.md`, `instanceof.md`, `operator.md`, `extends.md`, `implements.md`.
6. **Standard Keyword Template**:
   - Each keyword file follows a uniform schema:
     - 1. Summary & Classification
     - 2. Permitted Contexts (Syntax & Grammar)
     - 3. Semantics & Compiler Rules
     - 4. Code Examples (Basic & Idiomatic)
     - 5. Common Pitfalls & Compiler Diagnostics
     - 6. Related Keywords & Guides

---

## Phase 12: Developer Guides, Standard Library API & Architecture Internals

### Issue
To support both application developers writing Solix software and systems engineers maintaining the Solix VM/compiler, documentation must cover progressive user guides, complete standard library API references, and internal compiler pipeline deep dives.

### Solution
1. **Part A: Progressive Developer Guide (`docs/guide/`)**:
   - `01_getting_started.md`: Toolchain installation, `solix compile`, `solix run`, project layout, CLI options.
   - `02_variables_and_types.md`: Primitives (`int8`–`uint64`, `float32`, `float64`, `char`, `bool`), arrays (`T[]`), lexical scopes, immutability (`const`).
   - `03_control_flow.md`: Conditionals, `switch` pattern matching, loops (`while`, `for`, `do-while`), loop control (`break`, `continue`).
   - `04_classes_and_oop.md`: Constructors, instance vs. static members, inheritance (`extends`), polymorphism (`override`), abstract contracts.
   - `05_memory_management_and_arc.md`: Deterministic ARC mechanics, LIFO destruction order, circular reference hazards, and cycle-breaking with `weak`.
   - `06_error_handling.md`: Exception hierarchy, `try-catch-finally` guarantees, throwing exceptions, custom exception classes.
2. **Part B: Standard Library Reference Wiki (`docs/wiki/stdlib/`)**:
   - **`core/`**:
     - `String.md` & `StringBuilder.md`: String immutability, UTF-8 indexing, string concatenation, and mutable buffer manipulation.
     - `Exceptions.md`: Hierarchy of built-in exceptions (`Exception`, `RuntimeException`, `NullPointerException`, `IndexOutOfBoundsException`, `TypeCastException`).
     - `Optional.md` & `Result.md`: Monadic error/value handling without exceptions.
     - `Arrays.md` & `Objects.md`: Core utility methods (array copying, equality, hash codes).
   - **`collections/`**:
     - `List.md`, `LinkedList.md`, `Stack.md`, `Queue.md`, `Deque.md`: Linear collections and operational complexities.
     - `Map.md`, `HashMap.md`, `ArrayMap.md`: Key-value associations, hash table performance, lookup semantics.
     - `Set.md`, `HashSet.md`, `ArraySet.md`: Unique element sets.
     - `Pair.md`: Generic 2-tuple utility.
   - **`systems/`**:
     - `Console.md`: Standard input, output, formatting, and console logging.
3. **Part C: Compiler & VM Architecture Internals (`docs/architecture/`)**:
   - `pipeline.md`: Complete compilation pipeline flow (Lexer $\to$ Parser $\to$ Binder Passes 1a/1b/2/3 $\to$ Assembler $\to$ VM).
   - `binder_passes.md`: Detailed semantics of each binder pass (Pass 1a global symbols, Pass 1b member registration, Pass 2 type frames, Pass 3 monomorphization and ARC insertion).
   - `arc_internals.md`: Object header layout, reference counter increment/decrement sequences, weak reference registry, and heap allocation.
   - `exception_unwinding.md`: Bytecode try/catch tables, dynamic handler lookup, call frame stack unwinding, and `finally` trampoline guarantees.

---

# Part III: New Features & System Capabilities

---

## Phase 13: Object-Oriented Primitive & Array Boxed Wrappers

### Issue
In Solix, primitive types (`int32`, `float64`, etc.) and raw arrays (`int32[]`) are primitive values and cannot be treated as first-class objects. They lack object-oriented methods like `to_string()`, `parse()`, `hash_code()`, `equals()`, and cannot be directly stored in collections that require object reference types without ad-hoc conversion.

### Solution
1. **Scalar Primitive Wrappers (`solix.core`)**:
   - Implement boxed wrapper classes:
     - `Bool`, `Char`
     - Signed integers: `Int8`, `Int16`, `Int32`, `Int64`
     - Unsigned integers: `UInt8`, `UInt16`, `UInt32`, `UInt64`
     - Floating point: `Float32`, `Float64`
   - Common features on all scalar wrappers:
     - Boxing constructor: `new Int32(42)` and unboxing accessor `int32 get_value()`.
     - Static parsing methods: `Int32.parse(String str)`.
     - Conversion methods: `to_string()`, `to_int64()`, `to_float64()`.
     - Standard equality and hashing: `bool equals(Object other)`, `int32 hash_code()`, `int32 compare_to(T other)`.
     - Static constants: `MIN_VALUE`, `MAX_VALUE`, `BYTES`, `BITS`.
2. **Object-Oriented Array Wrapper (`Array<T>`)**:
   - Implement `Array<T>` wrapping raw `T[]`:
     - Capacity and size queries: `int32 length()`, `bool is_empty()`.
     - Bounds-checked access: `T get(int32 index)`, `void set(int32 index, T value)`.
     - Algorithms: `void fill(T value)`, `void reverse()`, `Array<T> slice(int32 from, int32 to)`.
     - Conversions: `T[] to_raw()`, `static Array<T> from_raw(T[] raw)`.
3. **Verification**:
   - Comprehensive tests validating boxing/unboxing, parsing validity, out-of-bounds error handling, and collection storage.

---

## Phase 14: Core System & Mathematics Standard Library Modules

### Issue
Seven critical standard library modules exist only as empty 0-byte stubs or are missing essential OS, I/O, process, and mathematical operations:
- `systems/Environment.slx`, `systems/FileSystem.slx`, `systems/Process.slx`, `systems/Time.slx`
- `math/Operations.slx`, `math/Conversions.slx`, `math/Random.slx`

### Solution
Implement complete, robust APIs backed by native C++ runtime bridges in `language/src/natives/`:
1. **`solix.systems.Environment`**:
   - `String get_variable(String name)`, `bool has_variable(String name)`.
   - `String current_directory()`, `String os_name()`, `int32 processor_count()`.
   - `void exit(int32 status)`.
2. **`solix.systems.FileSystem`**:
   - `bool exists(String path)`, `bool is_file(String path)`, `bool is_directory(String path)`.
   - `String read_text(String path)`, `void write_text(String path, String content)`.
   - `void delete_file(String path)`, `void create_directory(String path)`.
   - `int64 file_size(String path)`.
3. **`solix.systems.Process`**:
   - `int32 execute(String command)`, `String execute_capture(String command)`.
   - `int32 get_pid()`.
4. **`solix.systems.Time`**:
   - `int64 now_millis()`, `int64 now_nanos()`.
   - `void sleep_millis(int64 millis)`.
5. **`solix.math.Operations`**:
   - `abs`, `min`, `max`, `floor`, `ceil`, `round`, `sqrt`, `pow`, `log`, `sin`, `cos`, `tan`.
   - Constants: `PI = 3.141592653589793`, `E = 2.718281828459045`.
6. **`solix.math.Conversions`**:
   - Radix conversions (`to_binary_string`, `to_hex_string`, `from_hex`).
   - Degree/radian conversions.
7. **`solix.math.Random`**:
   - Pseudo-random number generator (xoshiro256** or Mersenne Twister):
     - `int32 next_int()`, `int32 next_int_bounded(int32 bound)`.
     - `float64 next_float()`, `bool next_bool()`.
     - `void set_seed(int64 seed)`.

---

## Phase 15: Dedicated Performance Benchmarking Suite (`benchmarks/`)

### Issue
Performance validation is currently performed via ad-hoc scripts in `scratch_bench/`. There is no version-controlled, automated benchmarking framework to track VM execution speed, ARC overhead, and compiler throughput across commits or against other language runtimes.

### Solution
1. **Dedicated Directory Structure (`benchmarks/`)**:
   - Build a formal benchmarking suite:
     - `benchmarks/micro/`: Focused tight-loop benchmarks.
       - `alu_loop.slx`: Pure opcode dispatch overhead (`int32`, `int64`, `float64`).
       - `function_calls.slx`: Direct method calls, virtual vtable dispatch, and native call transitions.
       - `arc_allocation.slx`: Heap churn, allocation latency, and retain/release cycles.
       - `array_access.slx`: Bounds-checked sequential and random array indexing.
       - `exceptions.slx`: Try-catch setup overhead and stack unwinding latency.
       - `string_concat.slx`: Immutable string concatenation vs. `StringBuilder`.
     - `benchmarks/macro/`: Real-world algorithmic workloads.
       - `fibonacci.slx`: Recursive Fibonacci ($n = 35$).
       - `prime_sieve.slx`: Sieve of Eratosthenes ($1,000,000$ numbers).
       - `matrix_mult.slx`: $256 \times 256$ floating-point matrix multiplication.
       - `collections_churn.slx`: Inserting, searching, and sorting $100,000$ items in `List`, `HashMap`, and `HashSet`.
2. **Automated Multi-Language Benchmark Runner**:
   - Implement a benchmark driver script (`benchmarks/run_benchmarks.py`) executing 5 timed runs, calculating statistical mean, median, standard deviation, and peak memory usage.
   - Include comparison harnesses for CPython 3, PUC Lua 5.4, LuaJIT, and native C++20.
   - Support generating markdown comparison tables and JSON performance logs.

---

## Phase 16: Developer Tooling: Project Manifest & Build System (`solix.toml`)

### Issue
Compiling Solix projects requires specifying individual source files via the command line (`solix compile src/a.slx src/b.slx ...`). There is no project-level configuration file to specify dependencies, package names, compilation targets, or compiler flags.

### Solution
1. **Declarative Project Manifest (`solix.toml`)**:
   - Define project configuration schema:
     ```toml
     [package]
     name = "my_app"
     version = "0.1.0"
     authors = ["Author <author@example.com>"]
     license = "MIT"

     [build]
     entry = "src/main.slx"
     output = "bin/my_app.slxb"
     stdlib = true
     log_level = "WARN"

     [dependencies]
     # Future package dependencies
     ```
2. **CLI Project Management Subcommands**:
   - In `launcher/src/main.cpp`:
     - `solix new <project_name>`: Scaffolds a new project directory with `solix.toml`, `src/main.slx`, `.gitignore`, and `tests/`.
     - `solix build`: Parses `solix.toml`, discovers all `.slx` source files recursively in `src/`, compiles them, and outputs to the configured binary path.
     - `solix run [args...]`: Builds (if sources are out of date) and runs the target application.
     - `solix test`: Discovers and executes all test scripts in `tests/`.
     - `solix clean`: Removes build outputs and cached `.slxb` files.

---

## Phase 17: Developer Tooling: Language Server Protocol (LSP) & Editor Extension

### Issue
Solix lacks IDE developer tooling. Developers editing `.slx` files have no syntax highlighting, real-time diagnostic errors, hover information, or go-to-definition in editors like VS Code, Cursor, or Neovim.

### Solution
1. **Language Server Protocol Subcommand (`solix lsp`)**:
   - Add `solix lsp` subcommand implementing the JSON-RPC Language Server Protocol over standard input/output.
   - Features:
     - `textDocument/didOpen`, `textDocument/didChange`: Re-run lexer, parser, and binder on in-memory buffers in real-time.
     - `textDocument/publishDiagnostics`: Push syntax errors, semantic type mismatches, and warnings directly to the editor.
     - `textDocument/hover`: Display variable types, method signatures, parameter names, and docstring comments on hover.
     - `textDocument/definition`: Jump from identifiers to class declarations, field declarations, and method definitions across packages.
     - `textDocument/documentSymbol`: Generate document outlines displaying classes, methods, and member fields.
2. **VS Code Extension Package**:
   - Create a VS Code extension under `editors/vscode/`:
     - Syntax highlighting grammar (`solix.tmLanguage.json`) covering all keywords, types, literals, and comments.
     - Language configuration (bracket matching, auto-closing pairs, comment toggles).
     - LSP client integration pointing to the `solix` executable (`solix lsp`).

---

## Phase 18: Functional Programming: Lambdas, Closures & First-Class Functions

### Issue
Solix lacks first-class functions, anonymous closures, and lambda expressions. Developers cannot write functional code or use higher-order functions like `list.map(...)`, `list.filter(...)`, or custom comparators `sort_by(...)`.

### Solution
1. **Language Syntax**:
   - Lambda expression syntax:
     - Concise: `(x) => x * 2`
     - Typed: `(int32 a, int32 b) => a + b`
     - Block body: `(item) => { Console.println(item); return true; }`
   - Function type signatures:
     - `Func<T, R>`, `Action<T>`, `Predicate<T>`, or `(int32, int32) -> bool`.
2. **Compiler Architecture**:
   - **Lexer**: Add `=>` operator token (`TokenType::OPERATOR_ARROW`).
   - **Parser**: Parse `LambdaExpressionNode` with parameter lists and expression/block bodies.
   - **Binder**:
     - Variable capture analysis: detect outer local variables referenced inside the lambda.
     - Closure synthesis: generate an internal anonymous class implementing an invoker interface (`invoke(...)`), capturing local variables as member fields.
     - Type inference: infer parameter and return types from context.
   - **Assembler & VM**:
     - Emit closure instantiation bytecode, storing captured environment variables.
     - Add `CALL_CLOSURE` opcode to invoke synthesized closure instances.
3. **Standard Library Integration**:
   - Add higher-order methods:
     - `List<T>`: `map()`, `filter()`, `reduce()`, `for_each()`, `any()`, `all()`.
     - `Optional<T>`: `map()`, `flat_map()`, `filter()`.
     - `Arrays`: `sort_by<T>(T[] array, (T, T) -> int32 comparator)`.

---

## Phase 19: Multithreading & Concurrency Runtime

### Issue
The Solix runtime VM is currently single-threaded. There are no language constructs or runtime threading mechanisms to utilize multi-core processors, spawn threads, or synchronize shared resources.

### Solution
1. **Thread-Safe Runtime VM Architecture**:
   - **Thread-Safe ARC**:
     - Replace raw uint32 reference counters in object headers with `std::atomic<uint32_t>` when concurrency is enabled.
     - Introduce atomic retain and release operations in bytecode.
   - **Per-Thread Stacks & Execution Contexts**:
     - Allocate isolated operand stacks and call frames for each thread.
     - Make heap memory allocation thread-safe using thread-local allocation blocks (TLAB) or fine-grained allocator mutexes.
2. **Standard Library Concurrency API (`solix.threading`)**:
   - **`Thread` Class**:
     - `Thread.spawn(() => { ... })` or `new Thread(runnable).start()`.
     - Methods: `join()`, `detach()`, `sleep(int64 millis)`, `yield()`, `is_alive()`, `id()`.
   - **Synchronization Primitives**:
     - `Mutex` / `Lock`: `acquire()`, `release()`, `try_acquire()`.
     - `ConditionVariable`: `wait(mutex)`, `notify_one()`, `notify_all()`.
     - `Atomic<T>`: Lock-free atomic integers and booleans (`load()`, `store()`, `compare_and_set()`, `fetch_add()`).
3. **Compiler Integration**:
   - Wire the existing `--multithreaded` compiler option to trigger atomic ARC bytecode emission and multithreaded runtime support.

---

## Phase 20: Compiler & VM Optimizations: Bytecode Optimizer & Profiler

### Issue
1. **Unoptimized Bytecode Sequences**:
   - The assembler currently emits unoptimized instruction streams with redundant load/store operations, non-folded constant expressions, and unmerged instructions.
2. **Lack of Runtime Memory Visibility & Leak Detection**:
   - Developers have no runtime tooling to measure heap churn, identify memory allocation hotspots, or track down circular reference leaks caused by missing `weak` references.

### Solution
1. **Assembler Bytecode Peephole Optimizer**:
   - Implement an optimization pass in `assembler.cpp` running prior to bytecode serialization:
     - **Constant Folding**: Evaluate arithmetic on constant literals at compile-time (e.g., `PUSH_CONST_I32 2` + `PUSH_CONST_I32 3` + `ADD_I64` $\to$ `PUSH_CONST_I32 5`).
     - **Redundant Load/Store Elimination**: Replace adjacent `SET_LOCAL x; GET_LOCAL x;` patterns with `DUP; SET_LOCAL x;`.
     - **Instruction Specialization**:
       - `PUSH_CONST_I32 1; ADD_I64` $\to$ `INC_I64`.
       - `PUSH_CONST_I32 1; SUB_I64` $\to$ `DEC_I64`.
       - `PUSH_CONST_I32 0; EQ_I64` $\to$ `IS_ZERO_I64`.
     - **Dead Code Elimination**: Strip unreachable instructions following unconditional `RETURN`, `THROW`, or `JMP`.
2. **Memory Profiler & ARC Leak Detector**:
   - Add runtime CLI flags: `--profile-mem` and `--trace-gc`.
   - **Heap Tracking**: Track total bytes allocated, active object count, and peak memory usage.
   - **Circular Reference & Leak Detection**:
     - At program termination, inspect the heap. If objects remain allocated with non-zero reference counts, report memory leak diagnostics:
       ```
       [ARC Leak Detector] 2 uncollected objects detected at program shutdown:
         - Address 0x0042: Class 'Node' (ref_count: 1)
         - Address 0x0048: Class 'Node' (ref_count: 1)
         Likely circular reference cycle. Consider marking parent reference 'weak'.
       ```
   - **Allocation Heatmaps**: Log top allocation call sites to guide user-level and stdlib optimizations.

---

## Phase 21: Runnable Examples Showcase (`examples/`)

### Issue
The repository lacks clean, runnable sample programs demonstrating language features to new users. Existing sample code is scattered across internal test cases and scratch files.

### Solution
1. **Curated Showcase Structure (`examples/`)**:
   - **`01_basics/`**:
     - `hello_world.slx`: Console output, reading command-line arguments, program exit codes.
     - `primitives.slx`: Numeric types, chars, booleans, arithmetic, and array literals.
     - `control_flow.slx`: `if-else`, loops (`while`, `for`, `do-while`), `switch-case`.
     - `functions.slx`: Static functions, default arguments, and recursion.
   - **`02_oop/`**:
     - `classes.slx`: Encapsulation, constructors, member fields, `this`.
     - `inheritance.slx`: `extends`, `virtual`, `override`, `super` method dispatch.
     - `operator_overloading.slx`: Custom `operator+`, `operator==`, and `operator=`.
   - **`03_generics/`**:
     - `generic_box.slx`: Defining and using class templates (`Box<T>`).
     - `generic_methods.slx`: Method templates and implicit type deduction.
   - **`04_exceptions/`**:
     - `try_catch.slx`: Catching exceptions and guaranteed execution in `finally`.
     - `custom_exceptions.slx`: Subclassing `Exception` for domain-specific errors.
   - **`05_stdlib/`**:
     - `collections_demo.slx`: `List<T>`, `HashMap<K, V>`, `HashSet<T>`, `Stack<T>`, `Queue<T>`.
     - `optional_result_demo.slx`: Error handling with `Optional<T>` and `Result<T, E>`.
   - **`06_applications/`**:
     - `calculator.slx`: Interactive expression calculator.
     - `algorithms.slx`: Sorting algorithms (quicksort, merge sort) and binary search.
2. **Automated Runner Script**:
   - Provide an executable runner script (`examples/run_all.sh`) or CMake target (`make run_examples`) that compiles and executes every example to guarantee ongoing compatibility.
