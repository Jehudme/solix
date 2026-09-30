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

## Phase 9: Root Project Gateway & Onboarding (`README.md`) [COMPLETED]

### Status: COMPLETED

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

## Phase 10: Formal Language & VM Lowering Specification (`docs/spec/`) [COMPLETED]

### Status: COMPLETED

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

## Phase 11: Developer Keyword Reference Wiki (`docs/wiki/keywords/`) [COMPLETED]

### Status: COMPLETED

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

## Phase 12: Developer Guides, Standard Library API & Architecture Internals [COMPLETED]

### Status: COMPLETED

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

---

## Phase 22: Type & Instance Sizing: `sizeof` Operator & Memory Introspection [COMPLETED]

### Status: COMPLETED

### Issue
Solix lacks a `sizeof` operator or memory introspection mechanism. Developers writing low-level systems code, binary serialization libraries, network buffers, or performance-critical data structures have no way to determine the byte size occupied by primitive types or dynamically allocated class instances.

### Technical Feasibility Analysis
**Yes, implementing `sizeof` is completely possible and directly aligns with Solix's architecture:**
1. **Compile-Time Primitive Sizing**: Primitive types in Solix have fixed bit-widths defined by the specification (`int8`/`uint8`/`bool`/`char` = 1 byte, `int16`/`uint16` = 2 bytes, `int32`/`uint32`/`float32` = 4 bytes, `int64`/`uint64`/`float64` = 8 bytes).
2. **Compile-Time Class Instance Sizing**: The semantic binder (`binder.cpp`) already calculates each class's layout and stores it in `ClassDeclaration::instance_size`. In the Solix VM, each instance field and the vtable pointer occupies an 8-byte word. Thus, an instance's payload size is deterministically `instance_size * 8` bytes.
3. **Runtime Dynamic Sizing**: Solix heap allocations (`memory.dynamic_allocation`) store an object header word immediately before the payload at `heap[address - 1]`. Bits 32..63 of this header store the allocated block capacity in 64-bit words (`blk_size`), allowing the runtime to inspect the allocated footprint of any live object reference or array at $O(1)$ cost.

### Solution
1. **Lexer & Parser**:
   - Add `TokenType::KEYWORD_SIZEOF` (`"sizeof"`).
   - In `parser.cpp`, parse `SizeOfExpression` accepting both type identifiers and expressions: `sizeof(int32)`, `sizeof(Point)`, or `sizeof(myInstance)`.
2. **Binder & Semantic Analysis**:
   - In `binder.cpp`, resolve target operand:
     - **Type Argument (`sizeof(Type)`)**:
       - Primitive types: Return constant byte size (1, 2, 4, 8).
       - Class types: Resolve `ClassDeclaration`, compute `instance_size * 8` bytes (including vtable pointer and member fields).
       - Fold into compile-time constant integer expression of type `int32`.
     - **Expression Argument (`sizeof(expr)`)**:
       - Type-check the operand expression. If primitive or statically resolved class, fold or mark for runtime opcode resolution.
   - Return type is `int32`.
3. **Assembler & Runtime VM**:
   - **Compile-Time Folding**: Emit `PUSH_CONST_I32 <bytes>` directly for compile-time determinable types, avoiding runtime overhead.
   - **Dynamic Evaluation**: For dynamic instance evaluation `sizeof(instance)`:
     - Introduce `OpCode::SIZEOF`.
     - In `runtime.cpp`, pop object reference address from stack. Check for null (throw `NullPointerException` or return `0`).
     - Read word capacity from `heap[address - 1] >> 32` and multiply by 8 bytes to return the total allocated memory footprint.
4. **Standard Library Integration**:
   - In `solix.core.Objects`, provide helper method:
     ```solix
     public static int32 size_of<T>(T instance) {
         return sizeof(instance);
     }
     ```
5. **Verification**:
   - Catch2 unit tests in `tests/statements/expressions/test_sizeof_expression.cpp`:
     - Constant folding for all primitive types (`sizeof(int8) == 1`, `sizeof(int32) == 4`, `sizeof(float64) == 8`).
     - Class instance byte size calculation with single and inherited fields (`sizeof(EmptyClass) == 8` for vtable word, classes with $N$ fields).
     - Dynamic instance sizing on heap objects (`Point p = new Point(); sizeof(p)`).
     - Spec documentation update in `docs/spec/` and `tests/statements/TESTS.md`.

---

## Phase 23: Function Call Architecture Modernization & Callee Frame Allocation (`ALLOC_FRAME`)

### Status: PLANNED

### Issue & Architectural Motivation
1. **Inverted Caller/Callee Responsibility (Leaky Abstraction)**:
   - In Solix's current VM instruction set, the **caller** is responsible for specifying the **callee's internal frame size**:
     - Direct calls push: `PUSH target_ip`, `PUSH frame_size`, `PUSH arg_count`, followed by `CALL`.
     - Virtual calls embed: `CALL_VIRTUAL vtable_slot, frame_size, arg_count`.
   - A function's frame size (the number of local variable slots needed during execution) is an internal implementation detail of the callee. Exposing it to callers creates significant structural defects.
