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
| **Phase 1** | Total Decoupling & Legacy Stdlib Purge | `P0 Blocker` | `launcher`, `tests`, `build` | - [x] Completed |
| **Phase 2** | Interface VTable Dynamic Dispatch Stabilization | `P0 Blocker` | `compiler`, `runtime`, `tests` | - [x] Completed |
| **Phase 3** | Generic Type Safety & Default Value Slot Clearance | `P0 Blocker` | `compiler`, `runtime`, `tests` | - [x] Completed |
| **Phase 4** | Cross-Platform Path & Toolchain Portability | `P1 High` | `launcher`, `runtime` | - [x] Completed |
| **Phase 5** | Two's-Complement & Arithmetic Invariant Hardening | `P1 High` | `runtime`, `compiler`, `tests` | - [ ] Not Started |
| **Phase 6** | Deterministic ARC Lifecycle Verification | `P1 High` | `runtime`, `tests` | - [ ] Not Started |
| **Phase 7** | Documentation & Specification Synchronization | `P2 Polish` | `docs`, `launcher`, `tests` | - [x] Completed |
| **Phase 8** | Project Manifest & Modular Build Subcommand (`solix build`) | `P1 High` | `launcher`, `build` | - [x] Completed |
| **Phase 9** | Project Scaffolding & Initialization Subcommand (`solix new`) | `P1 High` | `launcher`, `templates` | - [x] Completed |
| **Phase 10** | Package Lifecycle Management (`install`, `uninstall`, `list`, `details`) | `P1 High` | `launcher` | - [x] Completed |

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



