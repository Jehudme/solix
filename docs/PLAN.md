# Solix Engineering Master Plan — Post-v1.1.0 Integration Cycle

This document tracks all planned, in-progress, and completed engineering phases for the next Solix development and integration cycle following the v1.0.0 General Availability release.

> **Historical Archive**: Phases 1 through 58 of the initial v1.0.0 GA development cycle are archived at [`docs/archive/PLAN_v1.0.0.md`](archive/PLAN_v1.0.0.md).

---

## Phase Index & Status

## Phase Index & Status

| Phase | Title | Priority | Status |
|---|---|---|---|
| [Phase 1](#phase-1-build-system-hygiene--sanitizer-foundation) | Build System Hygiene & Sanitizer Foundation | P0 Blocker | - [x] Completed & Merged |
| [Phase 2](#phase-2-manifest-redesign-hello-world-template--ide-activation) | Manifest Redesign (`manifest.json`), Hello World Template & IDE Activation | P1 High | - [x] Completed & Merged |
| [Phase 3](#phase-3-standard-library-pruning--manifest-ingestion) | Standard Library Pruning & Complete Manifest Ingestion | P1 High | - [x] Completed & Merged |

---

## Phase 1: Build System Hygiene & Sanitizer Foundation

- **Priority**: `P0 Blocker`
- **Affected Modules**: `CMakeLists.txt`, `.github/workflows/integration.yml`, `docs/WORKFLOW.md`, `tests/commands/TESTS.md`, `core/`, `lsp/`
- **Status**: - [x] Completed & Merged (2026-10-14)

### Objective
Enable strict compiler warning flags (`-Wall -Wextra` on GCC/Clang and `/W4` on MSVC), establish a zero-warning policy across all C++ source files, introduce an AddressSanitizer/UBSan CI pipeline, and enforce zero-warning and sanitizer invariants in `WORKFLOW.md`.

### Action Items
- [x] Configure `-Wall -Wextra` (and `/W4` on MSVC) in root `CMakeLists.txt`.
- [x] Add `ENABLE_SANITIZERS` CMake option with `-fsanitize=address,undefined`.
- [x] Add automated `sanitizers` CI job in `.github/workflows/integration.yml`.
- [x] Fix compiler warnings in `statements.hpp`, `ast_visitor.hpp`, `template_substitution.hpp/cpp`, and `lsp/src/main.cpp`.
- [x] Define test specifications in `tests/commands/TESTS.md` (Part XIII: Cases 19.1 & 19.2).
- [x] Formalize zero-warning and sanitizer invariants in `docs/WORKFLOW.md`.
- [x] Verify 100% test pass rate across all 76 test suites.

---

## Phase 2: Manifest Redesign (`manifest.json`), Hello World Template & IDE Activation

- **Priority**: `P1 High`
- **Affected Modules**: `cli/src/commands/new.cpp`, `cli/src/commands/ide.cpp`, `cli/src/package_manager.cpp`, `cli/src/dependency_resolver.hpp`, `lsp/src/server.cpp`, `CMakeLists.txt`, `tests/commands/`
- **Status**: - [x] Completed & Merged (2026-10-14)

### Objective
1. **Manifest File Redesign**:
   - Rename default project manifest to `manifest.json` while maintaining backward-compatible fallback to `solix.json`.
   - Update dependency taxonomy: support `{ "type": "library", "name": "foo" }` alongside `{ "type": "project" }`.
2. **`solix new` Hello World Template**:
   - `solix new` generates `src/main.slx` printing `"Hello, World!"` with `solix.system.Console` and includes standard library dependency.
3. **IDE Activation Command**:
   - Introduce `solix activate vscode` (and `solix activate cursor`) command to activate the Solix extension for the target editor.

### Action Items
- [x] Add `solix::find_project_manifest()` path utility prioritizing `manifest.json` over `solix.json`.
- [x] Update `solix build`, `solix run`, `solix install`, and dependency resolver to support `manifest.json` and `"type": "library"`.
- [x] Update LSP workspace root discovery to detect `manifest.json` and `solix.json`.
- [x] Update `solix new` to scaffold `manifest.json` and `Console.println("Hello, World!")` template with `solixlib` dependency.
- [x] Implement `solix activate <editor>` command supporting `vscode`, `code`, `cursor`, `codium`.
- [x] Fix entry point candidate resolution in `assembler.cpp` to ignore non-static class member methods.
- [x] Update test specifications in `tests/commands/TESTS.md` and Catch2 test suites.
- [x] Update CLI specifications (`manifest.md`, `new.md`, `ide.md`, `README.md`).
- [x] Verify 100% test pass rate across all 76 test suites.

---

## Phase 3: Standard Library Pruning & Complete Manifest Ingestion

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/project/src/solix/core/`, `solixlib/project/manifest.json`, `solixlib/native/src/primitives.cpp`, `CMakeLists.txt`, `tests/solixlib/`, `tests/commands/`
- **Status**: - [x] Completed & Merged (2026-10-14)

### Objective
1. **Boxed Numeric Wrappers Purge**:
   - Remove redundant boxed wrapper files: `Bool.slx`, `Byte.slx`, `Char.slx`, `Double.slx`, `Float.slx`, `Int.slx`, `Long.slx`, `Short.slx`, `UByte.slx`, `UInt.slx`, `ULong.slx`, `UShort.slx`.
   - Consolidate bounds constants, bit manipulation, IEEE 754 floating point utilities, and character operations into `solix.core.Primitives`.
2. **Complete Manifest Ingestion**:
   - Rename `solixlib/project/solix.json` to `manifest.json`.
   - Ingest all 50 active source files (crypto, collections, io, system, time, math, primitives) into `manifest.json`.
   - Update CMake packaging rules to install `manifest.json`.
   - Verify all tests pass 100%.

### Action Items
- [x] Remove 12 redundant boxed scalar type files from `solixlib/project/src/solix/core/`.
- [x] Implement `solix.core.Primitives` utility class with integer bounds, bitwise operations, IEEE 754 helpers, and character inspection.
- [x] Register native helper bindings in `solixlib/native/src/primitives.cpp` with zero warnings.
- [x] Rename `solixlib/project/solix.json` to `manifest.json` and add all 50 source files to dependencies.
- [x] Update CMake install patterns to include `manifest.json`.
- [x] Update `tests/solixlib/TESTS.md` specification and Catch2 suites (`test_primitives.cpp`, `test_collections_core.cpp`, `test_cpack_packaging.cpp`, `test_ide_command.cpp`, `test_package_commands.cpp`).
- [x] Update documentation in `docs/spec/solixlib/primitives.md`.
- [x] Verify 100% test pass rate across all 76 test suites.