2. **Hidden Stack Corruption Bug in Virtual Method Dispatch (`CALL_VIRTUAL`)**:
   - In [`language/src/runtime.cpp`](language/src/runtime.cpp), `CALL_VIRTUAL` reads `frame_size` encoded at the callsite. The compiler calculates this `frame_size` from the *base class or interface declaration*.
   - If a derived subclass overrides the method and requires more local variables than the base method, the caller allocates an insufficient frame. When the derived method writes to its local variables, it writes past the allocated frame, corrupting stack memory and adjacent call frames.
3. **Callsite Bytecode Bloat**:
   - Every single function call site redundantly emits `PUSH_CONST_I32 <frame_size>`. In real-world codebases with thousands of callsites, this substantially inflates bytecode binary size (`.slxb`).
4. **Architectural Barrier to Function Pointers & Lambda Closures**:
   - **Function Pointers**: For indirect calls (`fp()`), the caller cannot know which concrete function will be executed, making caller-specified frame sizing impossible without packing metadata or creating complex descriptor structures.
   - **Lambda Captures**: When generating synthetic functions for lambdas that capture outer variables, the lambda function can simply declare a larger frame size in its prologue to allocate local slots for its captured variables, without any caller having to be aware of the capture footprint.

---

### Solution Architecture

```
Current Flow (Caller-Dictated Frame):
  Caller: PUSH target_ip -> PUSH frame_size -> PUSH arg_count -> CALL
  Callee: (starts immediately with function statements)

Modernized Flow (Callee Prologue Frame):
  Caller: PUSH target_ip -> PUSH arg_count -> CALL (or CALL target_ip, arg_count)
  Callee: ALLOC_FRAME <frame_size> -> (function statements) -> RETURN
```

---

### Implementation Blueprint

#### 1. New Opcode & Instruction Set Updates
* **`OpCode::ALLOC_FRAME`**:
  * **Opcode Value**: Assign next available opcode in [`language/src/utilities/optcodes.hpp`](language/src/utilities/optcodes.hpp).
  * **Format**: `ALLOC_FRAME <uint32_t frame_size>` (inline 4-byte immediate payload).
  * **VM Execution Semantics** in [`language/src/runtime.cpp`](language/src/runtime.cpp):
    ```cpp
    op_ALLOC_FRAME:
    {
        uint32_t frame_size = read_u32(bytecode, program_counter);
        uint32_t arg_count = call_stack[call_depth - 1].arg_count;
        if (frame_size > arg_count) {
            sp += (frame_size - arg_count); // Reserve stack slots for local variables
        }
        DISPATCH();
    }
    ```
* **Refactor `OpCode::CALL`**:
  * Remove `frame_size` from stack requirements.
  * `CALL` now only consumes `target_ip` and `arg_count`:
    ```cpp
    op_CALL:
    {
        uint32_t arg_count = static_cast<uint32_t>(POP());
        uint32_t target_ip = static_cast<uint32_t>(POP());
        uint32_t current_sp_idx = static_cast<uint32_t>(sp - stack);
        uint32_t new_frame_pointer = current_sp_idx - arg_count;

        if (call_depth >= 65536)
            throw std::runtime_error("Stack overflow: max call depth exceeded");

        call_stack[call_depth++] = Frame(program_counter, new_frame_pointer, arg_count);
        program_counter = target_ip;
        DISPATCH();
    }
    ```
* **Refactor `OpCode::CALL_VIRTUAL`**:
  * Remove `frame_size` operand.
  * Format becomes: `CALL_VIRTUAL <uint32_t slot> <uint32_t arg_count>`.
  * The VM resolves `target_ip` from the object's vtable, creates the return `Frame`, and jumps directly to `target_ip`. The invoked target method executes its own `ALLOC_FRAME`, automatically sizing the stack according to the concrete subclass implementation.
* **Native Function Calls (`OpCode::CALL_NATIVE`)**:
  * Native C++ functions registered in `NativeRegistry` execute host code rather than bytecode; they do not require an `ALLOC_FRAME` instruction.
  * Verify that native function calls continue to pop their arguments cleanly without interacting with bytecode activation frames.

#### 2. Runtime Frame Structure & Return Semantics
* In [`language/include/solix/runtime.hpp`](language/include/solix/runtime.hpp), update `struct Frame`:
  ```cpp
  struct Frame {
      uint32_t return_address = 0;
      uint32_t frame_pointer = 0;
      uint32_t arg_count = 0;
      Frame() = default;
      Frame(uint32_t ret, uint32_t fp, uint32_t args)
          : return_address(ret), frame_pointer(fp), arg_count(args) {}
  };
  ```
* In `op_RETURN`:
  * Ensure stack restoration (`sp = stack + frame.frame_pointer`) cleanly collapses all local slots allocated by `ALLOC_FRAME`.

#### 3. Compiler Assembler Modernization ([`language/src/processes/assembler.cpp`](language/src/processes/assembler.cpp))
* **Function Prologue Emission**:
  * In `Assembler::visit(MethodDeclaration &)` and `Assembler::visit(ConstructorDeclaration &)`:
    * Emit `OpCode::ALLOC_FRAME` as the very first instruction before any body statements or field initializers:
      ```cpp
      emit_byte(static_cast<uint8_t>(OpCode::ALLOC_FRAME));
      emit_int32(function->frame_size);
      ```
