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
| **Phase 21** | Standard Library: `solix.system.Console` (Foundational Terminal I/O) | `P1 High` | `solixlib`, `solixlib/native`, `tests` | - [ ] Planned |
| **Phase 22** | Standard Library: `solix.core.String` & `StringBuilder` (`IStringable` Contract) | `P1 High` | `solixlib`, `solixlib/native`, `tests` | - [ ] Planned |
| **Phase 23** | Standard Library: `solix.core.Primitives` & Core Contracts (`IComparable`, `IEquatable`, etc.) | `P1 High` | `solixlib`, `tests` | - [ ] Planned |
| **Phase 24** | Standard Library: `solix.math.Math` & Numeric Algorithms (`Random`) | `P2 Medium` | `solixlib`, `solixlib/native`, `tests` | - [ ] Planned |
| **Phase 25** | Standard Library: `solix.time.Chrono` (`Duration`, `Instant`, `DateTime`, `Stopwatch`) | `P2 Medium` | `solixlib`, `solixlib/native`, `tests` | - [ ] Planned |
| **Phase 26** | Standard Library: `solix.collections.Core` (Interfaces, `IIterable`, `ICollection`, `to_string`) | `P1 High` | `solixlib`, `tests` | - [ ] Planned |
| **Phase 27** | Standard Library: `solix.collections.List` (Dynamic Array / ArrayList) | `P1 High` | `solixlib`, `tests` | - [ ] Planned |
| **Phase 28** | Standard Library: `solix.collections.Map` & `HashMap` (Associative Key-Value Store) | `P1 High` | `solixlib`, `tests` | - [ ] Planned |
| **Phase 29** | Standard Library: `solix.collections.Set` & `HashSet` (Distinct Element Container) | `P2 Medium` | `solixlib`, `tests` | - [ ] Planned |
| **Phase 30** | Standard Library: `solix.collections.Linear` (`Stack`, `Queue`, `Deque`, `PriorityQueue`) | `P2 Medium` | `solixlib`, `tests` | - [ ] Planned |
| **Phase 31** | Standard Library: `solix.io.Path` & `FileSystem` (Files, Directories, Metadata) | `P1 High` | `solixlib`, `solixlib/native`, `tests` | - [ ] Planned |
| **Phase 32** | Standard Library: `solix.io.Streams` (`IStream`, `FileStream`, `MemoryStream`, Readers/Writers) | `P1 High` | `solixlib`, `solixlib/native`, `tests` | - [ ] Planned |
| **Phase 33** | Standard Library: `solix.system.Environment` & `Process` (OS, Env, Subprocesses) | `P2 Medium` | `solixlib`, `solixlib/native`, `tests` | - [ ] Planned |
| **Phase 34** | Standard Library: `solix.diagnostics` (`Assert`, `Logger`, Benchmarking) | `P2 Medium` | `solixlib`, `tests` | - [ ] Planned |
| **Phase 35** | Standard Library: `solix.data.Json` (JSON Parsing, Serialization, DOM) | `P2 Medium` | `solixlib`, `solixlib/native`, `tests` | - [ ] Planned |
| **Phase 36** | Standard Library: `solix.crypto` (Base64, Hex, SHA-256, MD5) | `P3 Low` | `solixlib`, `solixlib/native`, `tests` | - [ ] Planned |
| **Phase 37** | Standard Library: `solix.concurrent` (`Thread`, `Mutex`, `LockGuard`, `AtomicInt`) | `P2 Medium` | `solixlib`, `solixlib/native`, `tests` | - [ ] Planned |
| **Phase 38** | Standard Library: `solix.net` (TCP/UDP Sockets & Lightweight `HttpClient`) | `P3 Low` | `solixlib`, `solixlib/native`, `tests` | - [ ] Planned |

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

## Phase 21: Standard Library — `solix.system.Console` (Foundational Terminal I/O)

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/project/src/solix/system/Console.slx`, `solixlib/native/src/console.cpp`, `solixlib/native/CMakeLists.txt`, `tests/commands/test_package_commands.cpp`
- **Status**: - [ ] Planned

### Objective
Implement the foundational terminal I/O module `solix.system.Console` allowing Solix applications to write primitives to `stdout` and `stderr`, read single characters from `stdin`, clear the console screen, flush streams, and manipulate terminal colors. This phase establishes the lowest-level I/O substrate using raw primitives before `String` or object abstractions exist.

### Interconnection & Layering
- **Immediate Capability**: Prints primitive types (`int`, `long`, `double`, `bool`, `char`) and raw C strings via native hooks.
- **Future Revisit (Phase 22)**: When `solix.core.String` and `IStringable` are implemented, `Console` will be revisited to introduce `print(String)`, `print_line(String)`, `print(IStringable)`, `print_line(IStringable)`, and `read_line() -> String`.
- **Future Revisit (Phase 26)**: When `ICollection<T>` is introduced, `Console` will print collections formatted as strings.

### Submodule Architecture & Types
- **Solix Surface**:
  - `class Console`:
    - `static void print(int value)`
    - `static void print(double value)`
    - `static void print(bool value)`
    - `static void print(char value)`
    - `static void print_line(int value)`
    - `static void print_line(double value)`
    - `static void print_line(bool value)`
    - `static void print_line(char value)`
    - `static void print_line()`
    - `static void print_error(int value)`
    - `static void print_line_error(int value)`
    - `static int read_char()`
    - `static void flush()`
    - `static void set_color(int ansi_code)`
    - `static void reset_color()`
- **Native Implementation (`solixlib/native/src/console.cpp`)**:
  - Standard C/C++ cross-platform terminal streams (`std::cout`, `std::cerr`, `std::cin`).
  - Terminal color sequences enabled for POSIX and Windows (Virtual Terminal Processing enabled via `SetConsoleMode`).

### Identified Test & Documentation Deliverables
- **1. Positive Test Scenarios**:
  - Printing integers, negative integers, zero, floating-point numbers, booleans (`true`/`false`), and characters to stdout.
  - `print_line()` outputs proper newline (`\n` on Unix, `\r\n` on Windows).
  - Outputting to stderr via `print_error` and `print_line_error`.
  - Color setting and resetting without crashing or terminal corruption.
- **2. Negative Test Scenarios**:
  - Extreme values (Int.MIN_VALUE, Int.MAX_VALUE, NaN, Infinity) printed accurately without SIGFPE or buffer overflow.
  - Flush on closed or redirected file descriptors handled gracefully without unhandled exceptions.
- **3. Documentation Deliverables**:
  - `docs/spec/solixlib/console.md`: Formal API and terminal behavior specification.

### Action Items
- [ ] Implement `solixlib/native/src/console.cpp` and register hooks in `solixlib/native/src/register.cpp`.
- [ ] Author `solixlib/project/src/solix/system/Console.slx`.
- [ ] Add integration test in `tests/commands/test_package_commands.cpp` building and running a Solix project that invokes `Console`.
- [ ] Author `docs/spec/solixlib/console.md`.
- [ ] Verify 100% test pass rate across all platforms.

---

## Phase 22: Standard Library — `solix.core.String` & `StringBuilder` (`IStringable` Contract)

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/project/src/solix/core/String.slx`, `solixlib/project/src/solix/core/StringBuilder.slx`, `solixlib/project/src/solix/core/IStringable.slx`, `solixlib/native/src/string.cpp`, `solixlib/project/src/solix/system/Console.slx`
- **Status**: - [ ] Planned

