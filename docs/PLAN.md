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
| **Phase 1** | Total Decoupling & Legacy Stdlib Purge | `P0 Blocker` | `launcher`, `tests`, `build` | - [ ] Not Started |
| **Phase 2** | Interface VTable Dynamic Dispatch Stabilization | `P0 Blocker` | `compiler`, `runtime`, `tests` | - [ ] Not Started |
| **Phase 3** | Generic Type Safety & Default Value Slot Clearance | `P0 Blocker` | `compiler`, `runtime`, `tests` | - [ ] Not Started |
| **Phase 4** | Cross-Platform Path & Toolchain Portability | `P1 High` | `launcher`, `runtime` | - [ ] Not Started |
| **Phase 5** | Two's-Complement & Arithmetic Invariant Hardening | `P1 High` | `runtime`, `compiler`, `tests` | - [ ] Not Started |
| **Phase 6** | Deterministic ARC Lifecycle Verification | `P1 High` | `runtime`, `tests` | - [ ] Not Started |
| **Phase 7** | Documentation & Specification Synchronization | `P2 Polish` | `docs`, `launcher`, `tests` | - [ ] Not Started |

---

## Phase 1: Total Decoupling & Legacy Stdlib Purge

- **Priority**: `P0 Blocker`
- **Affected Modules**: `tests`, `launcher`, `build`
- **Status**: - [ ] Pending

### Objective
Completely eliminate the bundled standard library (`launcher/rsc/lib/`), removing disk-based stdlib injection in the test harness, and converting all dependent unit tests into self-contained test scenarios.

### Action Items
- [ ] **Purge Test Harness Stdlib Injection (`tests/include/test_helper.hpp`)**:
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
- [ ] **Refactor Dependent Test Suites**:
  - `tests/statements/declarations/test_field_declaration.cpp`: Rewrite Cases 3.5 and 3.6 to remove `solix.core.String` and `include_stdlib = true`, validating in-class and static field initialization with local user-defined classes and primitives.
  - `tests/statements/expressions/test_unary_expression.cpp`: Rewrite Case 4.2 to remove `include_stdlib = true` and assert unary minus rejection on an isolated user class rather than `String`.
  - Audit all files in `tests/statements/**/*.cpp` to ensure zero dependencies on `solix.core.*`, `solix.collections.*`, or `solix.systems.*`.
- [ ] **Purge Launcher Build Targets & Assets**:
  - Remove the `copy_stdlib` custom target and dependencies in `launcher/CMakeLists.txt`.
  - Remove CLI flags `--stdlib` / `--no-stdlib` and stdlib path resolution logic from `launcher/src/commands/compile.cpp`.
  - Permanently delete the directory `launcher/rsc/lib/`.

### Acceptance Criteria
- `tests/include/test_helper.hpp` has no file I/O or standard library loading logic.
- `launcher/rsc/lib/` is completely removed from the repository.
- All 41 unit test suites compile and pass 100% with `ctest --test-dir build --output-on-failure`.

---

## Phase 2: Interface VTable Dynamic Dispatch Stabilization

- **Priority**: `P0 Blocker`
- **Affected Modules**: `compiler` (Binder, Assembler), `runtime`, `tests`
- **Status**: - [ ] Pending

### Objective
Stabilize interface polymorphism, correct multi-interface VTable layout and offset resolution in the compiler Binder, and ensure correct dynamic dispatch in the VM.

### Action Items
- [ ] **Correct Interface VTable Layout in Binder Pass 3**:
  - Fix slot mapping when a class implements multiple interfaces or inherits interface implementations.
  - Ensure interface method indices align with class dispatch tables and interface stub offsets.
- [ ] **Runtime Dynamic Dispatch for Multi-Interface Implementations**:
  - Fix VM dynamic dispatch execution (`OpCode::CALL_VIRTUAL`) when dispatching through interface references.
  - Ensure correct resolution of `this` instance offset and virtual table slot across disparate interface hierarchies.
- [ ] **Unblock and Verify Interface Unit Tests**:
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
- **Status**: - [ ] Pending

### Objective
Resolve the contradiction between primitive non-nullability and generic container initialization by introducing a `default(T)` intrinsic and dedicated slot reclamation instruction.

### Action Items
- [ ] **Implement `default(T)` Compiler Intrinsic**:
  - Parse and type-check `default(T)` expressions for primitive, object, and function pointer types.
  - Evaluate scalar types to `0`, `0.0`, `false`, `\0`, or `0ULL` (for function pointers).
  - Evaluate reference types (classes, arrays) to `null` (`0ULL`).