* **Callsite Emission Cleanup**:
  * In `compile_method_call()`, remove:
    ```cpp
    // REMOVE: emit_byte(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
    // REMOVE: emit_int32(frame_size);
    ```
  * In `CALL_VIRTUAL` emission, omit `frame_size` operand.
  * In binary/assignment operator overloads (`operator+`, `operator=`), remove `PUSH_CONST_I32 frame_size`.
  * In constructor invocation (`new MyClass()`), remove `PUSH_CONST_I32 ctor->frame_size`.
  * In entry point invocation within `compile_boot_sequence()`, remove `PUSH_CONST_I32 entry_method->frame_size`.

#### 4. Disassembler & Tooling Updates
* In `Assembler::disassemble()` and debug loggers, format `ALLOC_FRAME`:
  ```text
  000420:  ALLOC_FRAME         size=4
  ```
* Remove `frame_size` from `CALL_VIRTUAL` disassembly formatting.

#### 5. Verification & Test Plan
* **Polymorphic Virtual Call Test**:
  * Base class `Base { public virtual int32 compute() { return 1; } }` (0 extra locals).
  * Derived class `Derived extends Base { public override int32 compute() { int32 a = 10, b = 20, c = 30; return a + b + c; } }` (3 extra locals).
  * Invoke `Base b = new Derived(); b.compute();`. Verify clean execution without stack corruption.
* **Recursion & Deep Call Stacks**:
  * Verify Fibonacci and factorial recursive functions run correctly, proving `ALLOC_FRAME` and `RETURN` maintain perfect stack balance across thousands of frames.
* **Catch2 Regression Suite**:
  * Run all 38 existing test suites (`ctest`) to ensure 100% backward compatibility across expressions, control flow, declarations, and modules.

---

## Phase 24: Primitive Function Pointers: `<return_type>(*)(<arguments_types>)`

### Status: PLANNED (Depends on Phase 23)

### Architectural Overview
Solix introduces primitive, zero-overhead **Function Pointers** using a modern adaptation of C-style function pointer syntax. 

In traditional C/C++, function pointer syntax embeds the variable name in the middle (`int (*name)(int, int)`), which makes syntax parsing and readability notoriously difficult. In Solix, the type syntax is cleanly separated from the variable name:
$$\text{Type: } \texttt{<return\_type>(*)(<arguments\_types>)} \quad\quad \text{Variable: } \texttt{<name>}$$

Function pointer variables are treated as primitive values. At compile time, the compiler strictly enforces argument types, argument counts, and return types. At runtime, a function pointer is stored as a single 64-bit integer (`uint64_t`) where the lower 32 bits represent the bytecode instruction pointer (`target_ip`), and the upper 32 bits store the closure environment address (`0` for stateless functions, populated in Phase 25 for capturing closures).

---

### Language Syntax & Ergonomics

#### 1. Declaration & Null Initialization
```solix
// Type: int32(*)(int32, int32)   Name: op
int32(*)(int32, int32) op = null;

// Parameterless with void return
void(*)() onComplete = null;

// Multi-argument with reference types
bool(*)(solix.core.String, int32) validator = null;
```

#### 2. Taking the Address of a Function
Assigning a function name (without parentheses) takes its address:
```solix
public class MathUtils {
    public static int32 add(int32 a, int32 b) {
        return a + b;
    }
}

// Statically type-checked: arguments (int32, int32) and return int32 must match
int32(*)(int32, int32) op = MathUtils.add;
```

#### 3. Calling Through a Function Pointer
Invocation uses standard function call syntax:
```solix
int32 sum = op(10, 20); // Evaluates to 30
```

#### 4. Type Aliasing (`alias`)
Function pointer types can be aliased to create reusable, clean type names:
```solix
alias BinaryOp = int32(*)(int32, int32);
alias Callback = void(*)();
alias Predicate = bool(*)(solix.core.String);

BinaryOp op = MathUtils.add;
int32 result = op(5, 7);
```

#### 5. Higher-Order Functions (Passing and Returning Function Pointers)
```solix
public static int32 compute(int32 x, int32 y, int32(*)(int32, int32) operation) {
    return operation(x, y);
}

public static int32(*)(int32, int32) getOperation() {
    return MathUtils.add;
}
```

---

### Implementation Blueprint

#### 1. Type Representation ([`language/src/utilities/statements.hpp`](language/src/utilities/statements.hpp))
Extend `TypeInfo` to represent function pointer signatures:
```cpp
struct TypeInfo {
    std::string name;
    int array_depth = 0;
    std::vector<TypeInfo> type_args;

    // Function Pointer Extension
    bool is_function_pointer = false;
    std::shared_ptr<TypeInfo> return_type = nullptr;
    std::vector<TypeInfo> param_types;

    std::string to_string() const {
        if (is_function_pointer) {
            std::string res = return_type ? return_type->to_string() : "void";
            res += "(*)(";
            for (size_t i = 0; i < param_types.size(); ++i) {
                res += param_types[i].to_string();
                if (i + 1 < param_types.size()) res += ", ";
            }
            res += ")";
            return res;
        }
        // ... existing type formatting
    }

    bool operator==(const TypeInfo &other) const {
        if (is_function_pointer != other.is_function_pointer) return false;
        if (is_function_pointer) {
            if (*return_type != *other.return_type) return false;
            if (param_types.size() != other.param_types.size()) return false;
            for (size_t i = 0; i < param_types.size(); ++i) {
                if (param_types[i] != other.param_types[i]) return false;
            }
            return true;
        }
        // ... existing equality check
    }
};
```

