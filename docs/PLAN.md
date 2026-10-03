# Solix Engine Architecture Roadmap & Engineering Plan

> **Engineering Workflow**: Refer to [WORKFLOW.md](WORKFLOW.md) for the development lifecycle, branching, and testing protocol.

---

## Executive Summary & Core Objectives
1. **Total Stdlib Elimination**: Purge any legacy notion of a built-in standard library from the compiler and launcher to produce an isolated, self-contained language runtime.
2. **Pure Standalone Test Suite**: Ensure all Catch2 unit tests are 100% self-contained, executing purely on in-memory source strings without injecting or loading `.slx` files from disk.
3. **Granular Bug Fixing & Hardening**: Deliver robust, verifiable fixes for interface dispatch, generic default values, cross-platform portability, arithmetic boundary cases, and deterministic ARC verification.

---

## Roadmap Overview

| Phase | Title | Priority | Affected Modules | Status |
|---|---|---|---|---|
| **Phase 1** | Total Decoupling & Legacy Stdlib Purge | `P0 Blocker` | `cli`, `tests`, `build` | - [x] Completed |
| **Phase 2** | Interface VTable Dynamic Dispatch Stabilization | `P0 Blocker` | `core`, `tests` | - [x] Completed |
| **Phase 3** | Generic Type Safety & Default Value Slot Clearance | `P0 Blocker` | `core`, `tests` | - [x] Completed |
| **Phase 4** | Cross-Platform Path & Toolchain Portability | `P1 High` | `cli`, `core` | - [x] Completed |
| **Phase 5** | Two's-Complement & Arithmetic Invariant Hardening | `P1 High` | `core`, `tests` | - [ ] Deferred |
| **Phase 6** | Deterministic ARC Lifecycle Verification | `P1 High` | `core`, `tests` | - [ ] Deferred |
| **Phase 7** | Documentation & Specification Synchronization | `P2 Polish` | `docs`, `cli`, `tests` | - [x] Completed |
| **Phase 8** | Project Manifest & Modular Build Subcommand (`solix build`) | `P1 High` | `cli`, `build` | - [x] Completed |
| **Phase 9** | Project Scaffolding & Initialization Subcommand (`solix new`) | `P1 High` | `cli`, `templates` | - [x] Completed |
| **Phase 10** | Package Lifecycle Management (`install`, `uninstall`, `list`, `details`) | `P1 High` | `cli` | - [x] Completed |
| **Phase 11** | CLI Commands Test Suite & Master Specification (`tests/commands/`) | `P1 High` | `tests`, `cli`, `build` | - [x] Completed |
| **Phase 12** | CLI Commands & Toolchain Documentation (`docs/spec/cli/`) | `P1 High` | `docs`, `cli`, `guide` | - [x] Completed |
| **Phase 13** | Project-Type Dependencies & Transitive SemVer Resolution | `P1 High` | `cli`, `build`, `tests`, `docs` | - [x] Completed |
| **Phase 14** | Direct Project Execution (`solix run` for Projects) | `P1 High` | `cli`, `build`, `tests`, `docs` | - [x] Completed |
| **Phase 15** | Functions, Lambdas & Generics Documentation and Test Hardening | `P1 High` | `docs`, `tests`, `core` | - [x] Completed |
| **Phase 16** | Windows CI and Cross-Platform Test Stabilization | `P0 Blocker` | `core`, `build`, `ci` | - [x] Completed |
| **Phase 17** | Runtime Core & Dynamic Shared Library Loader | `P1 High` | `core`, `tests`, `docs` | - [x] Completed |
| **Phase 18** | CLI Auto-Discovery, Manifest Integration & Developer Guide | `P1 High` | `cli`, `tests`, `docs` | - [x] Completed |
| **Phase 19** | GitHub Actions CI Verification & Multi-Platform Validation | `P0 Blocker` | `ci`, `core` | - [x] Completed |
| **Phase 20** | Standard Library (`solixlib`) Project Scaffolding & Native Setup | `P1 High` | `solixlib`, `build`, `tests`, `docs` | - [x] Completed |
| **Phase 20.1** | Architecture Refactor: Rename Language & Launcher to Core & CLI | `P1 High` | `core`, `cli`, `build`, `tests`, `docs` | - [x] Completed |
| **Phase 21** | Standard Library: `solix.exceptions` (Foundational Exception Hierarchy) | `P0 Blocker` | `solixlib`, `tests` | - [x] Complete |
| **Phase 22** | Standard Library: `solix.system.Console` (Foundational Terminal I/O) | `P1 High` | `solixlib`, `solixlib/native`, `tests` | - [x] Complete |
| **Phase 23** | Standard Library: `solix.core.String` & `StringBuilder` (`IStringable` Contract) | `P1 High` | `solixlib`, `solixlib/native`, `tests` | - [x] Complete |
| **Phase 24** | Standard Library: `solix.core.Primitives` & Types (`Optional<T>`, `Any`, Contracts) | `P1 High` | `solixlib`, `tests` | - [x] Complete |
| **Phase 25** | Standard Library: `solix.math.Math` & Numeric Algorithms (`Random`) | `P2 Medium` | `solixlib`, `solixlib/native`, `tests` | - [x] Complete |
| **Phase 26** | Standard Library: `solix.time.Chrono` (`Duration`, `Instant`, `DateTime`, `Stopwatch`) | `P2 Medium` | `solixlib`, `solixlib/native`, `tests` | - [x] Complete |
| **Phase 27** | Standard Library: `solix.collections.Core` (Interfaces, `IIterable`, `ICollection`, `to_string`) | `P1 High` | `solixlib`, `tests` | - [x] Complete |
| **Phase 28** | Standard Library: `solix.collections.List` (`List<T>` Array & `LinkedList<T>`) | `P1 High` | `solixlib`, `tests` | - [x] Complete |
| **Phase 29** | Standard Library: `solix.collections.Map` (`HashMap<K, V>` & `TreeMap<K, V>`) | `P1 High` | `solixlib`, `tests` | - [x] Complete |
| **Phase 30** | Standard Library: `solix.collections.Set` (`HashSet<T>` & `TreeSet<T>`) | `P2 Medium` | `solixlib`, `tests` | - [x] Complete |
| **Phase 31** | Standard Library: `solix.collections.Linear` (`Stack`, `Queue`, `Deque`, `PriorityQueue`, `CircularBuffer`, `BitSet`) | `P2 Medium` | `solixlib`, `tests` | - [x] Complete |
| **Phase 32** | Standard Library: `solix.io.filesystem` (Unified Path & File System Operations) | `P1 High` | `solixlib`, `solixlib/native`, `tests` | - [x] Complete |
| **Phase 32.1** | Emergency Refactor: Purge `Any.slx`, Modernize `solix.math.Math` with Generics (`abs<T>`, `min<T>`, `max<T>`, `clamp<T>`, `sign<T>`) & `Optional<T>` | `P0 Blocker` | `solixlib/core`, `solixlib/math`, `tests` | - [x] Complete |
| **Phase 32.2** | Emergency Refactor: Generic Collections Sequences (`List<T>`, `LinkedList<T>`, `Collections.slx`, `Algorithms.slx`) with `for_each` & Lambdas | `P0 Blocker` | `solixlib/collections`, `tests` | - [x] Complete |
| **Phase 32.3** | Emergency Refactor: Generic Linear Containers (`Stack<T>`, `Queue<T>`, `Deque<T>`, `PriorityQueue<T>`, `CircularBuffer<T>`) with `for_each` | `P0 Blocker` | `solixlib/collections`, `tests` | - [x] Complete |
| **Phase 32.4** | Emergency Refactor: Generic Associative Containers (`KeyValuePair<K, V>`, `HashMap<K, V>`, `TreeMap<K, V>`, `HashSet<T>`, `TreeSet<T>`) with `for_each` | `P0 Blocker` | `solixlib/collections`, `tests` | - [x] Complete |
| **Phase 32.5** | Emergency Refactor: Align Filesystem (`File.slx`, `Directory.slx`) with `List<String>`, Update `test_filesystem.cpp`, Specs & Regression | `P0 Blocker` | `solixlib/io`, `docs`, `tests` | - [ ] Planned |
| **Phase 33** | Standard Library: `solix.io.Streams` (`IStream`, `FileStream`, `MemoryStream`, Readers/Writers) | `P1 High` | `solixlib`, `solixlib/native`, `tests` | - [ ] Planned |
| **Phase 34** | Standard Library: `solix.system.Environment` (OS, Env, Subprocesses) | `P2 Medium` | `solixlib`, `solixlib/native`, `tests` | - [ ] Planned |
| **Phase 35** | Core Compiler & Runtime: Language Intrinsics (`assert`, `exit`, Hardcoded Built-ins) | `P1 High` | `core`, `tests`, `docs` | - [ ] Planned |
| **Phase 36** | Standard Library: `solix.crypto` (Base64, Hex, SHA-256, MD5) | `P3 Low` | `solixlib`, `solixlib/native`, `tests` | - [ ] Planned |
| **Phase 37** | Standard Library: `solix.net` (TCP & UDP Sockets, Lightweight `HttpClient`) | `P3 Low` | `solixlib`, `solixlib/native`, `tests` | - [ ] Planned |

---

## Phase 1: Total Decoupling & Legacy Stdlib Purge

- **Priority**: `P0 Blocker`
- **Affected Modules**: `tests`, `launcher`, `build`
- **Status**: - [x] Completed & Merged

### Objective
Completely eliminate the bundled standard library (`launcher/rsc/lib/`), removing disk-based stdlib injection in the test harness, and converting all dependent unit tests into self-contained test scenarios.

### Action Items
- [x] **Purge Test Harness Stdlib Injection (`tests/include/test_helper.hpp`)**:
  - Delete `load_stdlib_into_options()` and all directory scanning under `launcher/rsc/lib`.
  - Remove parameter `bool include_stdlib = false` across all helper functions:
    - `compile_sources`
    - `compile_source`
    - `run_sources`
    - `run_source`
    - `assert_compile_success`
    - `assert_compile_error`
    - `run_and_evaluate_int`
  - Enforce that test compilation relies exclusively on in-memory source strings provided in unit tests.
- [x] **Refactor Dependent Test Suites**:
  - `tests/statements/declarations/test_field_declaration.cpp`: Rewrite Cases 3.5 and 3.6 to remove `solix.core.String` and `include_stdlib = true`, validating in-class and static field initialization with local user-defined classes and primitives.
  - `tests/statements/expressions/test_unary_expression.cpp`: Rewrite Case 4.2 to remove `include_stdlib = true` and assert unary minus rejection on an isolated user class rather than `String`.
  - Audit all files in `tests/statements/**/*.cpp` to ensure zero dependencies on `solix.core.*`, `solix.collections.*`, or `solix.systems.*`.
- [x] **Purge Launcher Build Targets & Assets**:
  - Remove the `copy_stdlib` custom target and dependencies in `launcher/CMakeLists.txt`.
  - Remove CLI flags `--stdlib` / `--no-stdlib` and stdlib path resolution logic from `launcher/src/commands/compile.cpp`.
  - Permanently delete the directory `launcher/rsc/lib/`.
- [x] **Purge Stdlib Native Function Implementations (`language/src/natives/`)**:
  - Permanently remove directory `language/src/natives/` and built-in native implementations (`console.cpp`, `utilities.cpp`, etc.).
  - Remove `solix::get_builtin_natives()` from `runtime.hpp` and CLI `--no-builtins` flag from `launcher/src/commands/execute.cpp`.
  - Decouple `tests/include/test_helper.hpp` from native functions and refactor unit tests to operate without stdlib native dependencies.

### Acceptance Criteria
- `tests/include/test_helper.hpp` has no file I/O or standard library loading logic.
- `launcher/rsc/lib/` and `language/src/natives/` are completely removed from the repository.
- All 42 unit test suites compile and pass 100% with `ctest --test-dir build --output-on-failure`.

---

## Phase 2: Interface VTable Dynamic Dispatch Stabilization

- **Priority**: `P0 Blocker`
- **Affected Modules**: `compiler` (Binder, Assembler), `runtime`, `tests`
- **Status**: - [x] Completed & Merged

### Objective
Stabilize interface polymorphism, correct multi-interface VTable layout and offset resolution in the compiler Binder, and ensure correct dynamic dispatch in the VM.

### Action Items
- [x] **Correct Interface VTable Layout in Binder Pass 3**:
  - Fix slot mapping when a class implements multiple interfaces or inherits interface implementations.
  - Ensure interface method indices align with class dispatch tables and interface stub offsets.
- [x] **Runtime Dynamic Dispatch for Multi-Interface Implementations**:
  - Fix VM dynamic dispatch execution (`OpCode::CALL_VIRTUAL` / `OpCode::CALL_INTERFACE`) when dispatching through interface references.
  - Ensure correct resolution of `this` instance offset and virtual table slot across disparate interface hierarchies.
- [x] **Unblock and Verify Interface Unit Tests**:
  - Remove `[!mayfail]` tag from `tests/statements/declarations/test_interface_declaration.cpp`.
  - Ensure all positive and negative interface test scenarios execute cleanly and deterministically.

### Acceptance Criteria
- `test_interface_declaration.cpp` passes without `[!mayfail]`.
- Multi-interface polymorphism and dynamic dispatch behave correctly at runtime.
- Full test suite passes 100% without regressions.

---

## Phase 3: Generic Type Safety & Default Value Slot Clearance

- **Priority**: `P0 Blocker`
- **Affected Modules**: `compiler`, `runtime`, `tests`
- **Status**: - [x] Completed & Merged
 
### Objective
Resolve the contradiction between primitive non-nullability and generic container initialization by introducing a `default(T)` intrinsic and dedicated slot reclamation instruction.

### Action Items
- [x] **Implement `default(T)` Compiler Intrinsic**:
  - Parse and type-check `default(T)` expressions for primitive, object, and function pointer types.
  - Evaluate scalar types to `0`, `0.0`, `false`, `\0`, or `0ULL` (for function pointers).
  - Evaluate reference types (classes, arrays) to `null` (`0ULL`).
- [x] **VM Instruction `DEC_REF_SLOT`**:
  - Introduce `OpCode::DEC_REF_SLOT` taking a local frame slot index and reference mask.
  - Decrement reference counts on non-null object pointers without overwriting primitive value representations.
- [x] **Add Unit Tests for Default Values**:
  - Validate `default(int32)`, `default(bool)`, `default(float64)`, `default(CustomClass)`, `default(T)` in generic classes.
  - Assert slot cleanup and verify that primitive fields are not clobbered.

### Acceptance Criteria
- Generic containers can safely initialize empty slots using `default(T)` without triggering primitive nullability rejection.
- Zero memory leaks during slot cleanup with `DEC_REF_SLOT`.
- All Catch2 tests pass 100%.

---

## Phase 4: Cross-Platform Path & Toolchain Portability

- **Priority**: `P1 High`
- **Affected Modules**: `launcher`, `runtime`
- **Status**: - [x] Completed & Merged

### Objective
Abstract binary path resolution to support Linux, macOS, and Windows cleanly without relying exclusively on `/proc/self/exe`.

### Action Items
- [x] **Abstract Executable Path Discovery**:
  - Replace raw `/proc/self/exe` reads in `launcher/src/commands/compile.cpp` with a cross-platform helper:
    - Linux: `/proc/self/exe` via `readlink`
    - macOS: `_NSGetExecutablePath`
    - Windows: `GetModuleFileNameW`
- [x] **Normalize File System Paths**:
  - Use `std::filesystem::path` uniformly across all path joins and directory inspections.
- [x] **Verify Portability**:
  - Verify executable builds and runs cleanly on Linux, macOS, and Windows environments without missing-file or path crashes.

### Acceptance Criteria
- Path resolution logic compiles and functions correctly across all three operating systems.
- Executable correctly identifies its own directory without hardcoded assumptions.

---

## Phase 5: Two's-Complement & Arithmetic Invariant Hardening

- **Priority**: `P1 High`
- **Affected Modules**: `runtime`, `compiler`, `tests`
- **Status**: - [x] Completed & Merged

### Objective
Prevent signed integer overflow crashes on extreme boundary values such as `INT32_MIN` (`-2147483648`) and `INT64_MIN`.

### Action Items
- [x] **Guard Boundary Negation**:
  - Protect integer-to-string formatting against `-INT32_MIN` overflow UB in C++ runtime logic.
  - Ensure compiler constant folding handles `-2147483648` correctly without accidental promotion or overflow warnings.
- [x] **Harden Hash Table Indexing**:
  - Apply positive bitmasking (`(hash & 0x7FFFFFFF) % capacity`) to prevent negative array indexing when hashing negative integers or `INT32_MIN`.
- [x] **Add Arithmetic Boundary Tests**:
  - Implement test cases covering `INT32_MIN`, `INT32_MAX`, `INT64_MIN`, and `INT64_MAX` across arithmetic, unary negation, string conversion, and comparisons.

### Acceptance Criteria
- Formatting, negating, or hashing `INT32_MIN` and `INT64_MIN` causes no undefined behavior or negative array indexing.
- Boundary test cases pass 100%.

---