### Objective
Implement the immutable text processing abstraction `solix.core.String`, the mutable string accumulator `solix.core.StringBuilder`, and the universal conversion contract `solix.core.IStringable`. Immediately revisit and upgrade `solix.system.Console` to print strings and any object implementing `IStringable`.

### Interconnection & Layering
- **Universal Text Contract**: Any type implementing `IStringable` (`string to_string()`) can be converted to text.
- **Revisiting Phase 21 (`Console`)**:
  - Add `Console.print(String s)`, `Console.print_line(String s)`.
  - Add `Console.print(IStringable obj)`, `Console.print_line(IStringable obj)`.
  - Add `Console.read_line() -> String`.
- **Downstream Consumer**: Primitives (Phase 23), Collections (Phase 26), Path (Phase 31), JSON (Phase 35) all rely directly on `String` and `IStringable`.

### Submodule Architecture & Types
- **Solix Surface**:
  - `interface IStringable`: `string to_string();`
  - `class String implements IStringable`:
    - `int length()`
    - `bool is_empty()`
    - `char char_at(int index)`
    - `String substring(int start, int length)`
    - `int index_of(String needle)`
    - `int last_index_of(String needle)`
    - `bool contains(String needle)`
    - `bool starts_with(String prefix)`
    - `bool ends_with(String suffix)`
    - `String to_lower()`
    - `String to_upper()`
    - `String trim()`
    - `String replace(String old_token, String new_token)`
    - `String[] split(String delimiter)`
    - `bool equals(String other)`
    - `string to_string()`
  - `class StringBuilder implements IStringable`:
    - `StringBuilder append(String s)`
    - `StringBuilder append(int val)`
    - `StringBuilder append(double val)`
    - `StringBuilder append(char c)`
    - `StringBuilder append_line(String s)`
    - `StringBuilder append_line()`
    - `int length()`
    - `void clear()`
    - `String to_string()`
- **Native Implementation (`solixlib/native/src/string.cpp`)**:
  - UTF-8 validation, slicing, case-mapping, search, and dynamic byte buffer management.

### Identified Test & Documentation Deliverables
- **1. Positive Test Scenarios**:
  - String concatenation, slicing with `substring`, trimming whitespace, prefix/suffix checks.
  - `StringBuilder` chained appends and capacity expansion.
  - `Console.print_line("Hello, Solix!")` and `Console.print_line(my_stringable)`.
  - `Console.read_line()` reading from redirected stdin.
- **2. Negative Test Scenarios**:
  - Out-of-bounds `char_at(-1)` or `char_at(len)` triggers runtime exception.
  - Negative substring lengths or start indices beyond string length rejected safely.
  - `split` with empty delimiter handled predictably.
- **3. Documentation Deliverables**:
  - `docs/spec/solixlib/string.md`: Text encoding, UTF-8 invariants, and `IStringable` protocol.

### Action Items
- [ ] Author `solix.core.IStringable.slx`, `String.slx`, `StringBuilder.slx`.
- [ ] Implement native string primitives in `solixlib/native/src/string.cpp`.
- [ ] Revisit `solix.system.Console.slx` and `console.cpp` to add string and `IStringable` overloads.
- [ ] Add unit and integration tests covering string manipulation, `StringBuilder`, and upgraded `Console`.
- [ ] Author `docs/spec/solixlib/string.md`.

---

