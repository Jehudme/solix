# Solix Project TODO & Roadmap

---

## 1. Compiler Pipeline & Error Halting

- [ ] **Halt Compilation on Stage Errors (Pipeline Integrity)**
  - **Issue**: When the parser encounters syntax errors (e.g. `Syntax Error in file.slx: Expected expression`), it logs an error but execution continues to the Binder and Assembler, ultimately outputting `Successfully compiled to out.slxb`.
  - **Fix**:
    - Update [`language/src/compilation.cpp`](language/src/compilation.cpp):
      - After `lexer.execute()`: check `context.diagnostic->has_errors()` or stage status. If errors exist, abort immediately with `CompilationFailedException`.
      - After `parser.execute()`: if any syntax errors occurred, **do not proceed** to `Binder`. Abort immediately.
      - After `binder.execute()`: if any semantic/binding errors occurred, **do not proceed** to `Assembler`. Abort immediately.
    - Ensure no bytecode (`.slxb`) file is written when any compilation stage fails.
    - Return non-zero exit code from launcher on any compilation failure.

---

## 2. Compilation Logging & Diagnostics Overhaul

- [ ] **Demote Verbose Internal Logs from `INFO` to `TRACE` / `DEBUG`**
  - **Issue**: Compilation logs are cluttered with noisy, internal step-by-step logs marked as `[info]`:
    - `[info] [Parser] Parsing source file: ...`
    - `[info] [Parser] Declared package: ...`
    - `[info] [Parser] Parsing class 'X' at line Y`
    - `[info] [Parser] Completed parsing ... generated N AST nodes`
    - `[info] [Binder] Pass 1a: Registering package and top-level symbols...`
    - `[info] [Binder] Configured active package: ...`
    - `[info] [Binder] Starting Pass 2: Type and Memory Binding...`
  - **Fix**:
    - Demote all fine-grained AST parsing, token walking, and binder pass transitions to `TRACE` or `DEBUG`.
    - Keep `INFO` minimal and user-facing (e.g. `Compiling N files...`, `Compilation completed in X ms`).
    - Provide clean, quiet output by default; enable detailed logs only via `-v` / `--log-level DEBUG`.

- [ ] **Improve Diagnostic Error Formatting & Context**
  - Standardize error output format: `file:line:column: error: <message>`.
  - Include the offending source line snippet with a caret indicator (`^`) pointing to the column.

- [ ] **Introduce Warnings (`WARN`) Subsystem**
  - **Issue**: The compiler has almost no warning diagnostics.
  - **Fix**: Add warnings for:
    - Unused variables and parameters.
    - Unused imports / alias declarations.
    - Shadowed variable declarations.
    - Implicit truncations or conversions.
    - Unreachable code after `return` or `throw`.

---

## 3. Launcher Executable & Build System

- [ ] **Rename Launcher Executable to `solix` in CMake**
  - **Issue**: The CLI tool binary is currently named `solix_launcher`. It should be `solix` for a clean, idiomatic user-facing CLI (e.g., `solix compile file.slx -o app.slxb` and `solix run app.slxb`).
  - **Fix**:
    - In [`launcher/CMakeLists.txt`](launcher/CMakeLists.txt), rename executable target from `solix_launcher` to `solix` (or set `OUTPUT_NAME "solix"`).
    - Update `copy_stdlib` dependency and target references to use `solix`.
    - Update test runners, documentation, and references expecting `solix_launcher`.

- [ ] **Fix 'run' Command to Return Exit Code from Program (`main`)**
  - **Issue**: The `solix run` (or `solix_launcher run`) command currently always terminates with exit code `0` on successful completion, completely discarding the integer return value from `main()` (e.g. `return 42;`). Shell scripts checking `$?` cannot detect program exit codes.
  - **Fix**:
    - In [`language/include/solix/runtime.hpp`](language/include/solix/runtime.hpp) and [`language/src/runtime.cpp`](language/src/runtime.cpp):
      - Update `run(RuntimeOptions &options)` to return `int32_t` (or store exit code in `RuntimeOptions::exit_code`).
      - In `op_HALT`, inspect the VM stack: if `main()` returned an integer value, pop it and use it as the return code. If `main()` has a `void` return type, default to `0`.
    - In [`launcher/src/commands/execute.cpp`](launcher/src/commands/execute.cpp):
      - Capture the integer exit code returned from `run(*opts)` and invoke `std::exit(exit_code)` (or propagate it through the CLI app callback to `main()`).
      - Ensure unhandled exceptions continue exiting with non-zero error code (`1`).

---

## 4. Standard Library Architecture & Naming