## Phase 6: Deterministic ARC Lifecycle Verification

- **Priority**: `P1 High`
- **Affected Modules**: `runtime`, `tests`
- **Status**: - [x] Completed & Merged

### Objective
Provide programmatic introspection of runtime heap allocations and verify deterministic deallocation and destruction order under ARC.

### Action Items
- [x] **Expose Runtime Live Object Hook**:
  - Add `solix::get_live_object_count()` to the runtime memory subsystem.
  - Track active reference-counted allocations and deallocations.
- [x] **Update ARC Test Scenarios**:
  - In `test_field_declaration.cpp` (Case 3.1: Cycle Breaking with Weak References), assert that `get_live_object_count()` drops to 0 when cycles are broken and scopes exit.
- [x] **LIFO Destruction & Nested Scope Regression Tests**:
  - Add unit tests verifying that destructors and cleanups run in strict reverse declaration order (LIFO) within blocks and call frames.

### Acceptance Criteria
- `get_live_object_count()` accurately reflects allocated heap instances.
- Weak references and scope exits cleanly reduce live object counts to zero.

---

## Phase 7: Documentation & Specification Synchronization

- **Priority**: `P2 Polish`
- **Affected Modules**: `docs`, `launcher`, `tests`
- **Status**: - [x] Completed & Merged

### Objective
Synchronize user-facing documentation, README badges, and code examples with the standalone Solix engine.

### Action Items
- [x] **Update `README.md`**:
  - Update test suite badges and documentation to reflect 42 complete test suites.
  - Remove all mentions of `launcher/rsc/lib/` or bundled standard library paths.
  - Fix syntax errors in example code snippets (such as top-level method access and entry signatures).
- [x] **Document Standard Entry Points**:
  - Explicitly document valid entry signatures:
    - `static int32 main()`
    - `static void main(char[][] args)`
    - `static int32 main(char[][] args)`
- [x] **Synchronize `tests/statements/TESTS.md`**:
  - Confirm all 42 test suites in `TESTS.md` reflect purely self-contained scenarios.

### Acceptance Criteria
- `README.md` and `docs/` contain zero references to `launcher/rsc/lib/`.
- Code snippets in documentation compile without syntax errors.
- Test documentation accurately matches all test suites.

---

## Phase 8: Project Manifest & Modular Build Subcommand (`solix build`)

- **Priority**: `P1 High`
- **Affected Modules**: `launcher`, `build`
- **Status**: - [x] Completed & Merged

### Objective
Implement the `solix build` launcher subcommand, enabling zero-configuration builds via `solix.json`. The implementation mirrors the option-mapping logic of `compile.cpp` to populate all fields of `solix::CompilationOptions`, shares the core compilation runner to avoid duplicate code, and adopts an extensible, modular architecture for resolving dependencies (`source`, and future `library` or physical/external files).

### Action Items
- [x] **1. Build System Integration**:
  - Add `nlohmann_json` via `FetchContent` in `launcher/CMakeLists.txt`.
  - Replace deleted `launcher/src/commands/install.cpp` with `launcher/src/commands/build.cpp` in `launcher/CMakeLists.txt`.
- [x] **2. Shared Compilation Execution**:
  - In `launcher/src/commands/compile.hpp` / `compile.cpp`, extract the common compilation execution pipeline (`solix::run(opts)`, output file writing, directory creation, error handling) into a reusable runner function.
  - Refactor `compile.cpp` to utilize the shared runner.
- [x] **3. Modular Dependency Resolution**:
  - Design an extensible `IDependencyResolver` interface and manager for dependency resolution.
  - Implement `SourceDependencyResolver` to handle `"type": "source"` dependencies by loading `.slx` files relative to project root.
  - Support both root `dependencies` and profile-specific `compilation.additional_dependencies`.
- [x] **4. Manifest Parsing & `build.cpp` Subcommand**:
  - Register `solix build` subcommand in `launcher/src/commands/build.cpp`.
  - Add CLI flags `-p, --profile <name>` (default: `"debug"`) and `-m, --manifest <path>` (default: `"./solix.json"`).
  - Map all `CompilationOptions` fields from the active profile:
    - `entry_point`
    - `use_multithreading`
    - `log_level`, `flush_level`, `sink_type`, `log_pattern`, `log_file_path`, `flush_every_seconds`
    - `assembly_output_path`
  - Support `relative_paths` to resolve or display paths relative to project root.
  - Ensure parent directory of output bytecode exists and write compiled artifact.
- [x] **5. Verification & Testing**:
  - Build `launcher/templates/project/` using `solix build`.
  - Verify `build/debug/out.slxbin` and `build/debug/out.slxasm` are produced.
  - Verify profile selection (`--profile release`, `--profile test`).
  - Run full test suite (`ctest`) to ensure zero regressions across the codebase.

### Acceptance Criteria
- `solix build` executes successfully on any project containing a valid `solix.json`.
- All fields of `CompilationOptions` are populated identically to `compile.cpp`.
- Dependency resolution is modular and extensible for future dependency types.
- All 42 unit test suites compile and pass 100%.

---

## Phase 9: Project Scaffolding & Initialization Subcommand (`solix new`)

- **Priority**: `P1 High`
- **Affected Modules**: `launcher`, `templates`
- **Status**: - [x] Completed & Merged

### Objective
Implement the `solix new` project scaffolding subcommand supporting hybrid generation (in-memory generation as the zero-dependency, self-contained default, with disk-based template reference preserved in `launcher/templates/project/` and selectable via `--template <path>`). All manifest parameters (`project`, `version`, `author`, `description`, `license`, `tags`, `entry`) are configurable via CLI arguments, and destination paths are protected against accidental file collisions.

### Action Items
- [x] **1. Create Command Header & Implementation**:
  - Create `launcher/src/commands/new.hpp` and `launcher/src/commands/new.cpp`.
  - Register `setup_new_command(CLI::App& app)`.
- [x] **2. Manifest & Source Scaffolding Logic**:
  - Implement in-memory generation of `solix.json` and `src/main.slx` using `nlohmann::json`.
  - Implement `--template <path>` option to copy from an on-disk template directory if requested.
  - Implement path collision protection (abort if path exists and is not an empty directory, unless `--force` is specified).
- [x] **3. Build System Integration**:
  - Add `src/commands/new.cpp` to `launcher/CMakeLists.txt`.
  - Register `setup_new_command(app)` in `launcher/src/main.cpp`.
- [x] **4. End-to-End Verification & Testing**:
  - Verify project creation with default arguments (`solix new test_default_proj`).
  - Verify project creation with custom parameters (`solix new custom_proj --author "Alice" --license "MIT" --version "1.0.0" --tag cli --entry main`).
  - Build both generated projects using `solix build` and run them with `solix run`.
  - Run full test suite (`ctest`) to ensure zero regressions across all 42 suites.

### Acceptance Criteria
- `solix new <path>` creates a compilable Solix project without requiring external disk assets.
- CLI flags override manifest fields in `solix.json`.
- Collision protection prevents accidental overwriting of existing non-empty folders without `--force`.
- All 42 unit test suites compile and pass 100%.

---

## Phase 10: Package Lifecycle Management (`install`, `uninstall`, `list`, `details`)

- **Priority**: `P1 High`
- **Affected Modules**: `launcher`
- **Status**: - [x] Completed & Merged

### Objective
Implement cross-platform local package management for Solix projects via subcommands `install`, `uninstall`, `list`, and `details`. Projects are installed into a dedicated platform directory (`~/.solix/installed/` or `$SOLIX_HOME/installed/`) inside directories named after a deterministic 16-hex hash ID computed from `name@version`. All package metadata is maintained in a centralized `installed.json` registry file for O(1) lookups, uninstalls, and listings.

### Action Items
- [x] **1. Implement Package Manager Core (`launcher/src/package_manager.hpp` / `.cpp`)**:
  - Implement deterministic 16-hex FNV-1a hash calculation `compute_project_id(name, version)`.
  - Implement cross-platform directory resolution (`get_solix_home()`, `get_installed_dir()`, `get_registry_path()`) supporting `$SOLIX_HOME` override.
  - Implement centralized registry operations on `installed.json` (load, save, add entry, remove entry).
  - Implement `install_project(path, force)`, `uninstall_project(name, version)`, `list_installed_projects()`, and `get_project_details(name, version)`.
- [x] **2. Implement CLI Subcommands (`launcher/src/commands/package.hpp` / `.cpp`)**:
  - `solix install [path] [-f,--force]`
  - `solix uninstall <name> <version>`
  - `solix list`
  - `solix details <name> [version]`
  - Register subcommands in `launcher/CMakeLists.txt` and `launcher/src/main.cpp`.
- [x] **3. End-to-End Verification & Testing**:
  - Test `solix install` on a newly created project -> verify hash ID directory creation and entry in `installed.json`.
  - Test `solix list` -> verify formatted tabular output.
  - Test `solix details <name> <version>` -> verify extensive project metadata display.
  - Test `solix uninstall <name> <version>` -> verify directory removal and registry cleanup.
  - Test collision prevention (require `--force` to reinstall identical version).
  - Run full test suite (`ctest`) to ensure zero regressions across all 42 suites.

### Acceptance Criteria
- `solix install` installs projects into `<installed_dir>/<hash_id>/` where `<hash_id>` is derived from `name@version`.
- `installed.json` centrally records all installed package metadata.
- `solix uninstall` requires both name and version and cleans up the package directory and registry entry.
- `solix list` and `solix details` correctly report installed package information.
- All 42 unit test suites compile and pass 100%.

---

## Phase 11: CLI Commands Test Suite & Master Specification (`tests/commands/`)

- **Priority**: `P1 High`
- **Affected Modules**: `tests`, `launcher`, `build`
- **Status**: - [x] Completed & Merged

### Objective
Establish a dedicated, comprehensive Catch2 unit test suite and master test specification for all Solix CLI subcommands (`compile`, `run`, `build`, `new`, `install`, `uninstall`, `list`, and `details`). The test suites mirror the architectural rigor of `tests/statements/` and `tests/statements/TESTS.md`, validating all positive execution scenarios and negative error handling paths with isolated sandbox test environments.

### Action Items
- [x] **1. Modularize Launcher Library Target**:
  - In `launcher/CMakeLists.txt`, create static library `solix_launcher_core` exposing command runners, dependency resolvers, and the package manager.
  - Link `solix_launcher_core` to both `solix` executable and `solix_tests`.
- [x] **2. Update Build System & Discover Command Tests**:
  - In `tests/CMakeLists.txt`, configure discovery for both `statements/*.cpp` and `commands/*.cpp`.
  - Add launcher include directories to `solix_tests`.
- [x] **3. Create In-Process CLI Test Helper (`tests/include/cli_test_helper.hpp`)**:
  - Implement sandbox temporary directory fixture (`TempDir`).
  - Implement CLI invocation harness capturing exit codes, `std::cout`, and `std::cerr` streams.
- [x] **4. Define Master Test Specification (`tests/commands/TESTS.md`)**:
  - Document all positive and negative test scenarios for every command (`compile`, `run`, `build`, `new`, `install`, `uninstall`, `list`, `details`).
  - Initially tag all scenarios with `[NOT IMPLEMENTED]`.
- [x] **5. Implement Command Catch2 Test Suites**:
  - `tests/commands/test_compile_command.cpp`: Verify compilation, custom output paths, assembly dumping, entry points, and error states.
  - `tests/commands/test_run_command.cpp`: Verify bytecode execution, arguments passing, stack/heap flags, and runtime errors.
  - `tests/commands/test_build_command.cpp`: Verify zero-arg builds, profiles (`debug`, `release`, `test`), manifest paths, and missing file faults.
  - `tests/commands/test_new_command.cpp`: Verify project scaffolding, manifest parameter overrides, template copying, and collision prevention.
  - `tests/commands/test_package_commands.cpp`: Verify `install`, `uninstall`, `list`, and `details` with central `installed.json` registry and deterministic hash IDs.
  - Update all tags in `tests/commands/TESTS.md` from `[NOT IMPLEMENTED]` to `[IMPLEMENTED]`.
- [x] **6. Full Test Suite Verification**:
  - Run `ctest --test-dir build --output-on-failure` to ensure all 42 statement test suites and all new command test suites pass 100%.

### Acceptance Criteria
- `tests/commands/TESTS.md` comprehensively documents all positive and negative scenarios for every subcommand.
- Catch2 test suites under `tests/commands/` execute and verify every scenario.
- All tests pass 100% via `ctest`.

---

## Phase 12: CLI Commands & Comprehensive Toolchain Documentation

- **Priority**: `P1 High`
- **Affected Modules**: `docs`, `launcher`, `guide`, `workflow`
- **Status**: - [x] Completed & Merged

### Objective
Establish comprehensive, authoritative, and formal documentation for the entire Solix toolchain and CLI command suite (`compile`, `run`, `build`, `new`, `install`, `uninstall`, `list`, `details`, and `solix.json` manifest). Bring developer guides up to date with the modular architecture, link CLI specifications into the language specification indexes, and formalize the engineering workflow requiring explicit identification of positive/negative tests and documentation for all commands, statements, and features.

### Identified Test & Documentation Deliverables
- **1. Test Identification**:
  - Verify that existing positive and negative test cases in `tests/commands/TESTS.md` completely match the documented CLI behaviors and arguments across all 8 subcommands.
  - Maintain Catch2 test suite integrity (`test_compile_command.cpp`, `test_run_command.cpp`, `test_build_command.cpp`, `test_new_command.cpp`, `test_package_commands.cpp`).
- **2. Documentation Identification**:
  - `docs/WORKFLOW.md`: Add mandatory rules governing tests (positive + negative) and documentation for all CLI commands, language statements, and features, mandating that each phase explicitly identify them.
  - `docs/spec/cli/README.md`: Master index and architectural overview of the Solix CLI, global invocation syntax, options, and exit codes.
  - `docs/spec/cli/compile.md`: Full formal specification for `solix compile`.
  - `docs/spec/cli/run.md`: Full formal specification for `solix run`.
  - `docs/spec/cli/build.md`: Full formal specification for `solix build`.
  - `docs/spec/cli/new.md`: Full formal specification for `solix new`.
  - `docs/spec/cli/package.md`: Full formal specification for local package management (`install`, `uninstall`, `list`, `details`).
  - `docs/spec/cli/manifest.md`: Complete schema specification for `solix.json`.
  - `docs/guide/01_getting_started.md`: Revise getting started guide to reflect the modern CLI workflow (`new`, `build`, `run`, `install`, `list`, `details`, `uninstall`) and purge obsolete standard library references.
  - `docs/spec/README.md`: Update specification map and tables to reference the new CLI documentation suite.

### Action Items
- [x] **1. Workflow Hardening (`docs/WORKFLOW.md`)**:
  - Formalize rule requiring that whenever any CLI command, statement, or feature is added or updated, positive and negative tests plus corresponding documentation must be updated in lockstep.
  - Enforce phase-level identification of tests and docs.
- [x] **2. Author CLI Command Specifications (`docs/spec/cli/`)**:
  - Write `README.md`, `compile.md`, `run.md`, `build.md`, `new.md`, `package.md`, `manifest.md`.
- [x] **3. Update User Guides & Top-Level Indexes**:
  - Modernize `docs/guide/01_getting_started.md` with accurate CLI instructions and examples.
  - Update `docs/spec/README.md` to link `docs/spec/cli/`.
- [x] **4. Verification & Regression Check**:
  - Ensure all documentation links resolve.
  - Run `ctest --test-dir build --output-on-failure` to verify 100% test passing.

### Acceptance Criteria
- Complete documentation suite exists under `docs/spec/cli/`.
- `docs/WORKFLOW.md` explicitly mandates positive/negative test coverage and documentation updates for all statements, features, and CLI commands.
- `docs/guide/01_getting_started.md` and `docs/spec/README.md` are synchronized.
- All 47 test suites pass 100%.

---

## Phase 13: Project-Type Dependencies & Transitive SemVer Resolution

- **Priority**: `P1 High`
- **Affected Modules**: `launcher`, `build`, `tests`, `docs`
- **Status**: - [x] Completed & Merged