## Phase 23: Standard Library — `solix.core.Primitives` & Fundamental Contracts

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/project/src/solix/core/IComparable.slx`, `IEquatable.slx`, `ICloneable.slx`, `IHashable.slx`, `Int.slx`, `Double.slx`, `Bool.slx`, `Char.slx`, `Nullable.slx`
- **Status**: - [ ] Planned

### Objective
Define universal language contracts (`IComparable<T>`, `IEquatable<T>`, `ICloneable<T>`, `IHashable`) and provide boxed object wrappers and parsing utilities for primitive types (`Int`, `Double`, `Bool`, `Char`) along with `Nullable<T>`.

### Interconnection & Layering
- **Foundation for Collections**: `IComparable<T>` enables sorting and priority queues; `IEquatable<T>` and `IHashable` enable `HashMap` and `HashSet`.
- **Extends String & Console**: All boxed primitives implement `IStringable`, allowing direct passing to `Console.print_line(boxed_val)`.
- **Revisit String**: `String` implements `IComparable<String>`, `IEquatable<String>`, and `IHashable`.

### Submodule Architecture & Types
- **Solix Surface**:
  - `interface IEquatable<T> { bool equals(T other); }`
  - `interface IComparable<T> { int compare_to(T other); }`
  - `interface IHashable { int hash_code(); }`
  - `interface ICloneable<T> { T clone(); }`
  - `class Int implements IStringable, IEquatable<Int>, IComparable<Int>, IHashable`:
    - `static int parse(String s)`
    - `static bool try_parse(String s, Int out_val)`
    - `const int MIN_VALUE = -2147483648`
    - `const int MAX_VALUE = 2147483647`
  - `class Double implements IStringable, IEquatable<Double>, IComparable<Double>`:
    - `static double parse(String s)`
    - `static bool is_nan(double d)`, `static bool is_infinite(double d)`
  - `class Bool implements IStringable, IEquatable<Bool>`:
    - `static bool parse(String s)`
  - `class Nullable<T> implements IStringable`:
    - `bool has_value()`, `T value()`, `T value_or(T fallback)`

### Identified Test & Documentation Deliverables
- **1. Positive Test Scenarios**:
  - `Int.parse("12345") == 12345`, `Double.parse("3.14159")`.
  - Equality checks and compare_to ordering (-1, 0, 1).
  - Hash code stability across identical values.
  - `Nullable<T>` unwrapping and fallback evaluation.
- **2. Negative Test Scenarios**:
  - `Int.parse("abc")` throws `FormatException`.
  - `Int.parse("99999999999999999999")` throws `OverflowException`.
  - Accessing `Nullable<T>.value()` when `has_value() == false` throws `InvalidOperationException`.
- **3. Documentation Deliverables**:
  - `docs/spec/solixlib/primitives.md`: Universal contracts, parsing rules, and error handling.

### Action Items
- [ ] Author contract interfaces (`IComparable`, `IEquatable`, `ICloneable`, `IHashable`).
- [ ] Author boxed primitive helper classes (`Int`, `Double`, `Bool`, `Char`, `Nullable`).
- [ ] Implement fast string-to-number parsing in `solixlib/native/src/primitives.cpp` (using `<charconv>`).
- [ ] Update `solix.core.String` to implement `IComparable<String>`, `IEquatable<String>`, and `IHashable`.
- [ ] Add positive and negative test cases.

---

## Phase 24: Standard Library — `solix.math.Math` & Numeric Algorithms (`Random`)

- **Priority**: `P2 Medium`
- **Affected Modules**: `solixlib/project/src/solix/math/Math.slx`, `solixlib/project/src/solix/math/Random.slx`, `solixlib/native/src/math.cpp`
- **Status**: - [ ] Planned

### Objective
Provide comprehensive mathematical constants, transcendental functions, geometric computations, rounding algorithms, and a cryptographically pseudo-random number generator (`Random`).

### Interconnection & Layering
- **Builds On**: Primitives from Phase 23.
- **Printable**: Formats floating-point outputs via `Double.to_string()` and outputs via `Console`.
- **Downstream Use**: Collections shuffling, graphics algorithms, physics computations.

### Submodule Architecture & Types
- **Solix Surface**:
  - `class Math`:
    - Constants: `PI = 3.141592653589793`, `E = 2.718281828459045`, `TAU = 6.283185307179586`
    - Basic: `abs(double v)`, `min(double a, double b)`, `max(double a, double b)`, `clamp(double val, double min, double max)`
    - Exponential/Log: `sqrt(double v)`, `cbrt(double v)`, `pow(double b, double exp)`, `exp(double v)`, `log(double v)`, `log10(double v)`, `log2(double v)`
    - Trigonometric: `sin(double rad)`, `cos(double rad)`, `tan(double rad)`, `asin(double v)`, `acos(double v)`, `atan(double v)`, `atan2(double y, double x)`, `to_radians(double deg)`, `to_degrees(double rad)`
    - Rounding: `floor(double v)`, `ceil(double v)`, `round(double v)`, `trunc(double v)`
  - `class Random`:
    - `Random(long seed)` / `Random()`
    - `int next_int()`, `int next_int(int max)`, `int next_int(int min, int max)`
    - `double next_double()` (0.0 to 1.0)
    - `bool next_bool()`
- **Native Implementation (`solixlib/native/src/math.cpp`)**:
  - Direct C++ `<cmath>` operations and `<random>` (Mersenne Twister `std::mt19937_64`).

### Identified Test & Documentation Deliverables
- **1. Positive Test Scenarios**:
  - High precision verification of trigonometric and exponential functions.
  - Deterministic pseudo-random output given identical seeds.
  - `clamp`, `min`, `max` boundaries.
- **2. Negative Test Scenarios**:
  - `sqrt(-1.0)` produces `Double.NaN`.
  - `log(0.0)` produces negative infinity.
  - `Random.next_int(max)` where `max <= 0` throws `ArgumentException`.
- **3. Documentation Deliverables**:
  - `docs/spec/solixlib/math.md`: Accuracy guarantees, IEEE 754 compliance, and PRNG specifications.

### Action Items
- [ ] Implement `solixlib/native/src/math.cpp` and bind in `register.cpp`.
- [ ] Author `solix.math.Math.slx` and `solix.math.Random.slx`.
- [ ] Add mathematical accuracy and edge-case unit tests.
- [ ] Document in `docs/spec/solixlib/math.md`.

---

## Phase 25: Standard Library — `solix.time.Chrono` (`Duration`, `Instant`, `DateTime`, `Stopwatch`)

- **Priority**: `P2 Medium`
- **Affected Modules**: `solixlib/project/src/solix/time/Duration.slx`, `Instant.slx`, `DateTime.slx`, `Stopwatch.slx`, `solixlib/native/src/time.cpp`
- **Status**: - [ ] Planned

### Objective
Provide high-resolution time measurements, date-time representations with timezone/UTC support, elapsed durations, and benchmarking timers.

### Interconnection & Layering
- **Builds On**: Primitives (Phase 23) and String (Phase 22).
- **Implements**: `IStringable`, `IComparable<Duration>`, `IComparable<DateTime>`, `IEquatable`.
- **Downstream Use**: Diagnostics & Benchmarks (Phase 34), Thread sleeping (Phase 37), Network timeouts (Phase 38).

### Submodule Architecture & Types
- **Solix Surface**:
  - `class Duration implements IStringable, IComparable<Duration>, IEquatable<Duration>`:
    - Factory methods: `from_nanoseconds(long ns)`, `from_milliseconds(long ms)`, `from_seconds(long s)`, `from_minutes(long m)`, `from_hours(long h)`, `from_days(long d)`
    - Properties: `total_nanoseconds()`, `total_milliseconds()`, `total_seconds()`
    - Arithmetic: `add(Duration other)`, `subtract(Duration other)`
    - Formatting: `to_string() -> String` (e.g. `"1h 23m 45s"`)
  - `class Instant implements IComparable<Instant>, IEquatable<Instant>`:
    - `static Instant now()`
    - `Duration elapsed()`
  - `class DateTime implements IStringable, IComparable<DateTime>, IEquatable<DateTime>`:
    - `static DateTime now()`, `static DateTime utc_now()`
    - Components: `year()`, `month()`, `day()`, `hour()`, `minute()`, `second()`, `millisecond()`
    - ISO 8601 string formatting: `to_iso8601() -> String`
  - `class Stopwatch`:
    - `void start()`, `void stop()`, `void reset()`, `void restart()`
    - `Duration elapsed()`, `bool is_running()`
- **Native Implementation (`solixlib/native/src/time.cpp`)**:
  - Standard C++20 `<chrono>` (`std::chrono::system_clock`, `std::chrono::steady_clock`).

### Identified Test & Documentation Deliverables
- **1. Positive Test Scenarios**:
  - Monotonic clock strictly advancing during `Stopwatch` execution.
  - Duration arithmetic: `Duration.from_seconds(30).add(Duration.from_seconds(30)) == Duration.from_minutes(1)`.
  - UTC and local date component extraction and ISO 8601 serialization.
- **2. Negative Test Scenarios**:
  - Subtracting a larger duration from a smaller duration yielding negative duration without underflow.
  - Leap year edge cases (February 29 validation).
- **3. Documentation Deliverables**:
  - `docs/spec/solixlib/time.md`: Monotonic guarantees, epoch definitions, and ISO 8601 syntax.

### Action Items
- [ ] Implement `solixlib/native/src/time.cpp` wrapping C++20 `<chrono>`.
- [ ] Author `Duration.slx`, `Instant.slx`, `DateTime.slx`, `Stopwatch.slx`.
- [ ] Add positive/negative tests for time math and benchmarking.
- [ ] Author `docs/spec/solixlib/time.md`.

---

## Phase 26: Standard Library — `solix.collections.Core` (Interfaces, `IIterable`, `ICollection`, `to_string`)

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/project/src/solix/collections/IIterable.slx`, `IIterator.slx`, `ICollection.slx`, `IList.slx`, `IReadOnlyCollection.slx`
- **Status**: - [ ] Planned

