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

## 3. Standard Library Architecture & Naming

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

## 4. Standard Library Fixes & Modernization

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