### Objective
Enhance `solix build` to support project-type dependencies (`"type": "project"`) alongside source file dependencies. Enable projects to reference other projects either by relative/absolute disk path or by package name and version from `$SOLIX_HOME`. Implement full transitive dependency resolution, graceful mutual/circular dependency de-duplication, pre-compilation dependency graph verification, and strict Semantic Versioning conflict resolution rules before invoking the compiler.

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/commands/TESTS.md` & `tests/commands/test_build_command.cpp`)**:
  - **Positive Scenarios**:
    - **Case 3.10**: Direct project dependency via local relative `path`.
    - **Case 3.11**: Transitive project dependency chain ($A \to B \to C$).
    - **Case 3.12**: Mutual / circular project dependencies ($A \leftrightarrow B$ and $A \to B \to C \to A$) gracefully terminate traversal, de-duplicate sources, and build successfully.
    - **Case 3.13**: Same major & minor version, different patch ($1.0.1$ vs $1.0.4$) $\to$ silently selects highest patch version $1.0.4$ and builds.
    - **Case 3.14**: Same major version, different minor version ($1.1.0$ vs $1.3.0$) $\to$ outputs warning, selects highest minor version $1.3.0$, and builds.
    - **Case 3.15**: Project dependency resolved from local `$SOLIX_HOME` installed packages.
  - **Negative Scenarios**:
    - **Case 3.16**: Incompatible major versions ($1.0.0$ vs $2.0.0$) $\to$ halts with error before compilation.
    - **Case 3.17**: Project dependency path does not exist or lacks `solix.json` $\to$ halts with descriptive error.
    - **Case 3.18**: Missing source file declared in a dependent project's manifest $\to$ halts with descriptive error.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/cli/manifest.md`: Update schema for `"type": "project"` (supporting `path` and `name`/`version`).
  - `docs/spec/cli/build.md`: Document pre-compilation dependency graph traversal, transitive inheritance, circular reference support, and SemVer conflict resolution matrix.
  - `docs/guide/01_getting_started.md`: Add a section on modular multi-project dependencies.

### Action Items
- [x] **1. Implement Semantic Versioning Parser (`launcher/src/semver.hpp`)**:
  - Create `SemVer` struct with `major`, `minor`, `patch`, parsing logic, comparison operators (`<`, `==`, `>`), and conflict assessment.
- [x] **2. Implement Project Dependency Resolver & Graph Walker (`launcher/src/dependency_resolver.hpp`)**:
  - Implement `ProjectDependencyResolver` supporting `path` and `$SOLIX_HOME` package lookup.
  - Implement recursive dependency graph traversal with a visited set to support mutual/circular dependencies ($A \leftrightarrow B$) without infinite loops.
  - Enforce SemVer conflict resolution:
    - Major gap: halt build with error.
    - Minor gap: emit warning and pick highest minor.
    - Patch gap: cleanly pick highest patch.
  - Collect all unique source files across the resolved project graph.
- [x] **3. Integrate Pre-Compilation Resolution in `solix build` (`launcher/src/commands/build.cpp`)**:
  - Execute full dependency graph resolution, conflict verification, and source ingestion before invoking `execute_compilation_and_write`.
- [x] **4. Update Master Test Specification (`tests/commands/TESTS.md`)**:
  - Add Cases 3.10 through 3.18 tagged with `[NOT IMPLEMENTED]`.
- [x] **5. Implement Catch2 Unit Tests (`tests/commands/test_build_command.cpp`)**:
  - Implement all positive and negative test cases.
  - Update tags in `tests/commands/TESTS.md` from `[NOT IMPLEMENTED]` to `[IMPLEMENTED]`.
- [x] **6. Update Documentation (`docs/spec/cli/manifest.md`, `docs/spec/cli/build.md`, `docs/guide/01_getting_started.md`)**:
  - Document project dependencies, manifest schema, and SemVer conflict resolution rules.
- [x] **7. Full Regression Testing & Merge**:
  - Run `ctest --test-dir build --output-on-failure`.
  - Non-fast-forward merge into `master`.

### Acceptance Criteria
- `"type": "project"` dependencies resolve both via relative `path` and via `$SOLIX_HOME` installed packages.
- Transitive dependencies and mutual/circular references build cleanly.
- SemVer conflict rules strictly enforced (error on different major, warning on different minor, clean on different patch).
- All positive and negative test cases pass 100%.

---

## Phase 14: Direct Project Execution (`solix run` for Projects)

- **Priority**: `P1 High`
- **Affected Modules**: `launcher`, `build`, `tests`, `docs`
- **Status**: - [x] Completed & Merged

### Objective
Extend `solix run` to directly run Solix projects (both uninstalled via directory paths and installed via package name and version from `$SOLIX_HOME`) while allowing user selection of the build profile (`--profile`), automatically building the target profile prior to execution and applying profile-specific runtime configurations (`heap_size`, `stack_size`, default arguments) unless overridden by CLI flags.

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/commands/TESTS.md` & `tests/commands/test_run_command.cpp`)**:
  - **Positive Scenarios**:
    - **Case 2.9**: Run uninstalled project from directory path (default `debug` profile).
    - **Case 2.10**: Run uninstalled project with explicit profile (`--profile release`).
    - **Case 2.11**: Run uninstalled project with manifest `runtime` configuration (`heap_size`, `stack_size`, default `arguments`).
    - **Case 2.12**: Run uninstalled project with CLI arguments overriding manifest default runtime arguments.
    - **Case 2.13**: Run installed project by name and version (`--package <name> --version <version>`).
    - **Case 2.14**: Run installed project using positional `<name>@<version>` syntax.
    - **Case 2.15**: Run installed project with explicit profile (`--profile release`).
  - **Negative Scenarios**:
    - **Case 2.16**: Run project with non-existent directory path.
    - **Case 2.17**: Run project in directory lacking `solix.json`.
    - **Case 2.18**: Run project requesting non-existent profile.
    - **Case 2.19**: Run installed project not found in `$SOLIX_HOME` registry.
    - **Case 2.20**: Run installed project with `--package` but missing `--version`.
    - **Case 2.21**: Run project where source has compilation errors (build fails before VM runs).
- **2. Identified Documentation Deliverables**:
  - `docs/spec/cli/run.md`: Document new options (`--profile`, `--package`, `--version`, `--project`), project execution modes, runtime config resolution, and example commands.
  - `docs/guide/01_getting_started.md`: Update "Running Your Application (`solix run`)" section to demonstrate running local and installed projects directly by profile.

### Action Items
- [x] **1. Expose Reusable Project Builder Helper (`launcher/src/commands/build.hpp` & `build.cpp`)**:
  - Extract and expose `build_project(manifest_path, profile_name, out_binary_path)` returning status and the resulting executable artifact path.
- [x] **2. Upgrade `solix run` Subcommand (`launcher/src/commands/execute.cpp`)**:
  - Add `--profile` (`-P`), `--package` (`-n`), `--version` (`-v`), and optional project path.
  - Distinguish between bytecode files, uninstalled project paths, and installed project packages.
  - Ingest `profiles.<profile>.runtime` settings from `solix.json` (`heap_size`, `stack_size`, `arguments`) with CLI flag overrides.
  - Build project before VM invocation and run compiled binary.
- [x] **3. Update Master Test Specification (`tests/commands/TESTS.md`)**:
  - Add Cases 2.9 through 2.21 tagged with `[NOT IMPLEMENTED]`.
- [x] **4. Implement Catch2 Unit Tests (`tests/commands/test_run_command.cpp`)**:
  - Implement positive and negative test cases.
  - Update tags in `tests/commands/TESTS.md` from `[NOT IMPLEMENTED]` to `[IMPLEMENTED]`.
- [x] **5. Update Documentation (`docs/spec/cli/run.md`, `docs/guide/01_getting_started.md`)**:
  - Document direct project execution and options.
- [x] **6. Full Regression Testing & Merge**:
  - Run `ctest --test-dir build --output-on-failure`.
  - Non-fast-forward merge into `master`.

### Acceptance Criteria
- Uninstalled projects run from directory path or manifest path using requested profile.
- Installed projects run by name and version (`--package`/`--version` or `<name>@<version>`).
- Manifest runtime settings apply properly and are overridable by CLI arguments.
- Existing bytecode file execution (`solix run file.slxbin`) remains 100% backward-compatible.
- All positive and negative test cases pass.

---

## Phase 15: Functions, Lambdas & Generics Documentation and Test Hardening

- **Priority**: `P1 High`
- **Affected Modules**: `docs`, `tests`, `language`
- **Status**: - [x] Completed & Merged

### Objective
Provide comprehensive test coverage and documentation for Function Pointers, First-Class Lambdas & Closures, and Generics / Templates in Solix. Create a dedicated test suite for Generics (`tests/statements/declarations/test_generics.cpp`), add formal specifications for function pointer expressions, lambda expressions, and generics in `docs/spec/`, and author complete guides (`docs/guide/07_functions_and_lambdas.md` and `docs/guide/08_generics_and_templates.md`).

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/statements/TESTS.md` & `tests/statements/declarations/test_generics.cpp`)**:
  - Add **Suite 48: Generics & Template Metaprogramming**
    - **Positive Scenarios**:
      - **Case 48.1**: Single-Parameter Generic Class (`Box<T>`) storing and retrieving primitive and object types.
      - **Case 48.2**: Multi-Parameter Generic Class (`Pair<K, V>`) with independent type instantiations.
      - **Case 48.3**: Generic Interface Implementation (`interface IContainer` and `class Holder<T> implements IContainer`).
      - **Case 48.4**: Generic Method with Explicit Type Arguments (`Utils.convert<int32>(55)`).
      - **Case 48.5**: Generic Method with Implicit Template Argument Deduction (`Deduce.identity(123)`).
      - **Case 48.6**: Generic Method with Deductions from Multiple Arguments (`DeduceMulti.selectFirst(77, 88)`).
      - **Case 48.7**: Nested Generic Types (`Cell<Cell<int32>>`).
      - **Case 48.8**: Generic Class with Function Pointer Fields / Lambdas (`Processor<T>` with `T(*)(T)`).
    - **Negative Scenarios**:
      - **Case 48.9**: Generic Type Argument Count (Arity) Mismatch (`Pair<int32>`).
      - **Case 48.10**: Incompatible Type Assignment Between Specialized Generic Instances (`Box<int32> b = new Box<string>()`).
      - **Case 48.11**: Ambiguous or Conflicting Template Deduction at Call Site (`DeduceMulti.selectFirst(1, "hello")`).
      - **Case 48.12**: Unbound / Undefined Type Parameter Identifier in Method Body.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/statements/expressions/function_pointer_expression.md`: Specification of function pointer types, static method binding, calling conventions, nullability, and invocation bytecode.
  - `docs/spec/statements/expressions/lambda_expression.md`: Specification of lambda expressions, capture lists, closures, ARC memory management, and bytecode emission.
  - `docs/spec/statements/declarations/generic_declaration.md`: Specification of generic classes, interfaces, method templates, deduction, and compile-time monomorphization.
  - `docs/guide/07_functions_and_lambdas.md`: Practical developer guide covering function pointers, static method references, lambda expressions, closures, and ARC memory patterns.
  - `docs/guide/08_generics_and_templates.md`: Practical developer guide covering generic classes, multi-type parameters, generic interfaces, generic methods, deduction, and best practices.
  - Update `docs/spec/statements/README.md`, `docs/guide/README.md`, and `README.md`.

### Action Items
- [x] **1. Define Test Specification in `tests/statements/TESTS.md`**:
  - Add Suite 48 with Cases 48.1 through 48.12.
- [x] **2. Implement Catch2 Unit Tests (`tests/statements/declarations/test_generics.cpp`)**:
  - Implement all 12 positive and negative test cases.
  - Register `test_generics.cpp` in `tests/CMakeLists.txt` (via glob/build).
  - Fix parser constructor/function pointer field disambiguation (`parser.cpp`).
  - Fix recursive template substitution for function pointer return and param types (`template_substitution.hpp`).
  - Fix interface vtable & itable dispatch generation for instantiated generic classes (`binder.cpp`).
- [x] **3. Author Specifications**:
  - Create `docs/spec/statements/expressions/function_pointer_expression.md`.
  - Create `docs/spec/statements/expressions/lambda_expression.md`.
  - Create `docs/spec/statements/declarations/generic_declaration.md`.
  - Update `docs/spec/statements/README.md`.
- [x] **4. Author Guides**:
  - Create `docs/guide/07_functions_and_lambdas.md`.
  - Create `docs/guide/08_generics_and_templates.md`.
  - Update `docs/guide/README.md` and `README.md`.
- [x] **5. Full Regression Testing & Merge**:
  - Run `ctest --test-dir build --output-on-failure` (48/48 test suites passing 100%).
  - Non-fast-forward merge into `master`.

### Acceptance Criteria
- All 12 new Generic test cases pass 100%.
- Existing function pointer and lambda test suites pass 100%.
- Comprehensive documentation created in `docs/spec/` and `docs/guide/`.
- Full test suite passes without regressions (48/48 suites, 100% pass rate).

---

## Phase 16: Windows CI and Cross-Platform Test Stabilization

- **Priority**: `P0 Blocker`
- **Affected Modules**: `runtime` (`language/include/solix/runtime.hpp`, `language/src/runtime.cpp`), `build` (`tests/CMakeLists.txt`, `launcher/CMakeLists.txt`, `.github/workflows/integration.yml`)
- **Status**: - [x] Completed & Merged

### Objective
Resolve the Windows CI test failure (`SIGSEGV - Stack overflow`) in GitHub Actions (`windows-latest (clang)`) across all unit test suites, eliminate runtime stack exhaustion, configure proper PE executable stack reserve, and ensure continuous integration passes 100% on Windows, Linux, and macOS.

### Root Cause Analysis
1. `struct RuntimeContext` in `language/include/solix/runtime.hpp` contained an embedded `std::array<Frame, 65536> call_stack;` taking over 1 MB of contiguous memory.
2. In `language/src/runtime.cpp`, `int32_t run(RuntimeOptions &options)` instantiated `RuntimeContext vm(options);` as an automatic local variable on the thread stack.
3. Windows PE default stack reserve is exactly 1 MB (1,048,576 bytes). Allocating `RuntimeContext` immediately triggered `EXCEPTION_STACK_OVERFLOW` (0xC00000FD) during compiler stack probing (`__chkstk`), causing Catch2 to report `SIGSEGV - Stack overflow` on every test invoking VM execution.

### Action Items
- [x] **1. Decouple RuntimeContext and Call Stack from Thread Stack**:
  - Convert `call_stack` in `RuntimeContext` to `std::vector<Frame>` dynamically allocated on the heap during initialization (`call_stack.resize(65536)`).
  - Allocate `RuntimeContext` on the heap via `std::make_unique<RuntimeContext>(options)` in `solix::run(RuntimeOptions &options)`.
- [x] **2. Cross-Platform Windows Compatibility & Compiler Configuration**:
  - Added `_CRT_SECURE_NO_WARNINGS` definition on Windows in root `CMakeLists.txt` to suppress MSVC runtime warnings.
  - Verified heap-allocated VM reduces stack frame footprint from >1,050,000 bytes down to 8 bytes, fully compatible with Windows PE 1 MB default thread stack.
- [x] **3. Push & Monitor GitHub Actions CI**:
  - Pushed branch to trigger `integration.yml` in GitHub Actions (Run ID `36984687253`).
  - Monitored CI run across all matrix targets (`ubuntu-latest gcc`, `ubuntu-latest clang`, `macos-latest apple-clang`, `windows-latest clang`).
  - Verified `windows-latest (clang)` passed 100% (all 48 test suites).
- [x] **4. Merge to Master**:
  - Merge feature branch into `master` using `--no-ff`.
  - Push `master` and verify final CI run succeeds.

### Acceptance Criteria
- `windows-latest (clang)` job in `.github/workflows/integration.yml` passes completely.
- All 48 test suites execute without stack overflow or segmentation faults on Windows, Linux, and macOS.
- GitHub Actions CI workflow concludes with green status across all matrix jobs.

---

## Phase 17: Runtime Core & Dynamic Shared Library Loader

- **Priority**: `P1 High`
- **Affected Modules**: `language` (`runtime.hpp`, `runtime.cpp`, `shared_library.hpp/cpp`, `native_registry.hpp/cpp`), `tests` (`tests/CMakeLists.txt`, `tests/runtime/fixtures/test_plugin.cpp`, `tests/runtime/test_native_library.cpp`), `docs/spec/runtime/native_interop.md`
- **Status**: - [x] Completed & Merged

### Objective
Implement the low-level C-ABI native interface and dynamic library loader in the Solix VM. Modernize native function representation from `std::function` to raw C function pointers (`NativeFunctionPtr`), implement lock-free $O(1)$ array dispatch in `RuntimeContext`, provide cross-platform `solix::SharedLibrary` for loading `.dll`, `.so`, and `.dylib` files, build thread-safe `solix::NativeRegistry`, support both registration hook (`solix_register_natives`) and direct dynamic symbol export lookup, create a CMake shared library test fixture, and build comprehensive Catch2 test suite (Suite 49) and formal specification.

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/statements/TESTS.md` & `tests/runtime/test_native_library.cpp`)**:
  - Add **Suite 49: Native Library & Function Pointer Interop**
    - **Positive Scenarios**:
      - **Case 49.1**: Static Native Function Call (`NativeMath.add(int32, int32)`).
      - **Case 49.2**: Instance Native Function Call (`Counter.increment()`, validating `self_address` `this` pointer).
      - **Case 49.3**: Batch Registration Hook (`solix_register_natives(NativeRegistry&)`).
      - **Case 49.4**: Direct Dynamic Symbol Resolution Fallback (exported C symbol conforming to `NativeFunctionPtr`).
      - **Case 49.5**: Multiple Shared Libraries Loaded Concurrently in Same Program.
    - **Negative Scenarios**:
      - **Case 49.6**: Missing / Non-Existent Shared Library Path (`LibraryNotFoundException`).
      - **Case 49.7**: Corrupted / Invalid Binary File as Library.
      - **Case 49.8**: Unresolved Native Method Symbol (`Call to unknown native function: <id>`).