#### 2. Parser Grammar ([`language/src/processes/parser.cpp`](language/src/processes/parser.cpp))
* In `ParserState::parse_type_info()`:
  * Parse base return type (e.g. `int32`, `void`, `String`).
  * Check for function pointer indicator: `match(TokenType::PUNCTUATION_OPEN_PAREN)` followed by `match(TokenType::OPERATOR_MULTIPLY)` and `consume(TokenType::PUNCTUATION_CLOSE_PAREN)`.
  * If detected:
    * Set `type.is_function_pointer = true`.
    * Move base type into `type.return_type = std::make_shared<TypeInfo>(base_type)`.
    * Consume opening `(` for parameters.
    * Parse comma-separated `parse_type_info()` parameters until closing `)`.
* In `ParserState::parse_call_or_access()`:
  * Expressions like `op(10, 20)` are parsed as a standard call expression where the callee is an expression (`IdentifierNode` or member access), not just a literal method name.

#### 3. Binder & Semantic Analysis ([`language/src/processes/binder.cpp`](language/src/processes/binder.cpp))
* **Function Address Binding (`IdentifierNode`)**:
  * In `Binder::visit(IdentifierNode &)`:
    * If identifier resolves to a `MethodDeclaration` without an immediate invocation parenthesis:
      * Validate that the target method is `static`. (Taking the address of instance methods without a bound receiver is disallowed in primitive function pointers).
      * Synthesize a function pointer `TypeInfo` from the method's return type and parameter types:
        - `return_type = method->return_type`
        - `param_types = method->parameters[i]->type_info`
      * Tag the identifier expression with this function pointer type.
* **Assignability Checking (`is_assignable`)**:
  * In `Binder::is_assignable(target, source)`:
    * If `target.is_function_pointer`:
      * `source == null` (type `void`): return `true` (null function pointer assignment).
      * If `source.is_function_pointer`:
        * Return `true` if `is_assignable(*target.return_type, *source.return_type)` AND each parameter satisfies `is_assignable(source.param_types[i], target.param_types[i])` (contravariant parameters, covariant return type).
* **Indirect Call Validation**:
  * When analyzing a call expression where `callee` has `is_function_pointer == true`:
    * Verify argument count matches `callee.type_info.param_types.size()`.
    * Verify each argument expression is assignable to the corresponding parameter type.
    * Set call expression type to `*callee.type_info.return_type`.

#### 4. Assembler & Linker Patches ([`language/src/processes/assembler.cpp`](language/src/processes/assembler.cpp))
* **Loading Function Address**:
  * When compiling an `IdentifierNode` that resolved to a `MethodDeclaration`:
    * Emit `PUSH_CONST_I32 0xFFFFFFFF`.
    * Register a `linker_patch` entry `{bytecode().size() - 4, method}`.
    * When `apply_linker_patches()` runs, `0xFFFFFFFF` is patched with `function_ips[method]` (stored in lower 32 bits of 64-bit slot; upper 32 bits remain 0).
* **Loading `null`**:
  * Emits `PUSH_CONST_I64 0`.
* **Compiling Indirect Call (`op(a, b)`)**:
  1. Compile and push arguments left-to-right.
  2. Compile and push the function pointer expression (leaves 64-bit callable value on the stack).
  3. Emit `PUSH_CONST_I32 <arg_count>`.
  4. Emit `OpCode::CALL`.
  *(Note: Relies on Phase 23 where `CALL` only pops `arg_count` and the 64-bit callable, and the callee executes its own `ALLOC_FRAME`).*

#### 5. Runtime Execution & Null Safety ([`language/src/runtime.cpp`](language/src/runtime.cpp))
* In `op_CALL`:
  * Read 64-bit callable value:
    * `uint32_t target_ip = (uint32_t)(callable & 0xFFFFFFFF);`
    * `uint32_t env_address = (uint32_t)(callable >> 32);`
  * If `target_ip == 0`:
    * Throw `NullPointerException: Attempted to invoke null function pointer`.
  * Set `context.active_closure_env = env_address`.
  * Jump to `target_ip` as normal.

#### 6. Verification & Test Plan
* **Positive Scenarios (`tests/statements/expressions/test_function_pointer.cpp`)**:
  1. Direct assignment and invocation: `int32(*)(int32, int32) add_ptr = Math.add; add_ptr(10, 20) == 30`.
  2. Null initialization and reassignment: `op = null; op = Math.add;`.
  3. Higher-order function passing: `apply(Math.add, 10, 20)`.
  4. Aliased type definitions: `alias Op = int32(*)(int32, int32);`.
  5. Returning function pointers: `get_math_func("add")(10, 20)`.