- [ ] **Rename `utilities/` to `core/` (or `base/`)**
  - **Issue**: "Utilities" is a vague junk-drawer name. Files inside `utilities/` declare `package solix;`, creating a mismatch between directory structure and package name.
  - **Fix**:
    - Rename directory `launcher/rsc/lib/solix/utilities/` to `launcher/rsc/lib/solix/core/`.
    - Standardize package declarations to `package solix.core;` (or root `package solix;`).

- [ ] **Directory & File Name Cleanup**
  - Rename `launcher/rsc/lib/solix/Maths/` -> `launcher/rsc/lib/solix/math/` (fix capitalization and pluralization).
  - Fix spelling typo: `systems/Environement.slx` -> `systems/Environment.slx`.

- [ ] **Remove or Implement Zombie (0-Byte) Files**
  - **Issue**: 7 standard library files are completely empty (0 bytes):
    - `systems/Environment.slx`
    - `systems/FileSystem.slx`
    - `systems/Process.slx`
    - `systems/Time.slx`
    - `math/Conversions.slx`
    - `math/Operations.slx`
    - `math/Random.slx`
  - **Fix**: Either implement complete APIs for these modules or remove empty stubs so the compiler does not waste time scanning empty files.

---

## 5. Standard Library Fixes & Modernization

- [ ] **Implement Primitives & Array Wrappers (`solix.primitives` / `solix.core`)**
  - **Issue**: Solix lacks object-oriented wrapper classes for primitive values and raw arrays, limiting generic collection storage and object-oriented operations.
  - **Fix**:
    - Create wrapper classes for all primitive scalar types:
      - `Bool` (wrapping `bool`)
      - `Char` (wrapping `char`)
      - `Int8`, `Int16`, `Int32`, `Int64` (wrapping signed integers)
      - `UInt8`, `UInt16`, `UInt32`, `UInt64` (wrapping unsigned integers)
      - `Float32`, `Float64` (wrapping floating-point numbers)
    - Provide standard features on each primitive wrapper:
      - Constructors for boxing (`new Int32(42)`) and accessor `get_value()`.
      - Parsing methods (e.g. `Int32.parse(String)`).
      - Conversion methods (`to_string()`, `to_int64()`, `to_float64()`).
      - Value equality (`equals`), comparison (`compare_to`), and hashing (`hash_code`).
      - Static constants: `MIN_VALUE`, `MAX_VALUE`, `BYTES`, `BITS`.
    - Implement array wrappers:
      - Generic `Array<T>` wrapping raw `T[]` with bounds checking, `size()`, slicing, cloning, mapping, and conversion to/from raw arrays.
      - Primitive-specialized array wrappers (e.g. `IntArray`, `CharArray`, `FloatArray`) if unboxed performance is desired.

- [ ] **Rewrite `Objects.slx` with Generics / Templates**
  - **Issue**: `Objects.slx` currently hardcodes only `String` and `char[]`. Calling `Objects.require_non_null()` or `Objects.is_null()` on user classes fails to compile.
  - **Fix**:
    - Make `Objects` generic or provide generic template methods:
      ```slx
      public static bool is_null<T>(T item);
      public static bool non_null<T>(T item);
      public static T require_non_null<T>(T item);
      public static T require_non_null_message<T>(T item, String message);
      public static bool equals<T>(T first, T second);
      public static int32 hash_code<T>(T item);
      ```
    - Support member method template deduction in `Binder::visit(MethodCallExpression)` for static class calls (`Class.method<T>()`).

- [ ] **Refactor `Arrays.slx`: Remove Typed Suffixes & Add Overloads / Generics**
  - **Issue**: Method names are cluttered with explicit type suffixes: `fill_i32`, `fill_char`, `fill_bool`, `fill_f64`, `swap_i32`, `reverse_i32`, `sort_i32`, `binary_search_i32`. Only `int32[]` has sorting/searching.
  - **Fix**:
    - Replace type suffixes with method overloading:
      - `Arrays.fill(int32[] target, int32 value)`
      - `Arrays.fill(int64[] target, int64 value)`
      - `Arrays.fill(float64[] target, float64 value)`
      - `Arrays.fill(bool[] target, bool value)`
      - `Arrays.fill(char[] target, char value)`
    - Implement generic algorithms:
      - `Arrays.swap<T>(T[] array, int32 first, int32 second)`
      - `Arrays.reverse<T>(T[] array)`
      - `Arrays.copy<T>(T[] src, int32 src_off, T[] dst, int32 dst_off, int32 count)`
      - `Arrays.equals<T>(T[] first, T[] second)`
    - Provide sort and binary search implementations for other numeric types (`float64`, `int64`, `char`).