### Objective
Define the foundational collection architecture in Solix. Establish iterator protocols (`IIterable<T>`, `IIterator<T>`), general collection properties (`ICollection<T>`), and linear indexing contracts (`IList<T>`). Standardize the `to_string()` contract across all collection implementations so that any collection is inherently `IStringable` and printable via `Console.print_line()`.

### Interconnection & Layering
- **Inherits From**: `solix.core.IStringable` (Phase 22). Every collection implements `to_string()`.
- **Prints Directly**: Any collection can be passed directly to `Console.print_line(collection)`.
- **Foundation For**: Concrete collections in Phases 27, 28, 29, 30.

### Submodule Architecture & Types
- **Solix Surface**:
  - `interface IIterator<T>`:
    - `bool has_next()`
    - `T next()`
  - `interface IIterable<T>`:
    - `IIterator<T> iterator()`
  - `interface ICollection<T> extends IIterable<T>, IStringable`:
    - `int size()`
    - `bool is_empty()`
    - `bool contains(T item)`
    - `void clear()`
    - `T[] to_array()`
    - Default `to_string()` formats elements as `"[e1, e2, e3]"`.
  - `interface IList<T> extends ICollection<T>`:
    - `T get(int index)`
    - `void set(int index, T item)`
    - `void add(T item)`
    - `void insert(int index, T item)`
    - `bool remove(T item)`
    - `T remove_at(int index)`
    - `int index_of(T item)`
  - `interface IReadOnlyCollection<T> extends IIterable<T>, IStringable`:
    - `int size()`, `bool is_empty()`, `bool contains(T item)`

### Identified Test & Documentation Deliverables
- **1. Positive Test Scenarios**:
  - Mock collection implementing `ICollection<T>` iterated cleanly with `while (it.has_next())`.
  - Formatted `to_string()` output verified across empty (`"[]"`) and multi-item collections.
  - Passing `ICollection<T>` directly to `Console.print_line()`.
- **2. Negative Test Scenarios**:
  - Calling `next()` on an exhausted iterator throws `NoSuchElementException`.
- **3. Documentation Deliverables**:
  - `docs/spec/solixlib/collections_core.md`: Interface contracts, iterator state machine, and string conversion invariants.

### Action Items
- [ ] Author `IIterable.slx`, `IIterator.slx`, `ICollection.slx`, `IList.slx`, `IReadOnlyCollection.slx`.
- [ ] Provide default `to_string()` algorithm for collections.
- [ ] Write contract conformance unit tests.
- [ ] Author `docs/spec/solixlib/collections_core.md`.

---

## Phase 27: Standard Library — `solix.collections.List` (Dynamic Array / ArrayList)

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/project/src/solix/collections/List.slx`, `solixlib/project/src/solix/collections/Algorithms.slx`
- **Status**: - [ ] Planned

### Objective
Implement the primary general-purpose resizable dynamic array `solix.collections.List<T>` implementing `IList<T>`. Provide amortized O(1) appending, growth factor resizing, sorting, binary search, and list transformations.

### Interconnection & Layering
- **Builds On**: `IList<T>` & `ICollection<T>` (Phase 26), `IComparable<T>` & `IEquatable<T>` (Phase 23).
- **Printable**: Automatically formats as `"[item1, item2, ...]"` and prints via `Console.print_line()`.
- **Universal Data Carrier**: Returned by `String.split()`, `Directory.list_files()`, `Environment.get_args()`, and `Json.parse()`.

### Submodule Architecture & Types
- **Solix Surface**:
  - `class List<T> implements IList<T>`:
    - Constructors: `List()`, `List(int initial_capacity)`
    - Capacity: `int capacity()`, `void ensure_capacity(int min_capacity)`, `void shrink_to_fit()`
    - Element Operations: `get(index)`, `set(index, item)`, `add(item)`, `insert(index, item)`, `remove(item)`, `remove_at(index)`
    - Bulk Operations: `add_all(ICollection<T> items)`, `clear()`
    - Searching & Sorting: `int index_of(item)`, `bool contains(item)`, `void sort()`, `void reverse()`, `int binary_search(item)`
    - Slicing: `List<T> sub_list(int start, int count)`
    - Iteration: `IIterator<T> iterator()`
    - String Conversion: `string to_string()`
  - `class Collections`:
    - Static utility methods: `sort<T>(IList<T> list)`, `reverse<T>(IList<T> list)`, `binary_search<T>(IList<T> list, T key)`.

### Identified Test & Documentation Deliverables
- **1. Positive Test Scenarios**:
  - Resizing across capacity boundaries (e.g. adding 10,000 items).
  - Insertion and removal at head, middle, and tail.
  - In-place sorting of integer and string lists.
  - `Console.print_line(list)` printing expected bracketed output.
- **2. Negative Test Scenarios**:
  - `get(index)` with negative index or `index >= size()` throws `IndexOutOfBoundsException`.
  - `remove_at(index)` on empty list throws `IndexOutOfBoundsException`.
  - Negative initial capacity in constructor throws `ArgumentException`.
- **3. Documentation Deliverables**:
  - `docs/spec/solixlib/list.md`: Dynamic array memory characteristics, amortized complexity, and methods.

### Action Items
- [ ] Author `solix.collections.List.slx` and `Algorithms.slx`.
- [ ] Implement array growth and shrink-to-fit logic.
- [ ] Implement sorting (quicksort or timsort) and binary search.
- [ ] Add stress tests (10k+ items) and index-boundary tests.
- [ ] Author `docs/spec/solixlib/list.md`.

---

## Phase 28: Standard Library — `solix.collections.Map` & `HashMap` (Associative Key-Value Store)

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/project/src/solix/collections/IMap.slx`, `HashMap.slx`, `KeyValuePair.slx`
- **Status**: - [ ] Planned

### Objective
Implement the associative hash map `solix.collections.HashMap<K, V>` implementing `IMap<K, V>`. Support efficient key-value lookups, automatic bucket rehashing based on load factor, key and value collection views, and `{k1: v1, k2: v2}` string formatting.

### Interconnection & Layering
- **Builds On**: `IHashable` and `IEquatable<K>` (Phase 23) for hash bucket indexing and key collision resolution.
- **Printable**: Formats as `"{key1: val1, key2: val2}"` and prints via `Console.print_line()`.
- **Collection Views**: Exposes `keys() -> ICollection<K>` and `values() -> ICollection<V>`.
- **Downstream Use**: Environment variables (Phase 33), HTTP headers (Phase 38), JSON objects (Phase 35).