* **Negative Scenarios (Compile-time & Runtime Diagnostics)**:
  1. Parameter count mismatch: Assigning 2-param function to 1-param function pointer.
  2. Return type mismatch: Assigning `void` function to `int32` function pointer.
  3. Taking address of non-static instance method without an instance (`[ERROR] Cannot take address of non-static method`).
  4. Invoking null function pointer throws runtime `NullPointerException`.

---

## Phase 25: First-Class Lambdas & Closures [PLANNED]

### Status: PLANNED

### Architectural Overview
In Solix, lambdas and function pointers are unified under the same first-class primitive callable type:
$$\texttt{<return\_type>(*)(<arguments\_types>)}$$

Every callable slot in Solix is a 64-bit word (`uint64_t`) packing both an instruction pointer and an optional capture environment address:
$$\texttt{callable\_val} = (\texttt{env\_address} \ll 32) \mid \texttt{target\_ip}$$

* **Normal Function Pointer**: Uses only the lower 32 bits (`target_ip = func_ip`, `env_address = 0`). Upper 32 bits are zero.
* **Capturing Lambda (Closure)**: Uses the full 64 bits:
  * Lower 32 bits: `target_ip` points to the synthesized lambda bytecode.
  * Upper 32 bits: `env_address` points to a heap-allocated array containing the captured variables ("backpack").
* **Stateless Lambda (`[]`)**: If a lambda captures no variables, `env_address = 0`, requiring **zero heap allocation** and operating with the exact same performance and footprint as a standard function pointer.

Because both fit into the same 64-bit value, any API accepting `<return_type>(*)(<args>)` can transparently receive a static function, a stateless lambda, or a capturing closure without wrapper objects or template bloat.

---

### Language Syntax & Ergonomics

#### 1. Lambda Expression Syntax
Lambdas use C++-inspired capture brackets followed by parameter declarations, an arrow `=>`, and a body:
```solix
// Basic capturing lambda
int32 factor = 5;
int32(*)(int32) multiplier = [factor](int32 x) => {
    return x * factor;
};

// Stateless lambda (zero heap allocation)
int32(*)(int32, int32) add = [](int32 a, int32 b) => {
    return a + b;
};

// Single-expression body shorthand
int32(*)(int32) double_val = [](int32 x) => x * 2;

// Explicit return type specification (optional)
int32(*)(int32) inc = [factor](int32 x): int32 => {
    return x + factor;
};
```

#### 2. Capture Semantics
Variables in the capture list `[var1, var2, ...]` are captured by value from the surrounding lexical scope at the exact moment the lambda expression is evaluated:
```solix
public class CounterManager {
    private int32 step = 10;

    public void registerHandler() {
        // Capturing 'this' allows access to instance fields inside the lambda:
        void(*)() callback = [this]() => {
            this.step += 1;
        };
        callback();
    }
}
```

#### 3. Distinct Backpack Instance per Evaluation
Executing a lambda inside a loop or recursive call allocates a fresh, independent capture environment each time:
```solix
alias Supplier = int32(*)();
Supplier[] suppliers = new Supplier[3];

for (int32 i = 0; i < 3; i += 1) {
    // Each iteration captures the current value of 'i' in its own separate heap backpack:
    suppliers[i] = [i]() => i;
}

suppliers[0](); // returns 0
suppliers[1](); // returns 1
suppliers[2](); // returns 2
```

---

### The Callable Address System & Scope Memory Lifecycle

#### 1. The 64-Bit Packed Address Model
In Solix, memory words on the stack and heap are 64 bits (`uint64_t`), while heap addresses and bytecode instruction pointers are 32 bits (`uint32_t`). A callable primitive `<return_type>(*)(<arguments_types>)` packs both into a single 64-bit value:
$$\texttt{callable\_val} = (\texttt{static\_cast<uint64\_t>(env\_address)} \ll 32) \mid \texttt{static\_cast<uint64\_t>(target\_ip)}$$

```
+------------------------------------+------------------------------------+
|   Upper 32 Bits: env_address       |   Lower 32 Bits: target_ip         |
|   (Heap backpack containing state) |   (Bytecode instruction pointer)   |
+------------------------------------+------------------------------------+
```

* **Stateless Function Pointer / Pure Lambda (`[]`)**: `env_address == 0`. Uses only the lower 32 bits; upper 32 bits are zero. Requires **zero heap allocation and zero deallocation**.
* **Capturing Lambda (Closure)**: `env_address != 0`. `env_address` is a valid heap block address returned by `Memory::dynamic_allocation`.

#### 2. The ARC Scope Cleanup Problem & Solution
In Solix's ARC (Automatic Reference Counting) engine, standard heap objects (`Class`, `Array`, `String`) hold a 32-bit `Address` in the lower bits of the slot. When exiting a scope, `Assembler::emit_cleanup_for_node` emits `GET_LOCAL` followed by `DEC_REF`:
```cpp
// Existing OpCode::DEC_REF:
Address addr = static_cast<Address>(POP()); // Truncates to lower 32 bits!
memory.decrease_reference(addr);
```

> [!CAUTION]
> If standard `DEC_REF` were executed on a 64-bit callable, casting to `Address` would extract `target_ip` (the bytecode address in the lower 32 bits) rather than `env_address`! Calling `decrease_reference(target_ip)` would corrupt unrelated heap headers. Conversely, doing nothing would leak the capture array ("backpack") on the heap forever.