- [ ] **Fix Lexer Character Literal Bug (`'c'` vs `"str"`)**
  - **Issue**: Single quotes (`'a'`) are handled by `handle_string()` in `lexer.cpp` and emitted as `TokenType::STRING` (`char[]`). There is no scalar `char` literal. This breaks method overload resolution (`fill(char[], char)`) and variable declarations (`char c = 'a'` throws "expected char, got char").
  - **Fix**: In `lexer.cpp`, parse `'...'` as a character literal emitting `TokenType::CHAR` with primitive `char` scalar type.

- [ ] **Collections: Fix $O(N)$ Implementations of `Map` and `HashSet`**
  - **Issue**: `Map<K, V>` and `HashSet<T>` are linear array scans using pointer `==` equality. They do not compute hash codes or use hash buckets.
  - **Fix**:
    - Implement true bucket-based `HashMap<K, V>` and `HashSet<T>` utilizing `Objects.hash_code()`.
    - Rename linear array implementations to `ArrayMap` and `ArraySet` if retained for small-collection optimizations.

- [ ] **Package Imports & Alias Boilerplate Elimination**
  - **Issue**: Solix lacks an `import` keyword, forcing every collection file to repeat 6 identical `alias` lines for exceptions.
  - **Fix**: Add `import package.Symbol;` or `import package.*;` syntax in parser and binder, or allow root `package solix;` types to be implicitly visible in subpackages (`solix.collections`).

---

## 6. Testing Architecture & Infrastructure Overhaul

- [ ] **Restructure Test Directory by Domain**
  - **Issue**: All tests are currently dumped into `tests/src/` with legacy, arbitrary "phase" names (`test_phase20.cpp` through `test_phase31.cpp`). It is impossible to tell what each file tests without reading the source code.
  - **Fix**: Reorganize into a clean, intuitive structure:
    ```
    tests/
    ├── unit/
    │   ├── lexer/          (Tokenization, literals, operators, comments)
    │   ├── parser/         (Grammar, AST generation, error recovery)
    │   ├── binder/         (Symbol resolution, type checking, generics, templates)
    │   ├── assembler/      (Bytecode emission, label resolution, opcode verification)
    │   └── runtime/        (VM execution, ARC memory, stack frames, exception unwinding)
    ├── stdlib/             (Standard library verification suites: core, collections, math, sys)
    ├── e2e/                (End-to-end full compilation & execution tests)
    └── fixtures/           (Standalone .slx test programs with expected output files)
    ```

- [ ] **Rename and De-Phase Legacy Test Files**
  - Replace historical `test_phaseXX.cpp` with descriptive semantic names:
    - `test_phase20.cpp` -> `test_function_templates.cpp`
    - `test_phase21.cpp` -> `test_template_deduction.cpp`
    - `test_phase22.cpp` -> `test_type_resolution_and_null.cpp`
    - `test_phase23.cpp` -> `test_reference_operators.cpp`
    - `test_phase24.cpp` -> `test_typed_alu_opcodes.cpp`
    - `test_phase25.cpp` -> `test_memory_integrity_arrays.cpp`
    - `test_phase26.cpp` -> `test_arc_ownership.cpp`
    - `test_phase27.cpp` -> `test_cleanup_and_control_flow.cpp`
    - `test_phase28.cpp` -> `test_object_init_and_arrays.cpp`
    - `test_phase29.cpp` -> `test_string_pool.cpp`
    - `test_phase30.cpp` -> `test_nested_templates.cpp`
    - `test_phase31.cpp` -> `test_engine_performance.cpp`

- [ ] **Build a Data-Driven File-Based E2E Test Harness**
  - **Issue**: Tests currently hardcode large Solix programs as raw multiline C++ strings and manually call internal compiler steps.
  - **Fix**:
    - Implement a test harness that executes real `.slx` files from `tests/fixtures/`.
    - Support declarative test assertions directly in `.slx` comments:
      ```slx
      // RUN: solix compile %s -o %t.slxb && solix run %t.slxb
      // EXPECT_EXIT: 0
      // EXPECT_STDOUT: "Test Passed"
      ```
    - Eliminate ad-hoc manual testing in `scratch/`.

- [ ] **Integrate Standard Library Regression Suite into `ctest`**
  - Standard library tests (currently in `scratch/test_all_collections.slx` and `scratch/test_all_utilities.slx`) should be registered in CMake/CTest so `ctest` verifies all collections, strings, exceptions, and utilities on every commit.

- [ ] **Clean Up `tests/CMakeLists.txt`**
  - Remove redundant `target_sources()` calls that duplicate `file(GLOB_RECURSE TEST_SOURCES)`.
  - Ensure Catch2 test discovery operates cleanly without duplicate source registrations.

---

## 7. Performance Benchmarking Suite