- **2. Identified Documentation Deliverables**:
  - `docs/spec/runtime/native_interop.md`: Formal specification of `NativeFunctionPtr`, calling conventions, argument/return value protocol, `SharedLibrary` lifecycle, `NativeRegistry`, and hook protocols.
  - Update `docs/spec/README.md`.

### Action Items
- [x] **1. Define Test Specification in `tests/statements/TESTS.md`**:
  - Add Suite 49 with Cases 49.1 through 49.8 tagged with `[NOT IMPLEMENTED]`.
- [x] **2. Modernize Native Function Representation (`language/include/solix/runtime.hpp` & `language/src/runtime.cpp`)**:
  - Define `NativeFunctionPtr` typedef: `uint64_t (*)(RuntimeContext&, uint64_t self, uint64_t* args, size_t argc)`.
  - Add `std::vector<NativeFunctionPtr> native_table;` in `RuntimeContext` for $O(1)$ dispatch.
- [x] **3. Implement Cross-Platform `SharedLibrary` (`language/include/solix/shared_library.hpp` & `language/src/runtime/shared_library.cpp`)**:
  - Encapsulate `LoadLibraryW`/`GetProcAddress`/`FreeLibrary` on Windows and `dlopen`/`dlsym`/`dlclose` on POSIX.
  - Provide RAII lifetime management.
- [x] **4. Implement Central `NativeRegistry` (`language/include/solix/native_registry.hpp` & `language/src/runtime/native_registry.cpp`)**:
  - Thread-safe global registry with mutex protection.
  - Automatic invocation of `solix_register_natives` hook and dynamic export fallback.
  - Wire resolution into `op_DEFINE_NATIVE` and `op_CALL_NATIVE`.
- [x] **5. Add CMake Shared Library Fixture & Catch2 Test Suite (`tests/CMakeLists.txt`, `tests/runtime/fixtures/test_plugin.cpp`, `tests/runtime/test_native_library.cpp`)**:
  - Define `solix_test_plugin` target.
  - Implement all 8 Catch2 test cases.
  - Update `tests/statements/TESTS.md` tags to `[IMPLEMENTED]`.
- [x] **6. Author Formal Specification (`docs/spec/runtime/native_interop.md`)**:
  - Document ABI, calling conventions, registration protocols, and lifecycle.
  - Update `docs/spec/README.md`.
- [x] **7. Full Regression Testing & Merge**:
  - Run `ctest --test-dir build --output-on-failure`.
  - Non-fast-forward merge into `master`.

### Acceptance Criteria
- All 8 Suite 49 test cases pass 100% on Windows, Linux, and macOS.
- Dynamic shared library loading works across all supported platforms without resource leaks.
- Zero regressions across existing 48 test suites.

---

## Phase 18: CLI Auto-Discovery, Manifest Integration & Developer Guide

- **Priority**: `P1 High`
- **Affected Modules**: `launcher` (`execute.cpp`, `build.hpp/cpp`), `tests` (`tests/commands/test_run_command.cpp`, `tests/commands/TESTS.md`), `docs` (`docs/spec/cli/run.md`, `docs/spec/cli/manifest.md`, `docs/guide/09_native_plugins.md`)
- **Status**: - [x] Completed & Merged

### Objective
Integrate native library loading into the Solix CLI toolchain and manifest system. Enable zero-config auto-discovery of `.dll`, `.so`, and `.dylib` files placed in project `./lib/` directories and alongside compiled `.slxbin` bytecode binaries. Add CLI flag `-L, --native-lib` to `solix run`. Ingest `"native_libraries"` array in `solix.json` root and profile runtime configurations. Author formal specifications and comprehensive developer guides.

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/commands/TESTS.md` & `tests/commands/test_run_command.cpp`)**:
  - Add Cases 2.22 to 2.26 to Suite 2:
    - **Case 2.22**: Run project with auto-discovery from project `lib/` directory.
    - **Case 2.23**: Run bytecode binary with auto-discovery from sibling directory.
    - **Case 2.24**: Run project with explicit `native_libraries` in `solix.json`.
    - **Case 2.25**: Run bytecode with explicit CLI `-L` / `--native-lib` flag.
    - **Case 2.26**: Negative: Run binary requiring native methods without providing library (exit code 1).
- **2. Identified Documentation Deliverables**:
  - `docs/spec/cli/run.md`: Document `--native-lib` / `-L` and auto-discovery rules.
  - `docs/spec/cli/manifest.md`: Document `native_libraries` configuration.
  - `docs/guide/09_native_plugins.md`: Complete developer guide on writing C/C++ plugins and deploying them in Solix.
  - Update `docs/guide/README.md` and `README.md`.

### Action Items
- [x] **1. Define Test Specification in `tests/commands/TESTS.md`**:
  - Add Cases 2.22 to 2.26 tagged with `[NOT IMPLEMENTED]`.
- [x] **2. Upgrade `solix run` Subcommand (`launcher/src/commands/execute.cpp`)**:
  - Add `-L, --native-lib` option.
  - Implement auto-discovery search in `./lib/` and bytecode directory.
  - Ingest `native_libraries` from `solix.json`.
- [x] **3. Implement CLI Unit Tests (`tests/commands/test_run_command.cpp`)**:
  - Implement Cases 2.22 to 2.26.
  - Update `tests/commands/TESTS.md` tags to `[IMPLEMENTED]`.
- [x] **4. Author Specifications and Developer Guide**:
  - Update `docs/spec/cli/run.md` and `docs/spec/cli/manifest.md`.
  - Create `docs/guide/09_native_plugins.md`.
  - Update `docs/guide/README.md` and `README.md`.
- [x] **5. Full Regression Testing & Merge**:
  - Run `ctest --test-dir build --output-on-failure`.
  - Non-fast-forward merge into `master`.

### Acceptance Criteria
- Projects automatically discover and load shared libraries from `./lib/` and `.slxbin` directory.
- CLI flag `--native-lib` / `-L` and `solix.json` `native_libraries` configurations load properly.
- All positive and negative CLI test cases pass 100%.

---

## Phase 19: GitHub Actions CI Verification & Multi-Platform Validation

- **Priority**: `P0 Blocker`
- **Affected Modules**: `.github/workflows/integration.yml`, CI matrix
- **Status**: - [x] Completed & Merged

### Objective
Verify that the complete native dynamic library loading system, CLI auto-discovery, and all unit tests build and pass 100% across all target operating systems and compilers on GitHub Actions (`windows-latest clang`, `ubuntu-latest gcc`, `ubuntu-latest clang`, `macos-latest apple-clang`).

### Action Items
- [x] **1. Push Master to Origin**:
  - Trigger GitHub Actions CI workflow on `master`.
- [x] **2. Monitor CI Run Across Matrix Jobs**:
  - Monitored workflow runs across all matrix targets.
  - Verified `windows-latest (clang)`, `ubuntu-latest (gcc)`, `ubuntu-latest (clang)`, and `macos-latest (apple-clang)` all complete with green status (Run ID `37009821793`).
- [x] **3. Multi-Platform Hardening**:
  - Resolved Windows modal error dialogs on corrupted file load by wrapping `LoadLibraryExW` with `SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX)`.
  - Defined `NOMINMAX` globally on Windows to prevent Win32 macro collisions with Catch2 generator methods (`min()` / `max()`).
  - Added `--timeout 120` to `ctest` in `integration.yml` to prevent indefinite CI blocking.
- [x] **4. Record Completion in PLAN.md**:
  - Marked Phase 19 as completed and verified.

### Acceptance Criteria
- All 4 matrix jobs in `.github/workflows/integration.yml` pass with 100% success.
- Native dynamic library loading verified functional on Windows (`.dll`), Linux (`.so`), and macOS (`.dylib`).

---

## Phase 20: Standard Library (`solixlib`) Project Scaffolding & Native Setup

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/` (`project/solix.json`, `native/CMakeLists.txt`, `project/src/`, `native/src/`), `CMakeLists.txt` (root), `tests/commands/` (`test_package_commands.cpp`, `TESTS.md`), `docs/spec/solixlib/`
- **Status**: - [x] Completed & Merged

### Objective
Establish the standard library (`solixlib`) architecture using strict sub-project separation (`solixlib/project/` for pure Solix and `solixlib/native/` for C/C++ native companions) with full package manager installability (`solix install`) and companion native shared library (`solixlib_native`) compilation. Maintain zero coupling to the launcher binary, enabling SemVer-based stdlib versioning and modular dependency resolution. Provide an empty native registration skeleton without implementing specific native functions yet, ensuring that CMake automatically outputs the shared library into `solixlib/project/lib/` and that the complete project can be installed into `$SOLIX_HOME` and consumed by user projects.

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/commands/TESTS.md` & `tests/commands/test_package_commands.cpp`)**:
  - Add Cases 5.7 to 5.10 under package management:
    - **Case 5.7**: Install `solixlib` project from local path via `solix install solixlib/project`.
    - **Case 5.8**: Verify `solix package list` reports `solixlib` v0.1.0 as installed with valid ID and contains native `lib/`.
    - **Case 5.9**: Verify a consumer project referencing installed `solixlib` compiles and runs.
    - **Case 5.10**: Negative: Re-installing `solixlib` without `--force` fails with exit code 1.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/solixlib/scaffolding.md`: Formal specification of `solixlib` sub-project layout, CMake companion target, and packaging lifecycle.
  - Update `docs/spec/README.md`.

### Action Items
- [x] **1. Define Test Specification in `tests/commands/TESTS.md`**:
  - Add Cases 5.7 to 5.10 tagged with `[NOT IMPLEMENTED]`.
- [x] **2. Scaffold `solixlib/` Project and Native Companion**:
  - Author `solixlib/project/solix.json` defining `solixlib` v0.1.0.
  - Create minimal `solixlib/project/src/solix/core/Internal.slx` placeholder.
  - Create `solixlib/native/src/register.cpp` with empty `solix_register_natives` hook.
  - Author `solixlib/native/CMakeLists.txt` configuring target `solixlib_native` and outputting directly into `solixlib/project/lib/`.
  - Wire `add_subdirectory(solixlib/native)` into root `CMakeLists.txt`.
- [x] **3. Implement Integration Tests (`tests/commands/test_package_commands.cpp`)**:
  - Implement Cases 5.7 to 5.10.
  - Update `tests/commands/TESTS.md` tags to `[IMPLEMENTED]`.
- [x] **4. Author Specifications**:
  - Create `docs/spec/solixlib/scaffolding.md`.
  - Update `docs/spec/README.md`.
- [x] **5. Full Regression Testing & Merge**:
  - Run `ctest --test-dir build --output-on-failure`.
  - Non-fast-forward merge into `master`.

### Acceptance Criteria
- `solixlib/project` is a valid Solix project with a valid `solix.json` manifest.
- CMake builds `solixlib_native` and deposits the binary in `solixlib/project/lib/`.
- `solix install solixlib/project` successfully copies the project and native library into `$SOLIX_HOME`.
- All 49 existing test suites and new package installation tests pass 100%.

---

## Phase 20.1: Architecture Refactor — Rename Language & Launcher to Core & CLI

- **Priority**: `P1 High`
- **Affected Modules**: `core/` (formerly `language/`), `cli/` (formerly `launcher/`), `CMakeLists.txt`, `tests/CMakeLists.txt`, `solixlib/native/CMakeLists.txt`, `.github/workflows/release.yml`, documentation
- **Status**: - [x] Completed & Merged

### Objective
Cleanly decouple and modernize repository architecture by renaming the compiler and runtime component `language/` to `core/` (target `solix_core`), and the command-line driver and packaging component `launcher/` to `cli/` (target `solix_cli_core`, executable `solix`, alias `solix_cli`). Preserve full backward compatibility through CMake aliases (`solix_language` -> `solix_core`, `solix_launcher_core` -> `solix_cli_core`, custom target `solix_launcher` -> `solix`).

### Action Items
- [x] **1. Rename Directories**:
  - Move `language/` to `core/`.
  - Move `launcher/` to `cli/`.
- [x] **2. Update CMake Targets & Includes**:
  - `core/CMakeLists.txt`: Define `solix_core` with backward-compatible alias `solix_language`.
  - `cli/CMakeLists.txt`: Define `solix_cli_core` with alias `solix_launcher_core` and `solix_cli` target.
  - Root `CMakeLists.txt`: Update `add_subdirectory` to `core` and `cli`.
  - `solixlib/native/CMakeLists.txt`: Update include paths to `core/include` and `core/src`.
  - `tests/CMakeLists.txt`: Link `solix_core` and `solix_cli_core`, update include directories.
- [x] **3. Update GitHub Workflows & Documentation**:
  - Update `.github/workflows/release.yml` build target to `solix` and artifact path to `build/cli/solix`.
  - Update `README.md`, `docs/guide/`, and `docs/spec/` to reflect `core/` and `cli/`.
- [x] **4. Full Regression Verification**:
  - Run full suite (`ctest --test-dir build --output-on-failure`): 100% tests pass (49/49).

---

## Standard Library Architecture Principles & Cross-Platform Library Strategy

All standard library (`solixlib`) submodules adhere to these non-negotiable architectural mandates:

1. **Feature-Rich & Comprehensive APIs**:
   - Every submodule must be expressive, fully fledged, and complete. No barebones or skeletal APIs.
   - For example, `String` must provide full static primitive converters (`String.from_int`, `String.from_double`, etc.), static string-to-primitive parsers (`String.to_int`, `String.to_double`), string interpolation/formatting (`String.format`), token joining, padding, casing, and search helpers.
   - `Console` provides colorized output channels (`error`, `warning`, `info`, `success`), typed inputs (`input_int`, `input_double`, `input_bool`, `input_char`, `input_chars`, `input_string`), and terminal control (`set_cursor_position`, `set_title`, `clear`).
2. **Native Performance & Zero-Allocation Conversions**:
   - Bidirectional string &lt;&mdash;&gt; number conversions are implemented in native companion C++ using modern `<charconv>` (`std::to_chars`, `std::from_chars`) for hardware-speed, locale-independent, and zero-allocation execution.