To solve this cleanly and efficiently, Solix introduces **callable-aware reference counting**:
* **`OpCode::INC_REF_CALLABLE`**:
  * Reads the 64-bit value: extracts `Address env = static_cast<Address>(val >> 32)`.
  * If `env != 0`: calls `memory.increase_reference(env)`.
  * If `env == 0`: instant no-op (zero overhead for stateless functions).
* **`OpCode::DEC_REF_CALLABLE`**:
  * Reads the 64-bit value: extracts `Address env = static_cast<Address>(val >> 32)`.
  * If `env != 0`: decrements the backpack's reference count via `decrease_reference_callable(env)`.
  * If `env == 0`: instant no-op.

#### 3. Automatic Backpack Deallocation on Scope Exit
When any variable of type `<return_type>(*)(<args>)` goes out of scope (at block termination, function return, or during exception unwinding), the compiler automatically emits cleanup bytecode:

```solix
{
    int32 count = 10;
    // Heap array ("backpack") allocated at env_address, ref_count = 1
    int32(*)(int32) adder = [count](int32 x) => x + count;
    adder(5);
    // Scope exits here:
    // Compiler automatically emits:
    //   GET_LOCAL <adder_slot>
    //   DEC_REF_CALLABLE
    // ref_count drops to 0 -> backpack array is FREED immediately!
}
```

When `decrease_reference_callable(env)` drops the backpack's reference count to 0:
1. The memory block `[env - 1]` is immediately reclaimed via `Memory::deallocate(env)`.
2. The block header is appended to `Memory::free_blocks`, and `currently_used_words` is decremented.
3. Future allocations (such as subsequent loop iterations or new objects) instantly reuse this freed memory block without fragmentation or heap growth.

#### 4. Deep Cleanup of Captured Reference Variables
What if a lambda captures reference types (such as `String`, a class instance, or another nested lambda)?
When the lambda is created, each captured reference type must have its reference count incremented so that the backpack safely retains it. When the lambda goes out of scope and the backpack is freed, those captured objects must not be leaked!

To ensure 100% leak-free closures with zero runtime overhead for primitives:
1. **Backpack Layout**:
   * **Slot 0**: `uint64_t capture_ref_mask` (a 64-bit bitmask where bit $i = 1$ indicates that captured item $i$ is a standard reference type).
   * **Slot 1**: `uint64_t capture_callable_mask` (a 64-bit bitmask where bit $i = 1$ indicates that captured item $i$ is a nested callable).
   * **Slots $2 \dots C + 1$**: The captured values.
2. **Deep Deallocation Sequence (`Memory::decrease_reference_callable(Address env)`)**:
   When the backpack's ref count drops to 0:
   * Read `ref_mask = heap[env]` and `callable_mask = heap[env + 1]`.
   * For each bit $i$ set in `ref_mask`:
     * `Address obj = static_cast<Address>(heap[env + 2 + i]);`
     * `memory.decrease_reference(obj);`
   * For each bit $i$ set in `callable_mask`:
     * `uint64_t nested = heap[env + 2 + i];`
     * `Address nested_env = static_cast<Address>(nested >> 32);`
     * If `nested_env != 0`: `memory.decrease_reference(nested_env);`
   * Finally, call `memory.deallocate(env)`.

> [!TIP]
> If all captured variables are primitives (`int32`, `float64`, etc.), `ref_mask == 0` and `callable_mask == 0`. The deep cleanup loop is bypassed completely, executing an instantaneous $O(1)$ deallocation into `free_blocks`.

#### 5. Assignment, Reassignment, and Returning Callables
* **Assignment (`callable_a = callable_b`)**:
  * Retain new value: `INC_REF_CALLABLE` on `callable_b`.
  * Release old value: `GET_LOCAL <callable_a>` followed by `DEC_REF_CALLABLE`.
* **Returning from Functions**:
  * In `compile_return_statement`: if returning a callable expression, emit `INC_REF_CALLABLE` on the returned value before `emit_cleanup_for_node` / `emit_cleanup_for_function` executes.
  * When local variables leave scope and run `DEC_REF_CALLABLE`, the returned closure retains a net reference count of 1 and safely propagates to the caller.
* **Exception Unwinding**:
  * `BlockStatement` contains an exception cleanup segment patched to run during stack unwinding. `emit_cleanup_for_node` emits `DEC_REF_CALLABLE` for all in-scope callables, guaranteeing zero memory leaks even if an exception aborts the scope early.

---

### Implementation Blueprint

#### 1. AST Node ([`language/src/utilities/statements.hpp`](language/src/utilities/statements.hpp))
```cpp
class LambdaExpression : public ExpressionNode {
public:
    std::vector<std::string> capture_names;
    std::vector<std::unique_ptr<VariableDeclaration>> parameters;
    std::shared_ptr<TypeInfo> explicit_return_type;
    std::unique_ptr<Node> body; // BlockStatement or ExpressionNode

    // Filled during Semantic Binding:
    std::string synthesized_func_name;
    std::shared_ptr<MethodDeclaration> synthesized_method;
    std::vector<std::shared_ptr<VariableDeclaration>> resolved_captures;
    uint64_t capture_ref_mask = 0;
    uint64_t capture_callable_mask = 0;

    void accept(ASTVisitor &visitor) override;
};
```