### Submodule Architecture & Types
- **Solix Surface**:
  - `class KeyValuePair<K, V> implements IStringable`:
    - `K key`, `V value`
    - `string to_string()`
  - `interface IMap<K, V> extends IStringable`:
    - `V get(K key)`
    - `void put(K key, V value)`
    - `bool contains_key(K key)`
    - `bool contains_value(V value)`
    - `V remove(K key)`
    - `int size()`, `bool is_empty()`, `void clear()`
    - `ICollection<K> keys()`, `ICollection<V> values()`, `ICollection<KeyValuePair<K, V>> entries()`
  - `class HashMap<K, V> implements IMap<K, V>`:
    - Constructors: `HashMap()`, `HashMap(int initial_capacity, double load_factor = 0.75)`
    - Collision resolution via separate chaining or open addressing with Robin Hood hashing.
    - Automatic rehashing when load factor is exceeded.
    - String formatting: `to_string() -> String`.

### Identified Test & Documentation Deliverables
- **1. Positive Test Scenarios**:
  - Putting, updating, and getting keys.
  - Rehashing during large insertions without losing data.
  - Key and value view iteration.
  - Formatted `{k: v}` string printed via `Console.print_line()`.
- **2. Negative Test Scenarios**:
  - Retrieving non-existent key throws `KeyNotFoundException` (or returns fallback via `get_or_default`).
  - Handling forced hash collisions safely.
- **3. Documentation Deliverables**:
  - `docs/spec/solixlib/map.md`: Hash distribution requirements, load factor behavior, and API contract.

### Action Items
- [ ] Author `IMap.slx`, `KeyValuePair.slx`, and `HashMap.slx`.
- [ ] Implement bucket hashing and dynamic table resizing.
- [ ] Add collision and stress tests.
- [ ] Author `docs/spec/solixlib/map.md`.

---

## Phase 29: Standard Library — `solix.collections.Set` & `HashSet` (Distinct Element Container)

- **Priority**: `P2 Medium`
- **Affected Modules**: `solixlib/project/src/solix/collections/ISet.slx`, `HashSet.slx`
- **Status**: - [ ] Planned

### Objective
Implement the distinct element container `solix.collections.HashSet<T>` implementing `ISet<T>`, providing uniqueness guarantees and set algebra (union, intersection, difference, subset checks).

### Interconnection & Layering
- **Builds On**: `ICollection<T>` (Phase 26), `IHashable` & `IEquatable<T>` (Phase 23).
- **Printable**: Formats as `"{item1, item2, item3}"` and prints via `Console.print_line()`.

### Submodule Architecture & Types
- **Solix Surface**:
  - `interface ISet<T> extends ICollection<T>`:
    - `bool add(T item)`
    - `bool remove(T item)`
    - `bool contains(T item)`
    - `void union_with(ICollection<T> other)`
    - `void intersect_with(ICollection<T> other)`
    - `void difference_with(ICollection<T> other)`
    - `bool is_subset_of(ICollection<T> other)`
  - `class HashSet<T> implements ISet<T>`:
    - Backed by an internal `HashMap<T, bool>`.
    - Formats `to_string()` as `"{item1, item2}"`.

### Identified Test & Documentation Deliverables
- **1. Positive Test Scenarios**:
  - Duplicate additions return `false` and maintain single instance.
  - Set union, intersection, and difference mathematical verification.
  - Printing set via `Console.print_line()`.
- **2. Negative Test Scenarios**:
  - Modifying collection during iteration throws `ConcurrentModificationException`.
- **3. Documentation Deliverables**:
  - `docs/spec/solixlib/set.md`: Set theory operations and uniqueness semantics.

### Action Items
- [ ] Author `ISet.slx` and `HashSet.slx`.
- [ ] Implement set algebra methods (`union_with`, `intersect_with`, `difference_with`).
- [ ] Write positive uniqueness and mathematical set operation tests.
- [ ] Author `docs/spec/solixlib/set.md`.

---

## Phase 30: Standard Library — `solix.collections.Linear` (`Stack`, `Queue`, `Deque`, `PriorityQueue`)

- **Priority**: `P2 Medium`
- **Affected Modules**: `solixlib/project/src/solix/collections/Stack.slx`, `Queue.slx`, `Deque.slx`, `PriorityQueue.slx`
- **Status**: - [ ] Planned

### Objective
Implement specialized linear data structures: LIFO `Stack<T>`, FIFO `Queue<T>`, double-ended `Deque<T>`, and binary-heap `PriorityQueue<T>`.

### Interconnection & Layering
- **Builds On**: `ICollection<T>` (Phase 26) and `IComparable<T>` (Phase 23) for priority ordering.
- **Printable**: Formats elements sequentially and prints via `Console.print_line()`.

### Submodule Architecture & Types
- **Solix Surface**:
  - `class Stack<T> implements ICollection<T>`: `push(T item)`, `T pop()`, `T peek()`, `int size()`, `bool is_empty()`
  - `class Queue<T> implements ICollection<T>`: `enqueue(T item)`, `T dequeue()`, `T peek()`, `int size()`, `bool is_empty()`
  - `class Deque<T> implements ICollection<T>`: `push_front`, `push_back`, `pop_front`, `pop_back`, `peek_front`, `peek_back`
  - `class PriorityQueue<T> implements ICollection<T>`: `enqueue(T item)`, `T dequeue()`, `T peek()` (ordered by `compare_to` or min-heap invariant)

### Identified Test & Documentation Deliverables
- **1. Positive Test Scenarios**:
  - Strict LIFO order verification for `Stack`.
  - Strict FIFO order verification for `Queue`.
  - Highest-priority-first extraction from `PriorityQueue`.
- **2. Negative Test Scenarios**:
  - `pop()` or `peek()` on empty stack/queue throws `InvalidOperationException`.
- **3. Documentation Deliverables**:
  - `docs/spec/solixlib/linear_collections.md`: Performance complexity and buffer behaviors.

### Action Items
- [ ] Author `Stack.slx`, `Queue.slx`, `Deque.slx`, `PriorityQueue.slx`.
- [ ] Implement binary heap array for `PriorityQueue`.
- [ ] Add order validation and empty-state exception tests.
- [ ] Author `docs/spec/solixlib/linear_collections.md`.

---