3. **Use Vetted Cross-Platform C Libraries (No Handwriting Complex Math, Crypto, or OS Logic)**:
   - **Cryptography & Hashing (Phase 36)**: Do **NOT** handwrite complex cryptographic algorithms like SHA-256, SHA-1, or MD5. Use well-vetted, battle-tested, lightweight public-domain C libraries (e.g. Brad Conte's standard `crypto-algorithms` / `monocypher`).
   - **Process & Environment (Phase 34)**: Use established lightweight cross-platform C/C++ subprocess libraries (e.g. `subprocess.h` or `reproc`) ensuring robust pipes, timeouts, and error handling across POSIX and Windows.
   - **Networking (Phase 37)**: Use established BSD / Winsock cross-platform socket abstractions and fast HTTP parsing (e.g. `picohttpparser`).
   - **Math (Phase 25)**: Leverage standard IEEE 754 math functions in C/C++ `<cmath>` and `<random>` (Mersenne Twister `std::mt19937_64`).
4. **Expanded, Industrial-Grade Collections**:
   - Multiple collection variants tailored for distinct access patterns:
     - Sequential: `List<T>` (dynamic array) and `LinkedList<T>` (doubly linked list with O(1) head/tail insertions).
     - Associative: `HashMap<K, V>` (hash table) and `TreeMap<K, V>` (red-black tree sorted map).
     - Sets: `HashSet<T>` (hash set) and `TreeSet<T>` (balanced binary search tree sorted set).
     - Linear & Buffers: `Stack<T>`, `Queue<T>`, `Deque<T>`, `PriorityQueue<T>` (binary heap), `CircularBuffer<T>` (ring buffer), and `BitSet` (compact bit array).
5. **Universal Exception Integration**:
   - Every single function across all submodules consistently throws strongly-typed exceptions from `solix.exceptions` on invalid arguments, out-of-bounds access, or runtime failures.

---

## Phase 21: Standard Library — `solix.exceptions` (Foundational Exception Hierarchy)

- **Priority**: `P0 Blocker`
- **Affected Modules**: `solixlib/project/src/solix/exceptions/`, `tests/solixlib/TESTS.md`, `tests/solixlib/test_exceptions.cpp`, `docs/spec/solixlib/exceptions.md`
- **Status**: - [x] Completed & Verified

### Objective
Establish the foundational standard library exception hierarchy `solix.exceptions`. Provide a rich, object-oriented inheritance tree rooted at `Exception` with specialized general-purpose exceptions used consistently across all standard library submodules, compiler runtimes, and user code.

### Interconnection & Layering
- **Universal Error Contract**: Every standard library module (`Console`, `String`, `Primitives`, `Collections`, `FileSystem`, `Net`) will throw instances of this hierarchy on invalid input or unexpected runtime states.
- **Inheritance Tree**:
  ```
  Exception
  ├── RuntimeException
  │   ├── IllegalArgumentException
  │   ├── ArgumentNullException
  │   ├── ArgumentOutOfRangeException
  │   ├── IndexOutOfBoundsException
  │   ├── NullReferenceException
  │   ├── ArithmeticException
  │   │   ├── DivideByZeroException
  │   │   └── OverflowException
  │   ├── InvalidOperationException
  │   ├── FormatException
  │   ├── NotSupportedException
  │   ├── TimeoutException
  │   ├── NoSuchElementException
  │   └── KeyNotFoundException
  ├── IOException
  │   ├── FileNotFoundException
  │   ├── DirectoryNotFoundException
  │   └── SocketException
  └── AssertionError
  ```

### Submodule Architecture & Types
- **Solix Surface (`solixlib/project/src/solix/exceptions/`)**:
  - `class Exception`:
    - `String message`
    - `Exception cause`
    - `Exception(String message = "")`
    - `Exception(String message, Exception cause)`
    - `String get_message()`, `Exception get_cause()`, `String to_string()`
  - `class RuntimeException extends Exception`
  - `class IllegalArgumentException extends RuntimeException`
  - `class ArgumentNullException extends IllegalArgumentException`
  - `class ArgumentOutOfRangeException extends IllegalArgumentException`
  - `class IndexOutOfBoundsException extends RuntimeException`
  - `class NullReferenceException extends RuntimeException`
  - `class ArithmeticException extends RuntimeException`
  - `class DivideByZeroException extends ArithmeticException`
  - `class OverflowException extends ArithmeticException`
  - `class InvalidOperationException extends RuntimeException`
  - `class FormatException extends RuntimeException`
  - `class NotSupportedException extends RuntimeException`
  - `class TimeoutException extends RuntimeException`
  - `class NoSuchElementException extends RuntimeException`
  - `class KeyNotFoundException extends RuntimeException`
  - `class IOException extends Exception`
  - `class FileNotFoundException extends IOException`
  - `class DirectoryNotFoundException extends IOException`
  - `class SocketException extends IOException`
  - `class AssertionError extends Exception`

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/solixlib/TESTS.md` & `tests/solixlib/test_exceptions.cpp`)**:
  - Add and implement Cases 1.1 to 1.8:
    - **Case 1.1 [Positive]**: Instantiate base `Exception` and retrieve message and `to_string()`.
    - **Case 1.2 [Positive]**: Catch `IllegalArgumentException` using base `RuntimeException` catch block (polymorphism verification).
    - **Case 1.3 [Positive]**: Catch `ArgumentOutOfRangeException` using `IllegalArgumentException` and `Exception`.
    - **Case 1.4 [Positive]**: Exception chaining: verify `get_cause()` returns inner exception.
    - **Case 1.5 [Positive]**: `IndexOutOfBoundsException` carrying upper/lower index metadata.
    - **Case 1.6 [Positive]**: `FileNotFoundException` caught as `IOException`.
    - **Case 1.7 [Negative]**: Uncaught exception propagates and exits with non-zero error status.
    - **Case 1.8 [Negative]**: Nested `try-catch-finally` ensuring `finally` executes during exception unwinding.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/solixlib/exceptions.md`: Formal specification of the exception inheritance hierarchy, catching semantics, and recommended usage.

### Action Items
- [x] Define test specification in `tests/solixlib/TESTS.md` (Cases 1.1 to 1.8).
- [x] Author `solix.exceptions` classes under `solixlib/project/src/solix/exceptions/`.
- [x] Implement Catch2 test suite `tests/solixlib/test_exceptions.cpp`.
- [x] Author `docs/spec/solixlib/exceptions.md`.
- [x] Verify 100% test pass rate across all platforms.

---

## Phase 22: Standard Library — `solix.system.Console` (Foundational Terminal I/O)

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/project/src/solix/system/Console.slx`, `solixlib/native/src/console.cpp`, `solixlib/native/CMakeLists.txt`, `tests/solixlib/TESTS.md`, `tests/solixlib/test_console.cpp`, `docs/spec/solixlib/console.md`
- **Status**: - [x] Completed & Verified

### Objective
Implement the foundational terminal I/O module `solix.system.Console` with rich, idiomatic naming (`print`, `println`, `error`, `warning`, `info`, `success`, `input_*`). Deliver colorized diagnostics, multi-type console input for all primitive types, character array buffers, and terminal window/cursor management.

### Interconnection & Layering
- **Immediate Capability**:
  - `print(...)` and `println(...)` for `int`, `long`, `double`, `bool`, `char`, and `char[]`.
  - Colorized diagnostics: `error` (ANSI red `\033[31m`), `warning` (ANSI yellow `\033[33m`), `info` (ANSI cyan `\033[36m`), `success` (ANSI green `\033[32m`).
  - Typed inputs: `input_int()`, `input_double()`, `input_bool()`, `input_char()`, `input_chars()`. Throws `FormatException` from `solix.exceptions` on invalid inputs.
- **Future Revisit (Phase 23)**: When `solix.core.String` and `IStringable` are implemented, `Console` will be revisited to add `print(String)`, `println(String)`, `print(IStringable)`, `println(IStringable)`, `error(String)`, `warning(String)`, `info(String)`, `success(String)`, and `input() -> String`.
- **Future Revisit (Phase 27)**: When `ICollection<T>` is introduced, `Console` will print collections formatted as strings.

### Submodule Architecture & Types
- **Solix Surface (`solixlib/project/src/solix/system/Console.slx`)**:
  - `class Console`:
    - Output:
      - `static void print(int value)`, `static void print(double value)`, `static void print(bool value)`, `static void print(char value)`, `static void print(char[] value)`
      - `static void println(int value)`, `static void println(double value)`, `static void println(bool value)`, `static void println(char value)`, `static void println(char[] value)`, `static void println()`
    - Diagnostics (Colorized):
      - `static void error(int value)`, `static void error(char[] value)`
      - `static void warning(int value)`, `static void warning(char[] value)`
      - `static void info(int value)`, `static void info(char[] value)`
      - `static void success(int value)`, `static void success(char[] value)`
    - Input:
      - `static int input_int()` (throws `FormatException` on parse failure)
      - `static double input_double()` (throws `FormatException` on parse failure)
      - `static bool input_bool()` (parses "true"/"false" or "1"/"0")
      - `static char input_char()`
      - `static char[] input_chars()` (reads line into char array buffer)
    - Terminal Control:
      - `static void clear()`, `static void flush()`
      - `static void set_color(int ansi_code)`, `static void reset_color()`
      - `static void set_cursor_position(int row, int col)`
      - `static void set_title(char[] title)`
- **Native Implementation (`solixlib/native/src/console.cpp`)**:
  - Cross-platform terminal streams (`std::cout`, `std::cerr`, `std::cin`).
  - Terminal color sequences enabled for POSIX and Windows (Virtual Terminal Processing enabled via `SetConsoleMode`).

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/solixlib/TESTS.md` & `tests/solixlib/test_console.cpp`)**:
  - Add and implement Cases 2.1 to 2.11:
    - **Case 2.1 [Positive]**: `Console.print()` and `Console.println()` with primitive integers, booleans, doubles, and characters.
    - **Case 2.2 [Positive]**: `Console.print()` and `Console.println()` with primitive character arrays (`char[]`).
    - **Case 2.3 [Positive]**: `Console.error()` outputting in ANSI red (`\033[31m`) to standard error.
    - **Case 2.4 [Positive]**: `Console.warning()`, `Console.info()`, and `Console.success()` outputting appropriate ANSI colors.
    - **Case 2.5 [Positive]**: `Console.input_int()` parsing valid integer from redirected stdin.
    - **Case 2.6 [Positive]**: `Console.input_double()` parsing valid double from redirected stdin.
    - **Case 2.7 [Positive]**: `Console.input_bool()` parsing boolean (`true`/`false`).
    - **Case 2.8 [Positive]**: `Console.input_char()` reading single character.
    - **Case 2.9 [Positive]**: `Console.input_chars()` reading line into `char[]` buffer.
    - **Case 2.10 [Negative]**: `Console.input_int()` with non-numeric text throws `FormatException`.
    - **Case 2.11 [Negative]**: `Console.input_double()` with invalid text throws `FormatException`.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/solixlib/console.md`: Formal API and terminal behavior specification.

### Action Items
- [x] Define test specification in `tests/solixlib/TESTS.md` (Cases 2.1 to 2.11).
- [x] Implement `solixlib/native/src/console.cpp` and register hooks in `solixlib/native/src/register.cpp`.
- [x] Author `solixlib/project/src/solix/system/Console.slx`.
- [x] Implement Catch2 test suite `tests/solixlib/test_console.cpp`.
- [x] Author `docs/spec/solixlib/console.md`.
- [x] Verify 100% test pass rate across all platforms.

---

## Phase 23: Standard Library — `solix.core.String` & `StringBuilder` (`IStringable` Contract)

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/project/src/solix/core/String.slx`, `solixlib/project/src/solix/core/StringBuilder.slx`, `solixlib/project/src/solix/core/IStringable.slx`, `solixlib/native/src/string.cpp`, `solixlib/project/src/solix/system/Console.slx`, `tests/solixlib/test_string.cpp`, `docs/spec/solixlib/string.md`
- **Status**: - [ ] Planned

### Objective
Implement the comprehensive, feature-rich immutable text processing abstraction `solix.core.String`, the mutable string accumulator `solix.core.StringBuilder`, and the universal conversion contract `solix.core.IStringable`. Provide fast native static primitive converters (`String.from_*`) and parsers (`String.to_*`), string formatting, joining, padding, and case manipulation. Revisit `solix.system.Console` to print strings and any object implementing `IStringable`.

### Interconnection & Layering
- **Universal Text Contract**: Any type implementing `IStringable` (`string to_string()`) can be converted to text.
- **Fast Native Bidirectional Conversions**:
  - `String.from_int(val)`, `String.from_double(val, precision)`, `String.from_bool(val)`, `String.from_char(val)`, `String.from_chars(arr, offset, count)` implemented in C++ using `<charconv>`.
  - `String.to_int(s)`, `String.to_double(s)`, `String.to_bool(s)`, `String.try_to_int(s, out_val)` throwing `FormatException`.
- **Revisiting Phase 22 (`Console`)**:
  - Add `Console.print(String s)`, `Console.println(String s)`.
  - Add `Console.print(IStringable obj)`, `Console.println(IStringable obj)`.
  - Add `Console.error(String s)`, `Console.warning(String s)`, `Console.info(String s)`, `Console.success(String s)`.
  - Add `Console.input() -> String` (reads whole line as `String`).
- **Downstream Consumer**: Primitives (Phase 24), Collections (Phases 27-31), Filesystem (Phase 32), and Sockets (Phase 37) all rely directly on `String` and `IStringable`.

### Submodule Architecture & Types
- **Solix Surface**:
  - `interface IStringable`: `string to_string();`
  - `class String implements IStringable`:
    - Static Constructors & Conversions (Native `<charconv>`):
      - `static String from_int(int value)`
      - `static String from_double(double value, int precision = 6)`
      - `static String from_bool(bool value)`
      - `static String from_char(char value)`
      - `static String from_chars(char[] chars, int offset, int count)`
      - `static String join(String delimiter, String[] items)`
      - `static String format(String template_str, String[] args)`
      - `static bool is_null_or_empty(String s)`
      - `static bool is_null_or_whitespace(String s)`
    - Static Parsing (Native `<charconv>`):
      - `static int to_int(String s)` (throws `FormatException`)
      - `static double to_double(String s)` (throws `FormatException`)
      - `static bool to_bool(String s)` (throws `FormatException`)
      - `static bool try_to_int(String s, Int out_val)`
      - `static bool try_to_double(String s, Double out_val)`
    - Instance Methods:
      - `int length()`, `bool is_empty()`
      - `char char_at(int index)` (throws `IndexOutOfBoundsException`)
      - `String substring(int start, int length)` (throws `IndexOutOfBoundsException`)
      - `int index_of(String needle)`, `int last_index_of(String needle)`
      - `bool contains(String needle)`, `bool starts_with(String prefix)`, `bool ends_with(String suffix)`
      - `String to_lower()`, `String to_upper()`, `String trim()`, `String trim_start()`, `String trim_end()`
      - `String pad_left(int total_width, char pad_char = ' ')`
      - `String pad_right(int total_width, char pad_char = ' ')`
      - `String repeat(int count)`
      - `String reverse()`
      - `String replace(String old_token, String new_token)`
      - `String[] split(String delimiter)`
      - `String[] lines()`
      - `char[] to_char_array()`
      - `bool equals(String other)`
      - `string to_string()`
  - `class StringBuilder implements IStringable`:
    - `StringBuilder append(String s)`, `StringBuilder append(int val)`, `StringBuilder append(double val)`, `StringBuilder append(char c)`, `StringBuilder append(bool b)`
    - `StringBuilder append_line(String s)`, `StringBuilder append_line()`
    - `StringBuilder insert(int index, String s)`
    - `StringBuilder remove(int start, int length)`
    - `StringBuilder reverse()`
    - `int length()`, `void clear()`, `String to_string()`
- **Native Implementation (`solixlib/native/src/string.cpp`)**:
  - Ultra-fast UTF-8 slicing, `<charconv>` numeric conversions, memory allocation, and search algorithms.

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/solixlib/TESTS.md` & `tests/solixlib/test_string.cpp`)**:
  - Add and implement String test cases:
    - **Case 3.1 [Positive]**: Native bidirectional conversions (`String.from_int(42) == "42"`, `String.to_int("42") == 42`).
    - **Case 3.2 [Positive]**: String formatting, joining with delimiter, padding left and right.
    - **Case 3.3 [Positive]**: String slicing with `substring`, trimming whitespace, prefix/suffix checks.
    - **Case 3.4 [Positive]**: `StringBuilder` chained appends, inserts, removals, and capacity expansion.
    - **Case 3.5 [Positive]**: `Console.println("Hello, Solix!")` and `Console.println(my_stringable)`.
    - **Case 3.6 [Positive]**: `Console.input() -> String` reading from redirected stdin.
    - **Case 3.7 [Negative]**: `String.to_int("invalid")` throws `FormatException`.
    - **Case 3.8 [Negative]**: Out-of-bounds `char_at(-1)` or `char_at(len)` throws `IndexOutOfBoundsException`.
    - **Case 3.9 [Negative]**: Negative substring lengths or start indices beyond string length throw `IndexOutOfBoundsException`.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/solixlib/string.md`: Text encoding, UTF-8 invariants, bidirectional conversions, and `IStringable` protocol.

### Action Items
- [x] Define test specification in `tests/solixlib/TESTS.md`.
- [x] Author `IStringable.slx`, `String.slx`, `StringBuilder.slx`.
- [x] Implement native string primitives and fast `<charconv>` conversions in `solixlib/native/src/string.cpp`.
- [x] Revisit `Console.slx` and `console.cpp` to add string and `IStringable` overloads.
- [x] Implement Catch2 test suite `tests/solixlib/test_string.cpp`.
- [x] Author `docs/spec/solixlib/string.md`.

---

## Phase 24: Standard Library — `solix.core.Primitives` & Types (`Optional<T>`, `Any`, Contracts)

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/project/src/solix/core/IComparable.slx`, `IEquatable.slx`, `ICloneable.slx`, `IHashable.slx`, `Int.slx`, `Double.slx`, `Bool.slx`, `Char.slx`, `Optional.slx`, `Any.slx`, `solixlib/native/src/primitives.cpp`
- **Status**: - [ ] Planned

### Objective
Define universal language contracts (`IComparable<T>`, `IEquatable<T>`, `ICloneable<T>`, `IHashable`), provide boxed object wrappers and string-parsing utilities for primitive types (`Int`, `Double`, `Bool`, `Char`), implement `Optional<T>` for safe absent-value representation, and provide `Any` for general-purpose type encapsulation.

### Interconnection & Layering
- **Foundation for Collections**: `IComparable<T>` enables sorting and priority queues; `IEquatable<T>` and `IHashable` enable `HashMap` and `HashSet`.
- **Extends String & Console**: All boxed primitives, `Optional<T>`, and `Any` implement `IStringable`, allowing direct passing to `Console.println(boxed_val)`.
- **Revisit String**: `String` implements `IComparable<String>`, `IEquatable<String>`, and `IHashable`.
- **Exception Integration**: Parsing errors throw `FormatException`, accessing empty `Optional.value()` throws `InvalidOperationException`.

### Submodule Architecture & Types
- **Solix Surface**:
  - `interface IEquatable<T> { bool equals(T other); }`
  - `interface IComparable<T> { int compare_to(T other); }`
  - `interface IHashable { int hash_code(); }`
  - `interface ICloneable<T> { T clone(); }`
  - `class Int implements IStringable, IEquatable<Int>, IComparable<Int>, IHashable`:
    - `static int parse(String s)` (throws `FormatException`)
    - `static bool try_parse(String s, Int out_val)`
    - `const int MIN_VALUE = -2147483648`, `const int MAX_VALUE = 2147483647`
    - Bitwise operations: `count_leading_zeros(int v)`, `count_trailing_zeros(int v)`, `reverse_bytes(int v)`
  - `class Double implements IStringable, IEquatable<Double>, IComparable<Double>`:
    - `static double parse(String s)` (throws `FormatException`)
    - `static bool is_nan(double d)`, `static bool is_infinite(double d)`
    - Constants: `NaN`, `POSITIVE_INFINITY`, `NEGATIVE_INFINITY`, `MIN_VALUE`, `MAX_VALUE`
  - `class Bool implements IStringable, IEquatable<Bool>`:
    - `static bool parse(String s)` (throws `FormatException`)
  - `class Optional<T> implements IStringable`:
    - `static Optional<T> of(T value)`
    - `static Optional<T> empty()`
    - `bool has_value()`
    - `T value()` (throws `InvalidOperationException` if empty)
    - `T value_or(T fallback)`
  - `class Any implements IStringable`:
    - Encapsulates any object or boxed primitive value with type reflection and `to_string()`.

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/solixlib/TESTS.md` & `tests/solixlib/test_primitives.cpp`)**:
  - **Positive Tests**: Parsing valid numbers; equality checks and compare_to ordering (-1, 0, 1); `Optional.of(value)` has value and unwraps accurately; `Optional.empty().value_or(fallback)` returns fallback; `Any` boxing and string conversion.
  - **Negative Tests**: `Int.parse("abc")` throws `FormatException`; `Int.parse("99999999999999999999")` throws `FormatException` (overflow); accessing `Optional.empty().value()` throws `InvalidOperationException`.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/solixlib/primitives.md`: Universal contracts, parsing rules, and `Optional` semantics.

### Action Items
- [x] Define test specification in `tests/solixlib/TESTS.md`.
- [x] Author contract interfaces (`IComparable`, `IEquatable`, `ICloneable`, `IHashable`).
- [x] Author boxed primitive helper classes (`Int`, `Double`, `Bool`, `Char`, `Optional`, `Any`).
- [x] Implement fast string-to-number parsing in `solixlib/native/src/primitives.cpp` (using `<charconv>`).
- [x] Update `solix.core.String` to implement `IComparable<String>`, `IEquatable<String>`, and `IHashable`.
- [x] Author `docs/spec/solixlib/primitives.md`.


---

## Phase 25: Standard Library — `solix.math.Math` & Numeric Algorithms (`Random`)

- **Priority**: `P2 Medium`
- **Affected Modules**: `solixlib/project/src/solix/math/Math.slx`, `solixlib/project/src/solix/math/Random.slx`, `solixlib/native/src/math.cpp`, `tests/solixlib/test_math.cpp`, `docs/spec/solixlib/math.md`
- **Status**: - [ ] Planned

### Objective
Provide comprehensive mathematical constants, transcendental functions, geometric computations, rounding algorithms, and a cryptographically pseudo-random number generator (`Random`). Backed directly by hardware-accelerated IEEE 754 math and standard C++ `<cmath>` / `<random>` rather than handwritten floating-point math.

### Interconnection & Layering
- **Builds On**: Primitives and Contracts from Phase 24.
- **Cross-Platform Native Math**: Direct bindings to standard C++ `<cmath>` and Mersenne Twister `std::mt19937_64`.
- **Exception Integration**: Throws `IllegalArgumentException` on invalid mathematical bounds (e.g. `Random.next_int(max)` where `max <= 0`).
- **Downstream Use**: Collections shuffling, graphics algorithms, physics computations.

### Submodule Architecture & Types
- **Solix Surface**:
  - `class Math`:
    - Constants: `PI = 3.141592653589793`, `E = 2.718281828459045`, `TAU = 6.283185307179586`, `EPSILON = 1e-15`
    - Basic: `abs`, `min`, `max`, `clamp`, `sign`, `copy_sign`
    - Exponential/Log: `sqrt`, `cbrt`, `hypot`, `pow`, `exp`, `log`, `log10`, `log2`
    - Trigonometric: `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `to_radians`, `to_degrees`
    - Hyperbolic: `sinh`, `cosh`, `tanh`
    - Rounding: `floor`, `ceil`, `round`, `trunc`
  - `class Random`:
    - `Random(long seed)` / `Random()`
    - `int next_int()`, `int next_int(int max)`, `int next_int(int min, int max)`
    - `double next_double()` (0.0 to 1.0)
    - `bool next_bool()`
    - `byte[] next_bytes(int count)`
- **Native Implementation (`solixlib/native/src/math.cpp`)**:
  - Direct C++ `<cmath>` operations and `<random>` (`std::mt19937_64`).

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/solixlib/TESTS.md` & `tests/solixlib/test_math.cpp`)**:
  - **Positive Tests**: High precision verification of trigonometric and exponential functions; deterministic pseudo-random output given identical seeds; clamp boundaries.
  - **Negative Tests**: `Random.next_int(0)` or `Random.next_int(-5)` throws `IllegalArgumentException`; `sqrt(-1.0)` yields `Double.NaN`.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/solixlib/math.md`: Accuracy guarantees, IEEE 754 compliance, and PRNG specifications.

### Action Items
- [x] Define test specification in `tests/solixlib/TESTS.md`.
- [x] Implement `solixlib/native/src/math.cpp` and bind in `register.cpp`.
- [x] Author `solix.math.Math.slx` and `solix.math.Random.slx`.
- [x] Implement Catch2 test suite `tests/solixlib/test_math.cpp`.
- [x] Author `docs/spec/solixlib/math.md`.


---

## Phase 26: Standard Library — `solix.time.Chrono` (`Duration`, `Instant`, `DateTime`, `Stopwatch`)

- **Priority**: `P2 Medium`
- **Affected Modules**: `solixlib/project/src/solix/time/Duration.slx`, `Instant.slx`, `DateTime.slx`, `Stopwatch.slx`, `solixlib/native/src/time.cpp`, `tests/solixlib/test_time.cpp`, `docs/spec/solixlib/time.md`
- **Status**: - [x] Complete

### Objective
Provide high-resolution time measurements, date-time representations with timezone/UTC support, elapsed durations, and benchmarking timers using standard C++20 `<chrono>`.

### Interconnection & Layering
- **Builds On**: Primitives (Phase 24) and String (Phase 23).
- **Implements**: `IStringable`, `IComparable<Duration>`, `IComparable<DateTime>`, `IEquatable`.
- **Downstream Use**: Network timeouts (Phase 37), Filesystem timestamps (Phase 32).

### Submodule Architecture & Types
- **Solix Surface**:
  - `class Duration implements IStringable, IComparable<Duration>, IEquatable<Duration>`:
    - Factory methods: `from_nanoseconds`, `from_milliseconds`, `from_seconds`, `from_minutes`, `from_hours`, `from_days`
    - Arithmetic: `add`, `subtract`
    - Formatting: `to_string() -> String`
  - `class Instant implements IComparable<Instant>, IEquatable<Instant>`:
    - `static Instant now()`, `Duration elapsed()`
  - `class DateTime implements IStringable, IComparable<DateTime>, IEquatable<DateTime>`:
    - `static DateTime now()`, `static DateTime utc_now()`
    - `year()`, `month()`, `day()`, `hour()`, `minute()`, `second()`, `millisecond()`
    - `to_iso8601() -> String`
  - `class Stopwatch`:
    - `void start()`, `void stop()`, `void reset()`, `void restart()`, `Duration elapsed()`, `bool is_running()`
- **Native Implementation (`solixlib/native/src/time.cpp`)**:
  - Standard C++20 `<chrono>` (`system_clock`, `steady_clock`).

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/solixlib/TESTS.md` & `tests/solixlib/test_time.cpp`)**:
  - **Positive Tests**: Monotonic clock advancement during `Stopwatch` run; duration addition/subtraction; UTC and local date component extraction; ISO 8601 string formatting.
  - **Negative Tests**: Subtracting a larger duration from smaller yielding negative duration without underflow; February 29 leap year validation.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/solixlib/time.md`: Monotonic guarantees, epoch definitions, and ISO 8601 syntax.

### Action Items
- [x] Define test specification in `tests/solixlib/TESTS.md`.
- [x] Implement `solixlib/native/src/time.cpp` wrapping C++20 `<chrono>`.
- [x] Author `Duration.slx`, `Instant.slx`, `DateTime.slx`, `Stopwatch.slx`.
- [x] Implement Catch2 test suite `tests/solixlib/test_time.cpp`.
- [x] Author `docs/spec/solixlib/time.md`.

---

## Phase 27: Standard Library — `solix.collections.Core` (Interfaces, `IIterable`, `ICollection`, `to_string`)

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/project/src/solix/collections/IIterable.slx`, `IIterator.slx`, `ICollection.slx`, `IList.slx`, `IReadOnlyCollection.slx`, `IDeque.slx`, `tests/solixlib/test_collections_core.cpp`, `docs/spec/solixlib/collections_core.md`
- **Status**: - [x] Completed & Merged

### Objective
Define the foundational collection architecture in Solix. Establish iterator protocols (`IIterable`, `IIterator`), general collection properties (`ICollection`), linear indexing contracts (`IList`), and double-ended queue contracts (`IDeque`). Standardize the `to_string()` contract across all collection implementations so that any collection is inherently `IStringable` and printable via `Console.println()`.

### Interconnection & Layering
- **Inherits From**: `solix.core.IStringable` (Phase 23). Every collection implements `to_string()`.
- **Prints Directly**: Any collection can be passed directly to `Console.println(collection)`.
- **Exception Integration**: Iterating past end throws `NoSuchElementException`; index access errors throw `IndexOutOfBoundsException`.
- **Foundation For**: Concrete collections in Phases 28, 29, 30, 31.

### Submodule Architecture & Types
- **Solix Surface**:
  - `interface IIterator`: `bool has_next()`, `Any next()` (throws `NoSuchElementException` when exhausted)
  - `interface IIterable`: `IIterator iterator()`
  - `interface ICollection extends IIterable, IStringable`:
    - `int32 size()`, `bool is_empty()`, `bool contains(Any item)`, `void clear()`, `Any[] to_array()`
    - Default `to_string()` formats elements as `"[e1, e2, e3]"`.
  - `interface IList extends ICollection`:
    - `Any get(int32 index)` (throws `IndexOutOfBoundsException`)
    - `Any set(int32 index, Any item)` (throws `IndexOutOfBoundsException`)
    - `void add(Any item)`, `void insert(int32 index, Any item)`, `bool remove(Any item)`, `Any remove_at(int32 index)` (throws `IndexOutOfBoundsException`), `int32 index_of(Any item)`
  - `interface IDeque extends ICollection`:
    - `void add_first(Any item)`, `void add_last(Any item)`
    - `Any remove_first()`, `Any remove_last()` (throws `NoSuchElementException`)
    - `Any peek_first()`, `Any peek_last()` (throws `NoSuchElementException`)

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/solixlib/TESTS.md` & `tests/solixlib/test_collections_core.cpp`)**:
  - **Positive Tests**: Custom class implementing `ICollection` iterated cleanly with `while (it.has_next())`; default `to_string()` output verified across empty (`"[]"`) and multi-item collections; passing collection directly to `Console.println()`.
  - **Negative Tests**: Calling `next()` on an exhausted iterator throws `NoSuchElementException`.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/solixlib/collections_core.md`: Interface contracts, iterator state machine, and string conversion invariants.

### Action Items
- [x] Define test specification in `tests/solixlib/TESTS.md`.
- [x] Author `IIterable.slx`, `IIterator.slx`, `ICollection.slx`, `IList.slx`, `IReadOnlyCollection.slx`, `IDeque.slx`.
- [x] Provide default `to_string()` algorithm for collections in `Collections.slx`.
- [x] Implement Catch2 test suite `tests/solixlib/test_collections_core.cpp`.
- [x] Author `docs/spec/solixlib/collections_core.md`.

---

## Phase 28: Standard Library — `solix.collections.List` (`List` Array & `LinkedList`)

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/project/src/solix/collections/List.slx`, `solixlib/project/src/solix/collections/LinkedList.slx`, `solixlib/project/src/solix/collections/LinkedListNode.slx`, `solixlib/project/src/solix/collections/Algorithms.slx`, `tests/solixlib/test_list.cpp`, `docs/spec/solixlib/list.md`
- **Status**: - [x] Completed & Merged

### Objective
Implement two complementary sequential collection types:
1. `solix.collections.List`: resizable dynamic array with amortized O(1) appending and random access.
2. `solix.collections.LinkedList`: doubly-linked list implementing both `IList` and `IDeque`, providing O(1) insertions and removals at both head and tail.
3. `solix.collections.Algorithms`: static sorting, shuffling, filling, and searching algorithms.

### Interconnection & Layering
- **Builds On**: `IList`, `IDeque` & `ICollection` (Phase 27), `IComparable` & `IEquatable` (Phase 24).
- **Printable**: Automatically formats as `"[item1, item2, ...]"` and prints via `Console.println()`.
- **Exception Integration**: Out-of-bounds indexing throws `IndexOutOfBoundsException`; negative capacity throws `IllegalArgumentException`; empty access throws `NoSuchElementException`.
- **Universal Data Carrier**: Returned by `String.split()`, `filesystem.list_files()`, `Environment.get_args()`.

### Submodule Architecture & Types
- **Solix Surface**:
  - `class List implements IList`:
    - Constructors: `List()`, `List(int32 initial_capacity)`
    - Capacity: `int32 capacity()`, `void ensure_capacity(int32 min_capacity)`, `void shrink_to_fit()`
    - Operations: `get`, `set`, `add`, `insert`, `remove`, `remove_at`, `add_all`, `clear`
    - Searching & Sorting: `index_of`, `contains`, `reverse()`
    - Slicing: `List sub_list(int32 start, int32 count)`
    - Iteration: `IIterator iterator()`
    - String Conversion: `String to_string()`
  - `class LinkedList implements IList, IDeque`:
    - Doubly-linked node chain (`LinkedListNode prev, next; Any value;`)
    - Head & Tail Operations: `add_first(Any item)`, `add_last(Any item)`, `remove_first() -> Any`, `remove_last() -> Any`, `peek_first() -> Any`, `peek_last() -> Any`
    - Indexed Operations: `get(int32 index)`, `set(int32 index, Any item)`, `remove_at(int32 index)` (optimized traversal from closest end)
    - Iteration: `IIterator iterator()`
    - String Conversion: `String to_string()`
  - `class Algorithms`: static reversing, swapping, filling, sorting, and binary searching.

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/solixlib/TESTS.md` & `tests/solixlib/test_list.cpp`)**:
  - **Positive Tests**:
    - `List` dynamic capacity expansion, indexing, and iterator traversal.
    - `List` mutation (insert, remove, sub_list, reverse, to_string).
    - `LinkedList` double-ended queue operations and to_string formatting.
    - `LinkedList` indexed access, mutation, and bidirectional node traversal.
    - `Algorithms` static utilities (swap, reverse, fill, sort_int32, binary_search_int32).
  - **Negative Tests**:
    - `List.get(-1)`, `List.get(size)`, and `List(-1)` throw exceptions.
    - `LinkedList.remove_first()`, `peek_first()`, and `get(0)` on empty throw exceptions.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/solixlib/list.md`: Dynamic array vs doubly-linked list performance characteristics and API documentation.

### Action Items
- [x] Define test specification in `tests/solixlib/TESTS.md`.
- [x] Author `List.slx`, `LinkedList.slx`, `LinkedListNode.slx`, and `Algorithms.slx`.
- [x] Implement Catch2 test suite `tests/solixlib/test_list.cpp`.
- [x] Author `docs/spec/solixlib/list.md`.

---

## Phase 29: Standard Library — `solix.collections.Map` (`HashMap<K, V>` & `TreeMap<K, V>`)

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/project/src/solix/collections/IMap.slx`, `HashMap.slx`, `TreeMap.slx`, `KeyValuePair.slx`, `tests/solixlib/test_map.cpp`, `docs/spec/solixlib/map.md`
- **Status**: - [x] Complete

### Objective
Implement both unordered and ordered associative key-value dictionaries:
1. `solix.collections.HashMap<K, V>`: high-performance hash table with bucket chaining and automatic load-factor rehashing.
2. `solix.collections.TreeMap<K, V>`: balanced binary search tree (Red-Black tree) sorted map ordered by `IComparable<K>`, providing guaranteed O(log N) operations and ordered key ranges.

### Interconnection & Layering
- **Builds On**: `IHashable` and `IEquatable<K>` (Phase 24) for `HashMap`; `IComparable<K>` for `TreeMap`.
- **Printable**: Formats as `"{key1: val1, key2: val2}"` and prints via `Console.println()`.
- **Exception Integration**: Accessing non-existent key via `get(key)` throws `KeyNotFoundException` (or use `get_or_default`).

### Submodule Architecture & Types
- **Solix Surface**:
  - `class KeyValuePair<K, V> implements IStringable`: `K key`, `V value`, `string to_string()`
  - `interface IMap<K, V> extends IStringable`:
    - `V get(K key)` (throws `KeyNotFoundException`), `V get_or_default(K key, V default_val)`
    - `void put(K key, V value)`, `bool contains_key(K key)`, `bool contains_value(V value)`
    - `V remove(K key)`, `int size()`, `bool is_empty()`, `void clear()`
    - `ICollection<K> keys()`, `ICollection<V> values()`, `ICollection<KeyValuePair<K, V>> entries()`
  - `class HashMap<K, V> implements IMap<K, V>`:
    - Load factor (0.75), bucket array, automatic rehashing.
  - `class TreeMap<K, V> implements IMap<K, V>`:
    - Red-Black balanced tree. Keys sorted according to `IComparable<K>`.
    - Range methods: `K first_key()`, `K last_key()`, `IMap<K, V> sub_map(K from_key, K to_key)`.

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/solixlib/TESTS.md` & `tests/solixlib/test_map.cpp`)**:
  - **Positive Tests**:
    - `HashMap`: Putting, updating, getting, and rehashing across large insertions.
    - `TreeMap`: Keys traversed in strict sorted order; `first_key()` and `last_key()` retrieval.
    - Formatted string representation printed via `Console.println()`.
  - **Negative Tests**: Retrieving non-existent key throws `KeyNotFoundException`; empty `first_key()` throws `NoSuchElementException`.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/solixlib/map.md`: Hash distribution vs Red-Black tree complexity, load factors, and API specifications.

### Action Items
- [x] Define test specification in `tests/solixlib/TESTS.md`.
- [x] Author `IMap.slx`, `KeyValuePair.slx`, `HashMap.slx`, and `TreeMap.slx`.
- [x] Implement Catch2 test suite `tests/solixlib/test_map.cpp`.
- [x] Author `docs/spec/solixlib/map.md`.

---

## Phase 30: Standard Library — `solix.collections.Set` (`HashSet<T>` & `TreeSet<T>`)

- **Priority**: `P2 Medium`
- **Affected Modules**: `solixlib/project/src/solix/collections/ISet.slx`, `HashSet.slx`, `TreeSet.slx`, `tests/solixlib/test_set.cpp`, `docs/spec/solixlib/set.md`
- **Status**: - [x] Completed & Merged

### Objective
Implement distinct element containers:
1. `solix.collections.HashSet<T>`: backed by hash table for O(1) uniqueness.
2. `solix.collections.TreeSet<T>`: backed by Red-Black tree for sorted unique elements with O(log N) operations.

### Interconnection & Layering
- **Builds On**: `ICollection<T>` (Phase 27), `IHashable` & `IEquatable<T>` (Phase 24) for `HashSet`, `IComparable<T>` for `TreeSet`.
- **Printable**: Formats as `"{item1, item2, item3}"` and prints via `Console.println()`.

### Submodule Architecture & Types
- **Solix Surface**:
  - `interface ISet<T> extends ICollection<T>`:
    - `bool add(T item)` (returns true if added, false if duplicate)
    - `bool remove(T item)`, `bool contains(T item)`
    - `void union_with(ICollection<T> other)`, `void intersect_with(ICollection<T> other)`, `void difference_with(ICollection<T> other)`
    - `bool is_subset_of(ICollection<T> other)`, `bool is_superset_of(ICollection<T> other)`
  - `class HashSet<T> implements ISet<T>`: hash-table backed.
  - `class TreeSet<T> implements ISet<T>`: Red-Black tree backed, maintains elements in sorted order.

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/solixlib/TESTS.md` & `tests/solixlib/test_set.cpp`)**:
  - **Positive Tests**:
    - Uniqueness enforcement: duplicates rejected.
    - Set mathematical operations: union, intersection, difference, subset checks.
    - `TreeSet` traversing elements in strict ascending order.
  - **Negative Tests**: Concurrent modification detection during iteration throws `InvalidOperationException`.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/solixlib/set.md`: Set theory operations, hash vs tree implementations, and complexity guarantees.

### Action Items
- [x] Define test specification in `tests/solixlib/TESTS.md`.
- [x] Author `ISet.slx`, `HashSet.slx`, `TreeSet.slx`.
- [x] Implement Catch2 test suite `tests/solixlib/test_set.cpp`.
- [x] Author `docs/spec/solixlib/set.md`.

---

## Phase 31: Standard Library — `solix.collections.Linear` (`Stack`, `Queue`, `Deque`, `PriorityQueue`, `CircularBuffer`, `BitSet`)

- **Priority**: `P2 Medium`
- **Affected Modules**: `solixlib/project/src/solix/collections/Stack.slx`, `Queue.slx`, `Deque.slx`, `PriorityQueue.slx`, `CircularBuffer.slx`, `BitSet.slx`, `tests/solixlib/test_linear_collections.cpp`, `docs/spec/solixlib/linear_collections.md`
- **Status**: - [x] Completed & Merged

### Objective
Implement comprehensive specialized linear data structures and buffers:
- LIFO `Stack<T>`
- FIFO `Queue<T>`
- Double-ended `Deque<T>`
- Binary-heap `PriorityQueue<T>`
- Fixed-capacity FIFO `CircularBuffer<T>` (Ring Buffer)
- Compact, high-performance `BitSet` (bit array)

### Interconnection & Layering
- **Builds On**: `ICollection<T>` (Phase 27) and `IComparable<T>` (Phase 24) for priority ordering.
- **Exception Integration**: Empty pop, dequeue, or peek throws `InvalidOperationException`.
- **Printable**: Formats elements sequentially and prints via `Console.println()`.

### Submodule Architecture & Types
- **Solix Surface**:
  - `class Stack<T> implements ICollection<T>`: `push`, `pop`, `peek`, `size`, `is_empty`
  - `class Queue<T> implements ICollection<T>`: `enqueue`, `dequeue`, `peek`, `size`, `is_empty`
  - `class Deque<T> implements ICollection<T>`: `push_front`, `push_back`, `pop_front`, `pop_back`, `peek_front`, `peek_back`
  - `class PriorityQueue<T> implements ICollection<T>`: `enqueue`, `dequeue`, `peek` (binary heap)
  - `class CircularBuffer<T> implements ICollection<T>`: fixed capacity ring buffer with overwrite or blocking modes.
  - `class BitSet implements IStringable`:
    - `void set(int bit_index)`, `void clear(int bit_index)`, `bool get(int bit_index)`
    - `void and(BitSet other)`, `void or(BitSet other)`, `void xor(BitSet other)`
    - `int cardinality()`, `int size()`

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/solixlib/TESTS.md` & `tests/solixlib/test_linear_collections.cpp`)**:
  - **Positive Tests**:
    - Strict LIFO order for `Stack`, strict FIFO order for `Queue`.
    - Priority extraction order from `PriorityQueue`.
    - `CircularBuffer` wrapping around capacity boundaries cleanly.
    - `BitSet` bitwise operations (AND, OR, XOR) and cardinality counting.
  - **Negative Tests**: `pop()` or `peek()` on empty stack/queue throws `InvalidOperationException`.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/solixlib/linear_collections.md`: Performance complexity and buffer behaviors.

### Action Items
- [x] Define test specification in `tests/solixlib/TESTS.md`.
- [x] Author `Stack.slx`, `Queue.slx`, `Deque.slx`, `PriorityQueue.slx`, `CircularBuffer.slx`, `BitSet.slx`.
- [x] Implement Catch2 test suite `tests/solixlib/test_linear_collections.cpp`.
- [x] Author `docs/spec/solixlib/linear_collections.md`.

---

## Phase 32: Standard Library — `solix.io.filesystem` (Unified Path & File System Operations)

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/project/src/solix/io/filesystem/`, `solixlib/native/src/io_fs.cpp`, `tests/solixlib/test_filesystem.cpp`, `docs/spec/solixlib/filesystem.md`
- **Status**: - [x] Complete