#### 2. Parser Grammar ([`language/src/processes/parser.cpp`](language/src/processes/parser.cpp))
In `ParserState::parse_primary()`:
* Detect lambda initiation when encountering `[`:
  * Parse comma-separated capture identifiers `[x, y, this]` until `]`.
  * Consume `(` and parse comma-separated parameter declarations `(int32 a, String b)` until `)`.
  * If next token is `:`, parse explicit return `parse_type_info()`.
  * Consume `=>` (`TokenType::OPERATOR_FAT_ARROW` or `=` followed by `>`).
  * If next token is `{`, parse `parse_block_statement()`; otherwise parse single `parse_expression()`.

#### 3. Semantic Binder ([`language/src/processes/binder.cpp`](language/src/processes/binder.cpp))
* **Capture Resolution & Mask Generation**:
  * For each name in `capture_names`:
    * Look up in the enclosing lexical scope (parameters, local variables, or `this`).
    * Report compile error if identifier is not accessible.
    * Record resolved variable declaration and its `TypeInfo`.
    * If resolved variable is a reference type (`is_reference_type`), set bit $i$ in `capture_ref_mask`.
    * If resolved variable is a callable (`type_info.is_function_pointer`), set bit $i$ in `capture_callable_mask`.
* **Synthesized Method Creation**:
  * Synthesize an anonymous static method `__lambda_<id>`:
    * Parameters: all user-declared lambda parameters, followed by internal parameters representing the captures.
    * Return type: inferred from `return` statements in the body (or matching `explicit_return_type`).
  * Register `__lambda_<id>` into the current class or global package scope.
  * Bind the lambda body inside the synthetic method's own scope. Any reference to a captured variable is bound directly to its corresponding capture slot.
* **Expression Type Synthesis**:
  * Assign `type_info` of `LambdaExpression` as `TypeInfo` with `is_function_pointer = true`, populated with the synthetic method's return type and parameter types.

#### 4. VM Opcodes ([`language/src/utilities/optcodes.hpp`](language/src/utilities/optcodes.hpp))
Add the following dedicated opcodes:
* `OpCode::INC_REF_CALLABLE`:
  * Pops 64-bit callable, increments reference count of upper 32-bit `env_address` if non-zero, pushes 64-bit callable back.
* `OpCode::DEC_REF_CALLABLE`:
  * Pops 64-bit callable, decrements reference count of upper 32-bit `env_address` if non-zero, freeing backpack if ref count reaches zero.
* `OpCode::UNPACK_CAPTURES`:
  * Operands: `<dest_slot: u8> <count: u8>`.
  * Callee prologue instruction that reads `context.active_closure_env` and unpacks capture values (starting at offset 2 past the masks) directly into local frame slots.

#### 5. Assembler & Bytecode Generation ([`language/src/processes/assembler.cpp`](language/src/processes/assembler.cpp))
* **Scope Exit Generation (`emit_cleanup_for_node` & `emit_cleanup_for_function`)**:
  ```cpp
  if (var_decl->is_reference_type) {
      emit_byte(static_cast<uint8_t>(OpCode::GET_LOCAL));
      emit_int32(var_decl->memory_index);
      emit_byte(static_cast<uint8_t>(OpCode::DEC_REF));
  } else if (var_decl->type_info.is_function_pointer) {
      emit_byte(static_cast<uint8_t>(OpCode::GET_LOCAL));
      emit_int32(var_decl->memory_index);
      emit_byte(static_cast<uint8_t>(OpCode::DEC_REF_CALLABLE));
  }
  ```
* **Synthesized Lambda Bytecode Generation**:
  * Emit function label for `__lambda_<id>`.
  * Emit `ALLOC_FRAME <param_count + capture_count + local_count>`.
  * If `capture_count > 0`:
    * Emit `UNPACK_CAPTURES <param_count> <capture_count>`.
  * Emit lambda body bytecode. Access to captures is compiled as direct `GET_LOCAL` / `SET_LOCAL` to slots `[param_count ... param_count + capture_count - 1]`.
  * Emit default `RETURN_VOID` / `RETURN_VAL`.