## Phase 31: Standard Library — `solix.io.Path` & `FileSystem` (Files, Directories, Metadata)

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/project/src/solix/io/Path.slx`, `File.slx`, `Directory.slx`, `FileInfo.slx`, `solixlib/native/src/io_fs.cpp`
- **Status**: - [ ] Planned

### Objective
Implement cross-platform filesystem management and path manipulation via `solix.io.Path`, `solix.io.File`, and `solix.io.Directory` using standard C++20 `<filesystem>` natively.

### Interconnection & Layering
- **Builds On**: `String` (Phase 22), `List<String>` (Phase 27), `DateTime` (Phase 25).
- **Cross-Platform**: Normalizes Windows (`\`) and POSIX (`/`) separators automatically.

### Submodule Architecture & Types
- **Solix Surface**:
  - `class Path`:
    - `const char DIRECTORY_SEPARATOR`
    - `static String combine(String path1, String path2)`
    - `static String get_directory_name(String path)`
    - `static String get_file_name(String path)`
    - `static String get_extension(String path)`
    - `static String get_file_name_without_extension(String path)`
    - `static bool is_absolute(String path)`
    - `static String get_temp_path()`
  - `class File`:
    - `static bool exists(String path)`
    - `static String read_all_text(String path)`
    - `static void write_all_text(String path, String contents)`
    - `static List<String> read_all_lines(String path)`
    - `static void append_all_text(String path, String contents)`
    - `static void delete(String path)`
    - `static void copy(String src, String dest, bool overwrite = false)`
    - `static void move(String src, String dest)`
    - `static long get_size(String path)`
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
- **1. Positive Test Scenarios**:
  - Writing text to a temporary file, verifying existence, reading text back, and deleting.
  - Path joining across both Windows and Unix slash conventions.
  - Creating nested directories and listing directory contents.
- **2. Negative Test Scenarios**:
  - Reading non-existent file throws `FileNotFoundException`.
  - Deleting non-empty directory without recursive flag throws `IOException`.
- **3. Documentation Deliverables**:
  - `docs/spec/solixlib/io_fs.md`: Filesystem path contracts and cross-platform permissions.

### Action Items
- [ ] Implement `solixlib/native/src/io_fs.cpp` using `std::filesystem`.
- [ ] Author `Path.slx`, `File.slx`, `Directory.slx`.
- [ ] Write file reading, writing, and directory listing tests in temporary directories.
- [ ] Author `docs/spec/solixlib/io_fs.md`.

---

## Phase 32: Standard Library — `solix.io.Streams` (`IStream`, `FileStream`, `MemoryStream`, Readers/Writers)

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/project/src/solix/io/IStream.slx`, `FileStream.slx`, `MemoryStream.slx`, `TextReader.slx`, `TextWriter.slx`, `BinaryReader.slx`, `BinaryWriter.slx`, `solixlib/native/src/io_stream.cpp`
- **Status**: - [ ] Planned

### Objective
Implement the low-level byte-oriented and character-oriented streaming architecture: `IStream`, `FileStream`, `MemoryStream`, `TextReader`/`TextWriter`, and binary serializers `BinaryReader`/`BinaryWriter`.

### Interconnection & Layering
- **Builds On**: `String` (Phase 22), byte arrays, and `Path` (Phase 31).
- **Downstream Use**: Network communication (Phase 38), Cryptographic hashing (Phase 36), Structured logging (Phase 34).

### Submodule Architecture & Types
- **Solix Surface**:
  - `interface IStream`:
    - `int read(byte[] buffer, int offset, int count)`
    - `void write(byte[] buffer, int offset, int count)`
    - `long seek(long offset, int origin)` (Origin: Begin, Current, End)
    - `void flush()`, `void close()`
    - `long length()`, `long position()`
  - `class FileStream implements IStream`:
    - Modes: Read, Write, Append, ReadWrite.
  - `class MemoryStream implements IStream`:
    - In-memory resizable byte array stream with `byte[] to_array()`.
  - `class TextReader`: `read_line() -> String`, `read_to_end() -> String`, `peek() -> int`.
  - `class TextWriter`: `write(String s)`, `write_line(String s)`, `flush()`.
  - `class BinaryReader`: `read_int()`, `read_double()`, `read_bool()`, `read_string()`.
  - `class BinaryWriter`: `write_int(int v)`, `write_double(double v)`, `write_string(String s)`.

### Identified Test & Documentation Deliverables
- **1. Positive Test Scenarios**:
  - `MemoryStream` write, seek to 0, read back byte buffers.
  - `TextWriter` and `TextReader` line-by-line round-trip.
  - Binary serialization and deserialization of mixed primitive values.
- **2. Negative Test Scenarios**:
  - Reading from a closed stream throws `ObjectDisposedException`.
  - Writing to a read-only stream throws `NotSupportedException`.
- **3. Documentation Deliverables**:
  - `docs/spec/solixlib/streams.md`: Stream lifecycle, seeking rules, and buffering mechanisms.

### Action Items
- [ ] Implement `solixlib/native/src/io_stream.cpp` wrapping OS file descriptors.
- [ ] Author `IStream.slx`, `FileStream.slx`, `MemoryStream.slx`, `TextReader.slx`, `TextWriter.slx`, `BinaryReader.slx`, `BinaryWriter.slx`.
- [ ] Add round-trip streaming tests.
- [ ] Author `docs/spec/solixlib/streams.md`.

---

## Phase 33: Standard Library — `solix.system.Environment` & `Process` (OS, Env, Subprocesses)

- **Priority**: `P2 Medium`
- **Affected Modules**: `solixlib/project/src/solix/system/Environment.slx`, `Process.slx`, `ProcessResult.slx`, `solixlib/native/src/system.cpp`
- **Status**: - [ ] Planned

### Objective
Provide access to the host runtime operating environment: environment variables, command-line arguments, operating system identification, system exit codes, and child subprocess spawning.

### Interconnection & Layering
- **Builds On**: `String` (Phase 22), `List<String>` (Phase 27), `Map<String, String>` (Phase 28).
- **Printable**: Process output and environment info printable via `Console`.

### Submodule Architecture & Types
- **Solix Surface**:
  - `class Environment`:
    - `static List<String> get_args()`
    - `static String get_env(String key)`
    - `static void set_env(String key, String value)`
    - `static Map<String, String> get_all_env()`
    - `static void exit(int exit_code)`
    - `static String os_name()`, `static String os_version()`
    - `static bool is_windows()`, `static bool is_linux()`, `static bool is_macos()`
    - `static int processor_count()`
  - `class ProcessResult`:
    - `int exit_code`, `String standard_output`, `String standard_error`
  - `class Process`:
    - `static ProcessResult run(String command, List<String> args)`
    - `void start()`, `int wait_for_exit()`, `void kill()`
- **Native Implementation (`solixlib/native/src/system.cpp`)**:
  - POSIX `fork`/`exec`/`waitpid` and Windows `CreateProcess` with pipe redirection.

### Identified Test & Documentation Deliverables
- **1. Positive Test Scenarios**:
  - Setting and retrieving process-local environment variables.
  - Spawning a child echo process and capturing its standard output.
  - OS detection matching current build host platform.
- **2. Negative Test Scenarios**:
  - Spawning a non-existent binary returns non-zero error status without crashing parent process.
- **3. Documentation Deliverables**:
  - `docs/spec/solixlib/environment.md`: Environment variable security and process lifecycle.

### Action Items
- [ ] Implement `solixlib/native/src/system.cpp` with cross-platform process execution.
- [ ] Author `Environment.slx`, `Process.slx`, `ProcessResult.slx`.
- [ ] Add subprocess and environment variable test cases.
- [ ] Author `docs/spec/solixlib/environment.md`.

