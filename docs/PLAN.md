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
| **Phase 11** | CLI Commands Test Suite & Master Specification (`tests/commands/`) | `P1 High` | `tests`, `launcher`, `build` | - [x] Completed |
| **Phase 12** | CLI Commands & Toolchain Documentation (`docs/spec/cli/`) | `P1 High` | `docs`, `launcher`, `guide` | - [x] Completed |
| **Phase 13** | Project-Type Dependencies & Transitive SemVer Resolution | `P1 High` | `launcher`, `build`, `tests`, `docs` | - [x] Completed |
| **Phase 14** | Direct Project Execution (`solix run` for Projects) | `P1 High` | `launcher`, `build`, `tests`, `docs` | - [x] Completed |
| **Phase 15** | Functions, Lambdas & Generics Documentation and Test Hardening | `P1 High` | `docs`, `tests`, `language` | - [ ] In Progress |

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