- [ ] **VM Instruction `DEC_REF_SLOT`**:
  - Introduce `OpCode::DEC_REF_SLOT` taking a local frame slot index and reference mask.
  - Decrement reference counts on non-null object pointers without overwriting primitive value representations.
- [ ] **Add Unit Tests for Default Values**:
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
- **Status**: - [ ] Pending

### Objective
Abstract binary path resolution to support Linux, macOS, and Windows cleanly without relying exclusively on `/proc/self/exe`.

### Action Items
- [ ] **Abstract Executable Path Discovery**:
  - Replace raw `/proc/self/exe` reads in `launcher/src/commands/compile.cpp` with a cross-platform helper:
    - Linux: `/proc/self/exe` via `readlink`
    - macOS: `_NSGetExecutablePath`
    - Windows: `GetModuleFileNameW`
- [ ] **Normalize File System Paths**:
  - Use `std::filesystem::path` uniformly across all path joins and directory inspections.
- [ ] **Verify Portability**:
  - Verify executable builds and runs cleanly on Linux, macOS, and Windows environments without missing-file or path crashes.

### Acceptance Criteria
- Path resolution logic compiles and functions correctly across all three operating systems.
- Executable correctly identifies its own directory without hardcoded assumptions.

---

## Phase 5: Two's-Complement & Arithmetic Invariant Hardening

- **Priority**: `P1 High`
- **Affected Modules**: `runtime`, `compiler`, `tests`
- **Status**: - [ ] Pending

### Objective
Prevent signed integer overflow crashes on extreme boundary values such as `INT32_MIN` (`-2147483648`) and `INT64_MIN`.

### Action Items
- [ ] **Guard Boundary Negation**:
  - Protect integer-to-string formatting against `-INT32_MIN` overflow UB in C++ runtime logic.
  - Ensure compiler constant folding handles `-2147483648` correctly without accidental promotion or overflow warnings.
- [ ] **Harden Hash Table Indexing**:
  - Apply positive bitmasking (`(hash & 0x7FFFFFFF) % capacity`) to prevent negative array indexing when hashing negative integers or `INT32_MIN`.
- [ ] **Add Arithmetic Boundary Tests**:
  - Implement test cases covering `INT32_MIN`, `INT32_MAX`, `INT64_MIN`, and `INT64_MAX` across arithmetic, unary negation, string conversion, and comparisons.

### Acceptance Criteria
- Formatting, negating, or hashing `INT32_MIN` and `INT64_MIN` causes no undefined behavior or negative array indexing.
- Boundary test cases pass 100%.

---

## Phase 6: Deterministic ARC Lifecycle Verification

- **Priority**: `P1 High`
- **Affected Modules**: `runtime`, `tests`
- **Status**: - [ ] Pending

### Objective
Provide programmatic introspection of runtime heap allocations and verify deterministic deallocation and destruction order under ARC.

### Action Items
- [ ] **Expose Runtime Live Object Hook**:
  - Add `solix::get_live_object_count()` to the runtime memory subsystem.
  - Track active reference-counted allocations and deallocations.
- [ ] **Update ARC Test Scenarios**:
  - In `test_field_declaration.cpp` (Case 3.1: Cycle Breaking with Weak References), assert that `get_live_object_count()` drops to 0 when cycles are broken and scopes exit.
- [ ] **LIFO Destruction & Nested Scope Regression Tests**:
  - Add unit tests verifying that destructors and cleanups run in strict reverse declaration order (LIFO) within blocks and call frames.

### Acceptance Criteria
- `get_live_object_count()` accurately reflects allocated heap instances.
- Weak references and scope exits cleanly reduce live object counts to zero.

---

## Phase 7: Documentation & Specification Synchronization

- **Priority**: `P2 Polish`
- **Affected Modules**: `docs`, `launcher`, `tests`
- **Status**: - [ ] Pending

### Objective
Synchronize user-facing documentation, README badges, and code examples with the standalone Solix engine.

### Action Items
- [ ] **Update `README.md`**:
  - Update test suite badges and documentation to reflect 41 complete test suites.
  - Remove all mentions of `launcher/rsc/lib/` or bundled standard library paths.
  - Fix syntax errors in example code snippets (such as the missing closing brace in `hello.slx`).
- [ ] **Document Standard Entry Points**:
  - Explicitly document valid entry signatures:
    - `static int32 main()`
    - `static void main(char[][] args)`
- [ ] **Synchronize `tests/statements/TESTS.md`**:
  - Confirm all 41 test suites in `TESTS.md` reflect purely self-contained scenarios.

### Acceptance Criteria
- `README.md` and `docs/` contain zero references to `launcher/rsc/lib/`.
- Code snippets in documentation compile without syntax errors.
- Test documentation accurately matches all test suites.