---

## Phase 34: Standard Library — `solix.diagnostics` (`Assert`, `Logger`, Benchmarking)

- **Priority**: `P2 Medium`
- **Affected Modules**: `solixlib/project/src/solix/diagnostics/Assert.slx`, `Logger.slx`, `LogLevel.slx`, `Benchmark.slx`
- **Status**: - [ ] Planned

### Objective
Provide developer-facing diagnostic tools including runtime assertion testing (`Assert`), leveled and structured logging (`Logger`), and code execution benchmarking (`Benchmark`).

### Interconnection & Layering
- **Builds On**: `Console` (Phase 21) for colored output, `Stopwatch` & `Duration` (Phase 25) for timing, `IStringable` (Phase 22) for formatting.
- **Enables Self-Testing**: Solix projects and libraries can write self-contained unit tests using `Assert`.

### Submodule Architecture & Types
- **Solix Surface**:
  - `class Assert`:
    - `static void is_true(bool condition, String message = "")`
    - `static void is_false(bool condition, String message = "")`
    - `static void are_equal<T>(T expected, T actual, String message = "")`
    - `static void are_not_equal<T>(T a, T b, String message = "")`
    - `static void is_null(Object obj, String message = "")`
    - `static void is_not_null(Object obj, String message = "")`
    - `static void fail(String message)`
  - `enum LogLevel`: TRACE = 0, DEBUG = 1, INFO = 2, WARN = 3, ERROR = 4, FATAL = 5
  - `class Logger`:
    - `LogLevel minimum_level`
    - `void trace(String message)`, `void debug(String message)`, `void info(String message)`, `void warn(String message)`, `void error(String message)`, `void fatal(String message)`
    - Formats log entries with ISO timestamp, level tag, and optional ANSI color to `Console` or `TextWriter`.
  - `class Benchmark`:
    - `static Duration measure(Action action)`

### Identified Test & Documentation Deliverables
- **1. Positive Test Scenarios**:
  - Passing assertions proceed silently without error.
  - Logging at level INFO suppresses DEBUG logs when minimum level is INFO.
  - `Benchmark.measure()` capturing non-zero execution duration of a compute loop.
- **2. Negative Test Scenarios**:
  - `Assert.is_true(false)` throws `AssertionError` with specified message.
  - `Assert.are_equal(1, 2)` reports expected vs actual values in error description.
- **3. Documentation Deliverables**:
  - `docs/spec/solixlib/diagnostics.md`: Assertion syntax and logging architecture.

### Action Items
- [ ] Author `Assert.slx`, `LogLevel.slx`, `Logger.slx`, `Benchmark.slx`.
- [ ] Add unit tests verifying assert success and failure throwing.
- [ ] Author `docs/spec/solixlib/diagnostics.md`.

---

## Phase 35: Standard Library — `solix.data.Json` (JSON Parsing, Serialization, DOM)

- **Priority**: `P2 Medium`
- **Affected Modules**: `solixlib/project/src/solix/data/Json.slx`, `JsonValue.slx`, `JsonType.slx`, `solixlib/native/src/json.cpp`
- **Status**: - [ ] Planned

### Objective
Provide lightweight, high-performance JSON parsing, object model navigation, and serialization without introducing bulky third-party dependencies into the native library.

### Interconnection & Layering
- **Builds On**: `String` (Phase 22), `List<JsonValue>` (Phase 27), `Map<String, JsonValue>` (Phase 28), `IStringable` (Phase 22).
- **Printable**: Formats JSON strings directly printable to `Console`.

### Submodule Architecture & Types
- **Solix Surface**:
  - `enum JsonType`: NULL, BOOLEAN, NUMBER, STRING, ARRAY, OBJECT
  - `class JsonValue implements IStringable`:
    - `JsonType type()`
    - `bool is_null()`, `bool is_bool()`, `bool is_number()`, `bool is_string()`, `bool is_array()`, `bool is_object()`
    - `bool as_bool()`, `int as_int()`, `double as_double()`, `String as_string()`
    - `JsonValue get(String key)`, `void set(String key, JsonValue value)`
    - `JsonValue get_at(int index)`, `void add(JsonValue value)`
    - `int size()`, `List<String> keys()`
    - `string to_string()`
  - `class Json`:
    - `static JsonValue parse(String json_string)`
    - `static String stringify(JsonValue value, bool pretty = false)`
- **Native Implementation (`solixlib/native/src/json.cpp`)**:
  - Compact, fast RFC 8259 compliant recursive descent JSON parser and serializer.

### Identified Test & Documentation Deliverables
- **1. Positive Test Scenarios**:
  - Parsing nested JSON objects and arrays with strings, integers, floats, booleans, and null.
  - Serializing `JsonValue` back to compact and pretty-printed JSON text.
  - Navigating object keys and array indices.
- **2. Negative Test Scenarios**:
  - Malformed JSON (unclosed brackets, trailing commas, unexpected tokens) throws `JsonParseException` with line/column offset.
  - Accessing `as_int()` on a JSON object throws `InvalidCastException`.
- **3. Documentation Deliverables**:
  - `docs/spec/solixlib/json.md`: RFC 8259 conformance and DOM manipulation.

### Action Items
- [ ] Implement lightweight C++ JSON parser in `solixlib/native/src/json.cpp`.
- [ ] Author `JsonType.slx`, `JsonValue.slx`, `Json.slx`.
- [ ] Add JSON parsing, round-trip serialization, and error recovery test cases.
- [ ] Author `docs/spec/solixlib/json.md`.

---

## Phase 36: Standard Library — `solix.crypto` (Base64, Hex, SHA-256, MD5)

- **Priority**: `P3 Low`
- **Affected Modules**: `solixlib/project/src/solix/crypto/Base64.slx`, `Hex.slx`, `Hash.slx`, `solixlib/native/src/crypto.cpp`
- **Status**: - [ ] Planned

### Objective
Implement essential cryptographic hashing functions (SHA-256, SHA-1, MD5) and binary encodings (Base64, Hexadecimal) using a compact, self-contained native implementation without heavy external OpenSSL dependencies.

### Interconnection & Layering
- **Builds On**: `String` (Phase 22), byte arrays, and `Streams` (Phase 32).
- **Downstream Use**: Package integrity verification, cache checksums, HTTP basic authentication, security digests.

### Submodule Architecture & Types
- **Solix Surface**:
  - `class Base64`:
    - `static String encode(byte[] data)`
    - `static String encode_string(String text)`
    - `static byte[] decode(String base64)`
    - `static String decode_to_string(String base64)`
  - `class Hex`:
    - `static String encode(byte[] data)`
    - `static byte[] decode(String hex_string)`
  - `class Hash`:
    - `static byte[] sha256(byte[] data)`
    - `static String sha256_hex(String text)`
    - `static String sha1_hex(String text)`
    - `static String md5_hex(String text)`