### Objective
Unify all path manipulation and filesystem operations into a single cohesive package `solix.io.filesystem`. Expose static and object-oriented file, directory, and path APIs backed natively by cross-platform C++20 `<filesystem>`.

### Interconnection & Layering
- **Builds On**: `String` (Phase 23), `List<String>` (Phase 28), `DateTime` (Phase 26).
- **Exception Integration**: Throws `FileNotFoundException` when target file does not exist, and `IOException` on permissions or filesystem failures.
- **Cross-Platform**: Normalizes Windows (`\`) and POSIX (`/`) separators automatically.

### Submodule Architecture & Types
- **Solix Surface (`solixlib/project/src/solix/io/filesystem/`)**:
  - `class Path`:
    - `const char DIRECTORY_SEPARATOR`
    - `static String combine(String path1, String path2)`
    - `static String get_directory_name(String path)`
    - `static String get_file_name(String path)`
    - `static String get_extension(String path)`
    - `static String get_file_name_without_extension(String path)`
    - `static bool is_absolute(String path)`
    - `static String get_temp_path()`
    - `static String normalize(String path)`
  - `class File`:
    - `static bool exists(String path)`
    - `static String read_all_text(String path)` (throws `FileNotFoundException`, `IOException`)
    - `static void write_all_text(String path, String contents)` (throws `IOException`)
    - `static List<String> read_all_lines(String path)` (throws `FileNotFoundException`, `IOException`)
    - `static void append_all_text(String path, String contents)` (throws `IOException`)
    - `static void delete(String path)`
    - `static void copy(String src, String dest, bool overwrite = false)`
    - `static void move(String src, String dest)`
    - `static long get_size(String path)`
    - `static DateTime get_last_modified_time(String path)`
  - `class Directory`:
    - `static bool exists(String path)`
    - `static void create_directory(String path)`
    - `static void delete(String path, bool recursive = false)`
    - `static List<String> list_files(String path)`
    - `static List<String> list_directories(String path)`
    - `static String get_current_directory()`
- **Native Implementation (`solixlib/native/src/io_fs.cpp`)**:
  - C++20 `std::filesystem::path`, `std::filesystem::create_directories`, `std::ifstream`, `std::ofstream`.

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/solixlib/TESTS.md` & `tests/solixlib/test_filesystem.cpp`)**:
  - **Positive Tests**: Writing text to a temporary file, verifying existence, reading text back, and deleting; path joining across Windows and Unix slash conventions; creating nested directories and listing directory contents.
  - **Negative Tests**: Reading non-existent file throws `FileNotFoundException`; deleting non-empty directory without recursive flag throws `IOException`.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/solixlib/filesystem.md`: Filesystem path contracts, unified package structure, and cross-platform permissions.

### Action Items
- [x] Define test specification in `tests/solixlib/TESTS.md`.
- [x] Implement `solixlib/native/src/io_fs.cpp` using `std::filesystem`.
- [x] Author unified `filesystem` classes under `solixlib/project/src/solix/io/filesystem/`.
- [x] Implement Catch2 test suite `tests/solixlib/test_filesystem.cpp`.
- [x] Author `docs/spec/solixlib/filesystem.md`.

---

## Phase 32.1: Emergency Refactor — Purge `Any.slx`, Modernize `solix.math.Math` with Generics & `Optional<T>`

- **Priority**: `P0 Blocker`
- **Affected Modules**: `solixlib/project/src/solix/core/Any.slx`, `solixlib/project/src/solix/core/Optional.slx`, `solixlib/project/src/solix/math/Math.slx`, `tests/solixlib/test_math.cpp`, `tests/solixlib/test_primitives.cpp`, `docs/spec/solixlib/primitives.md`, `docs/spec/solixlib/math.md`
- **Status**: - [x] Complete

### Objective
Completely remove `Any.slx` from the standard library to eliminate untyped union overhead and ARC tracking pitfalls. Modernize `solix.math.Math` with generic static methods (`abs<T>`, `min<T>`, `max<T>`, `clamp<T>`, `sign<T>`) supporting arbitrary numeric types (`int32`, `int64`, `float64`). Enhance `Optional<T>` with functional lambda helpers (`if_present`, `filter`).

### Action Items
- [x] Modernize `solixlib/project/src/solix/math/Math.slx` with generic math templates:
  - `public static T abs<T>(T v)`
  - `public static T min<T>(T a, T b)`
  - `public static T max<T>(T a, T b)`
  - `public static T clamp<T>(T val, T min_val, T max_val)`
  - `public static int32 sign<T>(T v)`
  - `public static T copy_sign<T>(T magnitude, T sign_val)`
- [x] Add functional lambda helpers to `Optional<T>`:
  - `public void if_present(void(*)(T) consumer)`
  - `public Optional<T> filter(bool(*)(T) predicate)`
- [x] Purge `Any` from `tests/solixlib/test_primitives.cpp` and update `tests/solixlib/test_math.cpp`.
- [x] Update `tests/solixlib/TESTS.md` and documentation in `docs/spec/solixlib/math.md` and `primitives.md`.

---

## Phase 32.2: Emergency Refactor — Generic Collections Sequences (`List<T>`, `LinkedList<T>`, `Collections`, `Algorithms`) with Lambdas

- **Priority**: `P0 Blocker`
- **Affected Modules**: `solixlib/project/src/solix/collections/List.slx`, `LinkedList.slx`, `LinkedListNode.slx`, `Collections.slx`, `Algorithms.slx`, `tests/solixlib/test_list.cpp`, `test_collections_core.cpp`, `docs/spec/solixlib/list.md`
- **Status**: - [x] Complete

### Objective
Convert `List` and `LinkedList` to full generic classes `List<T>` and `LinkedList<T>`. Add first-class lambda iteration (`for_each(void(*)(T) action)`) and transformation methods (`filter(bool(*)(T) predicate)`). Modernize `Collections` and `Algorithms` with generic methods (`sort<T>`, `binary_search<T>`, `reverse<T>`, `swap<T>`).

### Action Items
- [x] Refactor `List.slx` to `List<T>` backed by `T[] _data`.
- [x] Implement `for_each(void(*)(T) action)` and `filter(bool(*)(T) predicate)` in `List<T>`.
- [x] Refactor `LinkedList.slx` & `LinkedListNode.slx` to `LinkedList<T>` with `LinkedListNode<T>`.
- [x] Implement `for_each(void(*)(T) action)` in `LinkedList<T>`.
- [x] Refactor `Collections.slx` and `Algorithms.slx` for generic `List<T>`.
- [x] Update `tests/solixlib/test_list.cpp` and `test_collections_core.cpp`.
- [x] Update `tests/solixlib/TESTS.md` and documentation in `docs/spec/solixlib/list.md`.

---

## Phase 32.3: Emergency Refactor — Generic Linear Containers (`Stack<T>`, `Queue<T>`, `Deque<T>`, `PriorityQueue<T>`, `CircularBuffer<T>`)

- **Priority**: `P0 Blocker`
- **Affected Modules**: `solixlib/project/src/solix/collections/Stack.slx`, `Queue.slx`, `Deque.slx`, `PriorityQueue.slx`, `CircularBuffer.slx`, `tests/solixlib/test_linear_collections.cpp`, `docs/spec/solixlib/linear_collections.md`
- **Status**: - [x] Complete

### Objective
Convert all linear collections and streaming buffers to compile-time generic classes. Implement `for_each(void(*)(T) action)` across all linear structures, enable comparator lambdas (`int32(*)(T, T)`) in `PriorityQueue<T>`, and eliminate all remaining references to `Any`.

### Action Items
- [x] Refactor `Stack.slx` to `Stack<T>` with `for_each`.
- [x] Refactor `Queue.slx` to `Queue<T>` with `for_each`.
- [x] Refactor `Deque.slx` to `Deque<T>` with `for_each`.
- [x] Refactor `PriorityQueue.slx` to `PriorityQueue<T>` with comparator lambda constructor and `for_each`.
- [x] Refactor `CircularBuffer.slx` to `CircularBuffer<T>` backed by `T[]` with `for_each`.
- [x] Update `tests/solixlib/test_linear_collections.cpp`.
- [x] Update `tests/solixlib/TESTS.md` and documentation in `docs/spec/solixlib/linear_collections.md`.

---

## Phase 32.4: Emergency Refactor — Generic Associative Containers (`KeyValuePair<K, V>`, `HashMap<K, V>`, `TreeMap<K, V>`, `HashSet<T>`, `TreeSet<T>`)

- **Priority**: `P0 Blocker`
- **Affected Modules**: `solixlib/project/src/solix/collections/KeyValuePair.slx`, `HashMapEntry.slx`, `HashMap.slx`, `TreeMapNode.slx`, `TreeMap.slx`, `HashSet.slx`, `TreeSet.slx`, `tests/solixlib/test_map.cpp`, `test_set.cpp`, `docs/spec/solixlib/map.md`, `set.md`
- **Status**: - [x] Complete
 
### Objective
Convert all associative dictionaries and distinct sets to generic types parameterized on key and value. Implement `for_each(void(*)(K, V) action)` for maps and `for_each(void(*)(T) action)` for sets. Ensure typed view collections (`keys(): List<K>`, `values(): List<V>`, `entries(): List<KeyValuePair<K, V>>`).

### Action Items
- [x] Refactor `KeyValuePair.slx` and `HashMapEntry.slx` to generic pairs/entries.
- [x] Refactor `HashMap.slx` to `HashMap<K, V>` with `for_each(void(*)(K, V) action)`.
- [x] Refactor `TreeMapNode.slx` and `TreeMap.slx` to `TreeMap<K, V>` with `for_each(void(*)(K, V) action)`.
- [x] Refactor `HashSet.slx` to `HashSet<T>` backed by `HashMap<T, bool>` with `for_each(void(*)(T) action)`.
- [x] Refactor `TreeSet.slx` to `TreeSet<T>` backed by `TreeMap<T, bool>` with `for_each(void(*)(T) action)`.
- [x] Update `tests/solixlib/test_map.cpp` and `tests/solixlib/test_set.cpp`.
- [x] Update `tests/solixlib/TESTS.md` and documentation in `docs/spec/solixlib/map.md` and `set.md`.

---

## Phase 32.5: Emergency Refactor — Filesystem & Downstream Alignment, Specs & Full Regression

- **Priority**: `P0 Blocker`
- **Affected Modules**: `solixlib/project/src/solix/io/filesystem/File.slx`, `Directory.slx`, `tests/solixlib/test_filesystem.cpp`, `docs/spec/solixlib/filesystem.md`, full test harness
- **Status**: - [ ] Planned

### Objective
Align `File.slx` and `Directory.slx` to return typed `List<String>` rather than untyped/Any collections. Update `tests/solixlib/test_filesystem.cpp` to verify direct string element operations without casts. Rebuild `solixlib.slxbin` and run the entire CTest regression suite to achieve 100% passing across all 61 tests.

### Action Items
- [ ] Update `File.read_all_lines` to return `List<String>`.
- [ ] Update `Directory.list_files` and `Directory.list_directories` to return `List<String>`.
- [ ] Update `tests/solixlib/test_filesystem.cpp`.
- [ ] Author/update `docs/spec/solixlib/filesystem.md`.
- [ ] Run full regression suite `ctest --test-dir build --output-on-failure` (100% pass required).

---

## Phase 33: Standard Library — `solix.io.Streams` (`IStream`, `FileStream`, `MemoryStream`, Readers/Writers)

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/project/src/solix/io/IStream.slx`, `FileStream.slx`, `MemoryStream.slx`, `TextReader.slx`, `TextWriter.slx`, `BinaryReader.slx`, `BinaryWriter.slx`, `solixlib/native/src/io_stream.cpp`, `tests/solixlib/test_streams.cpp`, `docs/spec/solixlib/streams.md`
- **Status**: - [ ] Planned