- [ ] **Create Dedicated Benchmark Suite (`benchmarks/`)**
  - **Goal**: Measure VM execution throughput, memory overhead, and compiler performance with repeatable, automated benchmarks.
  - **Micro-Benchmarks**:
    - **ALU & Loops**: Tight loops performing `int32`, `int64`, and `float64` operations to measure pure opcode dispatch overhead.
    - **Function Calls**: Cost of direct calls, virtual vtable dispatch, and native C++ call boundary transitions.
    - **Memory & ARC**: Heap allocation churn, object creation, array allocation, and reference counter (`retain`/`release`) latency.
    - **Array Access**: Bounds check overhead and sequential vs. random indexing throughput.
    - **Exception Handling**: Unwinding latency and `try-catch` setup/teardown impact on hot paths.
    - **String Manipulation**: `String` concatenation vs. `StringBuilder` append throughput.
  - **Macro-Benchmarks**:
    - Recursive Fibonacci (`fib(35)`).
    - Prime Sieve of Eratosthenes ($10^6$ elements).
    - Matrix multiplication ($256 \times 256$ float64).
    - Large collection operations: inserting, looking up, and sorting $100,000$ elements in `List`, `Map`, and `HashSet`.
  - **Benchmark Runner & Tooling**:
    - Implement a benchmark runner CLI or target producing structured execution metrics (time elapsed, peak memory, throughput).
    - Provide baseline comparison scripts against CPython, LuaJIT, and Native C++.

---

## 8. Language User Guide & Keyword Wiki

- [ ] **Complete Keyword Wiki (`docs/wiki/keywords.md`)**
  - Document every Solix keyword with syntax rules, semantic behavior, and code examples:
    - **Control Flow**: `if`, `else`, `for`, `while`, `do`, `switch`, `case`, `default`, `break`, `continue`, `return`.
    - **Exception Handling**: `try`, `catch`, `finally`, `throw`.
    - **Object-Oriented Programming**: `class`, `extends`, `super`, `new`, `virtual`, `override`, `abstract`, `interface`, `implements`, `instanceof`.
    - **Access & Scope Modifiers**: `public`, `private`, `protected`, `internal`, `static`, `inline`, `native`, `const`, `weak`.
    - **Modularity & Types**: `package`, `alias`, `enum`, `operator`.
  - Include common pitfalls and best practices for each keyword.

- [ ] **Language User Guide & Tutorial (`docs/guide/`)**
  - **Getting Started**: Installing Solix, using the CLI (`solix compile`, `solix run`), creating a first project.
  - **Type System**: Primitive numeric types (`int8` through `uint64`), floating-point types, `bool`, `char`, and arrays (`T[]`).
  - **Generics & Templates**: Writing template classes, generic methods, type deduction, and explicit specialization.
  - **Memory Management (ARC)**: Understanding Solix's reference counting, scope-based cleanup, and using `weak` to break circular references.
  - **Error Handling Guide**: Structuring custom exceptions, catching hierarchy levels, and resource guarantees with `finally`.

---

## 9. Internal Architecture & Feature Implementation Documentation

- [ ] **Compiler Pipeline Architecture Guide (`docs/architecture/pipeline.md`)**
  - Detailed documentation of each compilation process:
    1. **Lexer**: Token streaming, scanner states, character literal handling, keyword mapping.
    2. **Parser**: Recursive descent architecture, operator precedence climbing, AST node taxonomy (`statements.hpp`), and syntax error recording.
    3. **Binder (Semantic Analysis)**:
       - **Pass 1a**: Global symbol table construction, package scoping, class & function template blueprint registration.
       - **Pass 1b**: Class member registration, method signature mangling, operator overload cataloging.
       - **Pass 2**: Type resolution, inheritance DAG validation, memory frame sizing.
       - **Pass 3**: Expression type checking, template monomorphization, ARC lifecycle hook insertion, and method call resolution.
    4. **Assembler**: Bytecode layout, opcode generation, label jump resolution, literal pool serialization, `.slxb` binary header and section format.
    5. **Runtime (VM)**: Stack frame activation, operand stack, ARC heap management, native function registry, and exception unwinding tables.

- [ ] **Feature Implementation Deep Dives (`docs/architecture/features/`)**
  - **Try-Catch & Exception Unwinding**: Bytecode opcodes (`SETUP_TRY_BLOCK`, `TEARDOWN_TRY_BLOCK`, `THROW_EXCEPTION`), runtime handler table lookup, stack unwinding, and `finally` execution guarantees.
  - **Generics & Monomorphization**: Blueprint AST copying, generic parameter substitution, mangled name generation, and deduction algorithms.
  - **Automatic Reference Counting (ARC)**: Compiler-inserted retain/release points, assignment semantics, temporary expression destruction, and weak reference handling.
  - **Polymorphism & Vtables**: Vtable layout, method indexing, and the optimization where vtables are omitted for non-polymorphic, non-throwable classes.