* **Lambda Instantiation Site Generation**:
  * If `capture_count == 0` (stateless lambda):
    * Emit `PUSH_CONST_I32 0xFFFFFFFF` (linker patch for `__lambda_<id>` instruction pointer).
    * (Leaves 64-bit integer on stack with `env = 0`).
  * If `capture_count > 0` (capturing closure):
    1. Emit `PUSH_CONST_I32 <capture_count + 2>`.
    2. Emit `OpCode::ALLOC_DYNAMIC` (allocates array of $C + 2$ words on heap, leaves `env_address` on stack).
    3. Emit `DUP` -> `PUSH_CONST_I32 0` -> `PUSH_CONST_I64 <capture_ref_mask>` -> `OpCode::SET_ARRAY`.
    4. Emit `DUP` -> `PUSH_CONST_I32 1` -> `PUSH_CONST_I64 <capture_callable_mask>` -> `OpCode::SET_ARRAY`.
    5. For each capture $i \in [0, C-1]$:
       * Emit `DUP` (keeps `env_address` on stack).
       * Emit `PUSH_CONST_I32 <i + 2>`.
       * Emit expression loading captured variable value (`GET_LOCAL`, etc.).
       * If captured variable is reference type: emit `OpCode::INC_REF` (or `INC_REF_CALLABLE` if callable).
       * Emit `OpCode::SET_ARRAY` (stores captured value into `heap[env_address + 2 + i]`).
    6. Emit `PUSH_CONST_I32 0xFFFFFFFF` (linker patch for `__lambda_<id>` instruction pointer).
    7. Pack into single 64-bit word:
       * Shift `env_address` left by 32 bits and bitwise OR with `lambda_ip`.
       * Resulting 64-bit callable value `(env << 32) | ip` sits cleanly on top of the stack.

#### 6. VM Runtime Execution ([`language/src/runtime.cpp`](language/src/runtime.cpp))
* **In `decrease_reference_callable(Address env)`**:
  ```cpp
  void Memory::decrease_reference_callable(Address env) {
      if (env == 0) return;
      uint64_t &header = heap[env - 1];
      uint32_t ref_count = static_cast<uint32_t>(header & 0xFFFFFFFF);
      if (ref_count > 0) {
          ref_count--;
          header = (header & 0xFFFFFFFF00000000ULL) | ref_count;
          if (ref_count == 0) {
              // Deep cleanup: inspect masks
              uint64_t ref_mask = heap[env];
              uint64_t callable_mask = heap[env + 1];
              uint32_t size = static_cast<uint32_t>(header >> 32);
              uint32_t count = size >= 2 ? size - 2 : 0;
              for (uint32_t i = 0; i < count; ++i) {
                  if ((ref_mask >> i) & 1) {
                      decrease_reference(static_cast<Address>(heap[env + 2 + i]));
                  } else if ((callable_mask >> i) & 1) {
                      uint64_t val = heap[env + 2 + i];
                      Address nested_env = static_cast<Address>(val >> 32);
                      decrease_reference_callable(nested_env);
                  }
              }
              deallocate(env);
          }
      }
  }
  ```
* **In `op_INC_REF_CALLABLE`**:
  ```cpp
  uint64_t val = POP();
  Address env = static_cast<Address>(val >> 32);
  if (env != 0) memory.increase_reference(env);
  PUSH(val);
  DISPATCH();
  ```
* **In `op_DEC_REF_CALLABLE`**:
  ```cpp
  uint64_t val = POP();
  Address env = static_cast<Address>(val >> 32);
  if (env != 0) memory.decrease_reference_callable(env);
  DISPATCH();
  ```
* **In `op_CALL`**:
  * Pop 64-bit callable value.
  * Extract:
    ```cpp
    uint32_t target_ip = (uint32_t)(callable_val & 0xFFFFFFFF);
    uint32_t env_address = (uint32_t)(callable_val >> 32);
    ```
  * Verify `target_ip != 0` (throw `NullPointerException` if null).
  * Save caller's active closure env and set `context.active_closure_env = env_address`.
* **In `op_UNPACK_CAPTURES <dest_slot> <count>`**:
  * Fetch `Address env = context.active_closure_env;`.
  * Loop $i$ from 0 to `count - 1`:
    * `current_frame[dest_slot + i] = heap[env + 2 + i];`.
* **In `op_RETURN`**:
  * Restore previous frame's `active_closure_env`.

---

### Verification & Test Plan
* **Positive Scenarios (`tests/statements/expressions/test_lambda.cpp`)**:
  1. **Stateless Lambdas**: `[](int32 a, int32 b) => a + b` assigned to `int32(*)(int32, int32)` with zero heap allocations (`currently_used_words` remains unchanged).
  2. **Single & Multi-Value Captures**: `[x, y](int32 z) => x + y + z` capturing local variables from enclosing function.
  3. **Automatic Scope Deallocation**: Lambda created inside a block; verify `currently_used_words` returns to pre-block value after scope exit (backpack is freed into `free_blocks`).
  4. **Deep Capture Cleanup**: Lambda capturing a `String` and a custom `Class` instance; verify that when the lambda goes out of scope, both the backpack and the captured objects are freed.
  5. **Loop Independence & Recycling**: Creating lambdas in a loop capturing iteration counter `i`, ensuring each lambda retains its specific counter value and frees correctly.
  6. **Capturing `this`**: Lambda inside a class method capturing `this` and mutating instance fields.
  7. **Returning Closures**: Function returning `[x](int32 y) => x + y` survives caller's frame destruction and can be called repeatedly without premature deallocation.
  8. **Higher-Order Integration**: Passing capturing lambdas to generic collection algorithms (e.g. `List.filter([limit](int32 item) => item > limit)`).
* **Negative Scenarios**:
  1. Attempting to capture non-existent identifiers (`[ERROR] Undefined capture variable: foo`).
  2. Invoking null lambda pointer throws runtime `NullPointerException`.
  3. Attempting to invoke a lambda after its backpack has been improperly managed.