- **Native Implementation (`solixlib/native/src/crypto.cpp`)**:
  - Self-contained C implementations of FIPS 180-4 SHA-256, SHA-1, RFC 1321 MD5, and RFC 4648 Base64.

### Identified Test & Documentation Deliverables
- **1. Positive Test Scenarios**:
  - NIST test vectors for SHA-256 (e.g. hash of `"abc"` matches `ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad`).
  - Base64 encoding and decoding round-trips with padding (=, ==) and unpadded multiples of 3.
- **2. Negative Test Scenarios**:
  - Invalid Base64 characters or malformed padding throw `FormatException`.
  - Odd-length hex string throws `FormatException`.
- **3. Documentation Deliverables**:
  - `docs/spec/solixlib/crypto.md`: Cryptographic algorithms, test vector validations, and security boundaries.

### Action Items
- [ ] Implement self-contained crypto algorithms in `solixlib/native/src/crypto.cpp`.
- [ ] Author `Base64.slx`, `Hex.slx`, `Hash.slx`.
- [ ] Add NIST / RFC test vector verification suite.
- [ ] Author `docs/spec/solixlib/crypto.md`.

---

## Phase 37: Standard Library — `solix.concurrent` (`Thread`, `Mutex`, `LockGuard`, `AtomicInt`)

- **Priority**: `P2 Medium`
- **Affected Modules**: `solixlib/project/src/solix/concurrent/Thread.slx`, `Mutex.slx`, `LockGuard.slx`, `AtomicInt.slx`, `AtomicBool.slx`, `solixlib/native/src/concurrent.cpp`
- **Status**: - [ ] Planned

### Objective
Provide operating system thread management, mutual exclusion synchronization primitives, RAII lock guards, and hardware-accelerated lock-free atomics for multi-threaded Solix programs.

### Interconnection & Layering
- **Builds On**: Language function delegates / lambdas, `Duration` (Phase 25) for sleeping/timeouts.
- **Synchronizes**: Collections and shared state across threads.

### Submodule Architecture & Types
- **Solix Surface**:
  - `class Thread`:
    - `static Thread start(Action action)`
    - `void join()`
    - `static void sleep(Duration duration)`
    - `static void yield()`
    - `long id()`
    - `bool is_alive()`
  - `class Mutex`:
    - `void lock()`
    - `void unlock()`
    - `bool try_lock()`
  - `class LockGuard`:
    - Constructor acquires mutex; destructor releases mutex upon scope exit.
  - `class AtomicInt`:
    - `int get()`, `void set(int value)`
    - `int fetch_add(int delta)`, `int fetch_sub(int delta)`
    - `bool compare_exchange(int expected, int desired)`
  - `class AtomicBool`:
    - `bool get()`, `void set(bool value)`, `bool exchange(bool value)`
- **Native Implementation (`solixlib/native/src/concurrent.cpp`)**:
  - Standard C++20 `<thread>`, `<mutex>`, and `<atomic>`.

### Identified Test & Documentation Deliverables
- **1. Positive Test Scenarios**:
  - Spawning multiple threads that concurrently increment a shared counter protected by `Mutex`, verifying final count is exact.
  - Lock-free `AtomicInt` fetch_add race-condition resistance.
  - `Thread.sleep(Duration.from_milliseconds(50))` timing accuracy.
- **2. Negative Test Scenarios**:
  - Unlocking an unacquired mutex throws `SynchronizationLockException`.
  - Joining an already joined thread handled safely without crashing.
- **3. Documentation Deliverables**:
  - `docs/spec/solixlib/concurrent.md`: Memory model, thread safety rules, and lock semantics.

### Action Items
- [ ] Implement thread and mutex bindings in `solixlib/native/src/concurrent.cpp`.
- [ ] Author `Thread.slx`, `Mutex.slx`, `LockGuard.slx`, `AtomicInt.slx`, `AtomicBool.slx`.
- [ ] Add concurrency stress tests.
- [ ] Author `docs/spec/solixlib/concurrent.md`.

---

## Phase 38: Standard Library — `solix.net` (TCP/UDP Sockets & Lightweight `HttpClient`)

- **Priority**: `P3 Low`
- **Affected Modules**: `solixlib/project/src/solix/net/IPAddress.slx`, `IPEndPoint.slx`, `TcpClient.slx`, `TcpListener.slx`, `HttpClient.slx`, `HttpResponse.slx`, `solixlib/native/src/net.cpp`
- **Status**: - [ ] Planned

### Objective
Implement cross-platform networking primitives: IP address handling, TCP client/server streaming sockets, and a lightweight HTTP client for REST API consumption.

### Interconnection & Layering
- **Builds On**: `IStream` (Phase 32) for socket read/write, `String` (Phase 22), `Map<String, String>` (Phase 28) for headers, `Json` (Phase 35) for payload serialization.
- **Cross-Platform**: Berkeley sockets on POSIX, Winsock (`WSAStartup`) on Windows.

### Submodule Architecture & Types
- **Solix Surface**:
  - `class IPAddress`:
    - `static IPAddress parse(String ip_string)`
    - `static IPAddress loopback()`, `static IPAddress any()`
    - `String to_string()`
  - `class IPEndPoint`:
    - `IPAddress address`, `int port`
  - `class TcpClient`:
    - `void connect(String host, int port)`
    - `IStream get_stream()`
    - `void close()`, `bool is_connected()`
  - `class TcpListener`:
    - `void start(int port)`
    - `TcpClient accept()`
    - `void stop()`
  - `class HttpResponse`:
    - `int status_code`, `Map<String, String> headers`, `String body`
  - `class HttpClient`:
    - `HttpResponse get(String url)`
    - `HttpResponse post(String url, String body, String content_type = "application/json")`
- **Native Implementation (`solixlib/native/src/net.cpp`)**:
  - Cross-platform non-blocking TCP socket abstraction and minimal HTTP/1.1 client.

### Identified Test & Documentation Deliverables
- **1. Positive Test Scenarios**:
  - Parsing IPv4 strings (`127.0.0.1`).
  - Spawning a local `TcpListener` on a loopback port, connecting a `TcpClient`, sending data, and verifying receipt.
  - HTTP response header and status parsing.
- **2. Negative Test Scenarios**:
  - Connecting to a non-existent port returns error code / throws `SocketException` within timeout.
  - Invalid IP address string throws `FormatException`.
- **3. Documentation Deliverables**:
  - `docs/spec/solixlib/net.md`: Network protocol support, socket state diagrams, and timeouts.

### Action Items
- [ ] Implement cross-platform socket primitives in `solixlib/native/src/net.cpp`.
- [ ] Author `IPAddress.slx`, `IPEndPoint.slx`, `TcpClient.slx`, `TcpListener.slx`, `HttpClient.slx`, `HttpResponse.slx`.
- [ ] Add local loopback TCP echo tests.
- [ ] Author `docs/spec/solixlib/net.md`.