### Objective
Implement the low-level byte-oriented and character-oriented streaming architecture: `IStream`, `FileStream`, `MemoryStream`, `TextReader`/`TextWriter`, and binary serializers `BinaryReader`/`BinaryWriter`.

### Interconnection & Layering
- **Builds On**: `String` (Phase 23), byte arrays, and `filesystem` (Phase 32).
- **Exception Integration**: Throws `InvalidOperationException` on reading from closed stream, `IndexOutOfBoundsException` on buffer offsets.
- **Downstream Use**: Network socket streams (Phase 37), Cryptographic hashing (Phase 36).

### Submodule Architecture & Types
- **Solix Surface**:
  - `interface IStream`:
    - `int read(byte[] buffer, int offset, int count)`
    - `void write(byte[] buffer, int offset, int count)`
    - `long seek(long offset, int origin)` (Origin: Begin, Current, End)
    - `void flush()`, `void close()`
    - `long length()`, `long position()`
  - `class FileStream implements IStream`: file modes (Read, Write, Append, ReadWrite).
  - `class MemoryStream implements IStream`: in-memory resizable byte array stream with `byte[] to_array()`.
  - `class TextReader`: `read_line() -> String`, `read_to_end() -> String`, `peek() -> int`.
  - `class TextWriter`: `write(String s)`, `write_line(String s)`, `flush()`.
  - `class BinaryReader`: `read_int()`, `read_double()`, `read_bool()`, `read_string()`.
  - `class BinaryWriter`: `write_int(int v)`, `write_double(double v)`, `write_string(String s)`.

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/solixlib/TESTS.md` & `tests/solixlib/test_streams.cpp`)**:
  - **Positive Tests**: `MemoryStream` write, seek to 0, read back byte buffers; `TextWriter` and `TextReader` line-by-line round-trip; binary serialization and deserialization of mixed primitive values.
  - **Negative Tests**: Reading from a closed stream throws `InvalidOperationException`; writing to a read-only stream throws `InvalidOperationException`.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/solixlib/streams.md`: Stream lifecycle, seeking rules, and buffering mechanisms.

### Action Items
- [ ] Define test specification in `tests/solixlib/TESTS.md`.
- [ ] Implement `solixlib/native/src/io_stream.cpp` wrapping OS file descriptors.
- [ ] Author `IStream.slx`, `FileStream.slx`, `MemoryStream.slx`, `TextReader.slx`, `TextWriter.slx`, `BinaryReader.slx`, `BinaryWriter.slx`.
- [ ] Implement Catch2 test suite `tests/solixlib/test_streams.cpp`.
- [ ] Author `docs/spec/solixlib/streams.md`.

---

## Phase 34: Standard Library — `solix.system.Environment` (OS, Env, Subprocesses)

- **Priority**: `P2 Medium`
- **Affected Modules**: `solixlib/project/src/solix/system/Environment.slx`, `Process.slx`, `ProcessResult.slx`, `solixlib/native/src/system.cpp`, `tests/solixlib/test_environment.cpp`, `docs/spec/solixlib/environment.md`
- **Status**: - [ ] Planned

### Objective
Provide access to host runtime environment variables, command-line arguments, operating system identification, and child subprocess spawning. Natively backed by established lightweight cross-platform C/C++ libraries (e.g. `reproc` or `subprocess.h`) for guaranteed pipe redirection and process lifecycle handling across Linux, macOS, and Windows.

### Interconnection & Layering
- **Builds On**: `String` (Phase 23), `List<String>` (Phase 28), `Map<String, String>` (Phase 29).
- **Cross-Platform Native Subprocesses**: Powered by battle-tested C library (`reproc` / `subprocess.h`), avoiding raw unmaintainable OS syscalls.
- **Printable**: Process output and environment info printable via `Console`.

### Submodule Architecture & Types
- **Solix Surface**:
  - `class Environment`:
    - `static List<String> get_args()`
    - `static String get_env(String key)`
    - `static void set_env(String key, String value)`
    - `static Map<String, String> get_all_env()`
    - `static String os_name()`, `static String os_version()`
    - `static bool is_windows()`, `static bool is_linux()`, `static bool is_macos()`
    - `static int processor_count()`
  - `class ProcessResult`:
    - `int exit_code`, `String standard_output`, `String standard_error`
  - `class Process`:
    - `static ProcessResult run(String command, List<String> args)`
    - `void start()`, `int wait_for_exit()`, `void kill()`
- **Native Implementation (`solixlib/native/src/system.cpp`)**:
  - Cross-platform process runner via established single-header library (`subprocess.h` / `reproc`).

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/solixlib/TESTS.md` & `tests/solixlib/test_environment.cpp`)**:
  - **Positive Tests**: Setting and retrieving process-local environment variables; spawning a child echo process and capturing its standard output; OS detection matching current build host platform.
  - **Negative Tests**: Spawning a non-existent binary returns non-zero error status without crashing parent process.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/solixlib/environment.md`: Environment variable security and process lifecycle.

### Action Items
- [ ] Define test specification in `tests/solixlib/TESTS.md`.
- [ ] Implement `solixlib/native/src/system.cpp` with cross-platform process execution.
- [ ] Author `Environment.slx`, `Process.slx`, `ProcessResult.slx`.
- [ ] Implement Catch2 test suite `tests/solixlib/test_environment.cpp`.
- [ ] Author `docs/spec/solixlib/environment.md`.

---

## Phase 35: Core Compiler & Runtime — Language Intrinsics (`assert`, `exit`, Hardcoded Built-ins)

- **Priority**: `P1 High`
- **Affected Modules**: `core/src/processes/lexer.cpp`, `core/src/processes/parser.cpp`, `core/src/processes/binder.cpp`, `core/src/processes/assembler.cpp`, `core/src/runtime.cpp`, `tests/statements/expressions/test_intrinsics.cpp`, `docs/spec/statements/intrinsics.md`
- **Status**: - [ ] Planned

### Objective
Implement built-in hardcoded language expressions in the Solix core compiler and VM (modeled similarly to `instanceof` and `sizeof`), specifically `assert(condition, message)` and `exit(code)`. Enable zero-dependency language-level assertions that throw `AssertionError` (or abort) and immediate program termination without requiring library imports.

### Interconnection & Layering
- **Language Integration**: Parsed as first-class expressions or statements in `core/`.
- **Exception Integration**: Failed `assert` throws `solix.exceptions.AssertionError` (or triggers runtime trap in test runner).
- **Self-Testing Substrate**: All subsequent standard library modules and tests can use built-in `assert` directly in Solix source files.

### Submodule Architecture & Types
- **Language Syntax**:
  - `assert <expression>;` or `assert(<condition>, <message>);`
  - `exit(<int_expression>);`
- **Compiler Pipeline**:
  - Lexer: Recognize `assert` and `exit` as reserved keywords.
  - Parser: Parse `AssertStatementNode` / `ExitStatementNode` with condition and optional message expressions.
  - Binder: Type-check condition (must resolve to `bool`) and message (must resolve to `String` or primitive).
  - Assembler: Emit `OP_ASSERT` (or test-and-branch to throw/trap) and `OP_EXIT`.
  - Runtime VM: Handle opcode execution.

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/statements/TESTS.md` & `tests/statements/expressions/test_intrinsics.cpp`)**:
  - **Positive Tests**: `assert true;` passes silently; `assert(1 + 1 == 2, "Math holds");` executes without overhead.
  - **Negative Tests**: `assert false;` throws `AssertionError` with source line and message; `assert 123;` rejected at compile time with type mismatch.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/statements/intrinsics.md`: Specification of `assert` and `exit` keywords, bytecode semantics, and compilation rules.

### Action Items
- [ ] Define test specification in `tests/statements/TESTS.md`.
- [ ] Add `assert` and `exit` tokens in `core/src/utilities/token.hpp` and lexer.
- [ ] Implement AST nodes, parser rules, binder validation, assembler bytecode, and VM runtime dispatch.
- [ ] Implement Catch2 unit tests in `tests/statements/expressions/test_intrinsics.cpp`.
- [ ] Author `docs/spec/statements/intrinsics.md`.

---

## Phase 36: Standard Library — `solix.crypto` (Base64, Hex, SHA-256, MD5)

- **Priority**: `P3 Low`
- **Affected Modules**: `solixlib/project/src/solix/crypto/Base64.slx`, `Hex.slx`, `Hash.slx`, `solixlib/native/src/crypto.cpp`, `tests/solixlib/test_crypto.cpp`, `docs/spec/solixlib/crypto.md`
- **Status**: - [ ] Planned

### Objective
Implement essential cryptographic hashing functions (SHA-256, SHA-1, MD5) and binary encodings (Base64, Hexadecimal) using well-established, battle-tested lightweight C libraries (e.g. Brad Conte's standard `crypto-algorithms` or `monocypher`) rather than handwriting cryptographic math from scratch.

### Interconnection & Layering
- **Builds On**: `String` (Phase 23), byte arrays, and `Streams` (Phase 33).
- **Proven C Libraries**: Integrates standard, audited public-domain C cryptographic implementations for SHA-256, SHA-1, and MD5.
- **Exception Integration**: Malformed Base64 or odd-length hex strings throw `FormatException`.
- **Downstream Use**: Package integrity verification, cache checksums, HTTP basic authentication, security digests.

### Submodule Architecture & Types
- **Solix Surface**:
  - `class Base64`:
    - `static String encode(byte[] data)`
    - `static String encode_string(String text)`
    - `static byte[] decode(String base64)` (throws `FormatException`)
    - `static String decode_to_string(String base64)` (throws `FormatException`)
  - `class Hex`:
    - `static String encode(byte[] data)`
    - `static byte[] decode(String hex_string)` (throws `FormatException`)
  - `class Hash`:
    - `static byte[] sha256(byte[] data)`
    - `static String sha256_hex(String text)`
    - `static String sha1_hex(String text)`
    - `static String md5_hex(String text)`
- **Native Implementation (`solixlib/native/src/crypto.cpp`)**:
  - Standard, audited C implementations of FIPS 180-4 SHA-256, SHA-1, RFC 1321 MD5, and RFC 4648 Base64.

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/solixlib/TESTS.md` & `tests/solixlib/test_crypto.cpp`)**:
  - **Positive Tests**: NIST test vectors for SHA-256 (hash of `"abc"` matches expected hex digest); Base64 encoding and decoding round-trips with padding (=, ==) and unpadded multiples of 3.
  - **Negative Tests**: Invalid Base64 characters or malformed padding throw `FormatException`; odd-length hex string throws `FormatException`.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/solixlib/crypto.md`: Cryptographic algorithms, test vector validations, and security boundaries.

### Action Items
- [ ] Define test specification in `tests/solixlib/TESTS.md`.
- [ ] Embed audited C crypto library in `solixlib/native/src/crypto.cpp`.
- [ ] Author `Base64.slx`, `Hex.slx`, `Hash.slx`.
- [ ] Implement Catch2 test suite `tests/solixlib/test_crypto.cpp`.
- [ ] Author `docs/spec/solixlib/crypto.md`.

---

## Phase 37: Standard Library — `solix.net` (TCP & UDP Sockets, Lightweight `HttpClient`)

- **Priority**: `P3 Low`
- **Affected Modules**: `solixlib/project/src/solix/net/IPAddress.slx`, `IPEndPoint.slx`, `TcpClient.slx`, `TcpListener.slx`, `UdpClient.slx`, `UdpReceiveResult.slx`, `HttpClient.slx`, `HttpResponse.slx`, `solixlib/native/src/net.cpp`, `tests/solixlib/test_net.cpp`, `docs/spec/solixlib/net.md`
- **Status**: - [ ] Planned

### Objective
Implement cross-platform networking primitives: IP address handling, TCP client/server streaming sockets, connectionless UDP datagram transmission (`UdpClient`), and a lightweight HTTP client for REST API consumption. Leverages established cross-platform BSD/Winsock abstractions and fast HTTP parsing (`picohttpparser`).

### Interconnection & Layering
- **Builds On**: `IStream` (Phase 33) for socket read/write, `String` (Phase 23), `Map<String, String>` (Phase 29) for headers.
- **Exception Integration**: Throws `SocketException` on network failures, and `FormatException` on invalid IP strings.
- **Cross-Platform**: Berkeley sockets on POSIX, Winsock (`WSAStartup`) on Windows.

### Submodule Architecture & Types
- **Solix Surface**:
  - `class IPAddress`:
    - `static IPAddress parse(String ip_string)` (throws `FormatException`)
    - `static IPAddress loopback()`, `static IPAddress any()`
    - `String to_string()`
  - `class IPEndPoint`:
    - `IPAddress address`, `int port`
  - `class TcpClient`:
    - `void connect(String host, int port)` (throws `SocketException`)
    - `IStream get_stream()`, `void close()`, `bool is_connected()`
  - `class TcpListener`:
    - `void start(int port)` (throws `SocketException`)
    - `TcpClient accept()` (throws `SocketException`), `void stop()`
  - `class UdpReceiveResult`:
    - `byte[] buffer`, `IPEndPoint remote_endpoint`
  - `class UdpClient`:
    - `void bind(int port)` (throws `SocketException`)
    - `int send(byte[] data, IPEndPoint endpoint)` (throws `SocketException`)
    - `UdpReceiveResult receive()` (throws `SocketException`), `void close()`
  - `class HttpResponse`:
    - `int status_code`, `Map<String, String> headers`, `String body`
  - `class HttpClient`:
    - `HttpResponse get(String url)` (throws `SocketException`)
    - `HttpResponse post(String url, String body, String content_type = "application/json")` (throws `SocketException`)
- **Native Implementation (`solixlib/native/src/net.cpp`)**:
  - Cross-platform non-blocking TCP and UDP socket abstraction and minimal HTTP/1.1 client.

### Identified Test & Documentation Deliverables
- **1. Identified Test Deliverables (`tests/solixlib/TESTS.md` & `tests/solixlib/test_net.cpp`)**:
  - **Positive Tests**:
    - Parsing IPv4 strings (`127.0.0.1`).
    - Local loopback TCP client/server connection and streaming data echo.
    - Local loopback UDP client sending and receiving datagrams.
    - HTTP response header and status parsing.
  - **Negative Tests**:
    - Connecting to a non-existent port throws `SocketException` within timeout.
    - Invalid IP address string throws `FormatException`.
- **2. Identified Documentation Deliverables**:
  - `docs/spec/solixlib/net.md`: Network protocol support, TCP/UDP socket lifecycle, state diagrams, and timeouts.

### Action Items
- [ ] Define test specification in `tests/solixlib/TESTS.md`.
- [ ] Implement cross-platform socket primitives (TCP and UDP) in `solixlib/native/src/net.cpp`.
- [ ] Author `IPAddress.slx`, `IPEndPoint.slx`, `TcpClient.slx`, `TcpListener.slx`, `UdpClient.slx`, `UdpReceiveResult.slx`, `HttpClient.slx`, `HttpResponse.slx`.
- [ ] Implement Catch2 test suite `tests/solixlib/test_net.cpp`.
- [ ] Author `docs/spec/solixlib/net.md`.













