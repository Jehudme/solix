# Solix Engineering Master Plan — Post-v1.1.0 Integration Cycle

This document tracks all planned, in-progress, and completed engineering phases for the next Solix development and integration cycle following the v1.0.0 General Availability release.

> **Historical Archive**: Phases 1 through 58 of the initial v1.0.0 GA development cycle are archived at [`docs/archive/PLAN_v1.0.0.md`](archive/PLAN_v1.0.0.md).

---

## Phase Index & Status

| Phase | Title | Priority | Status |
|---|---|---|---|
| [Phase 1](#phase-1-build-system-hygiene--sanitizer-foundation) | Build System Hygiene & Sanitizer Foundation | P0 Blocker | - [x] Completed & Merged |
| [Phase 2](#phase-2-manifest-redesign-hello-world-template--ide-activation) | Manifest Redesign (`manifest.json`), Hello World Template & IDE Activation | P1 High | - [x] Completed & Merged |
| [Phase 3](#phase-3-standard-library-pruning--manifest-ingestion) | Standard Library Pruning & Complete Manifest Ingestion | P1 High | - [x] Completed & Merged |
| [Phase 4](#phase-4-byte-addressed-memory-model-transition) | Byte-Addressed Memory Model Transition | P0 Blocker | - [x] Completed & Merged |
| [Phase 5](#phase-5-heap-allocator--arc-overhaul) | Heap Allocator & ARC Overhaul | P1 High | - [x] Completed & Merged |
| [Phase 6](#phase-6-cross-platform-zip-engine-integration-miniz--solix-pack-command) | Cross-Platform ZIP Engine Integration (`miniz`) & `solix pack` Command | P1 High | - [x] Completed & Merged |
| [Phase 7](#phase-7-runtime--launcher-bundle-ingestion-and-dynamic-library-loading) | Runtime & Launcher Bundle Ingestion and Dynamic Library Loading | P1 High | - [x] Completed & Merged |
| [Phase 8](#phase-8-standard-library-solixlib-resource--filesystem-dual-location-api) | Standard Library (`solixlib`) Resource & Filesystem Dual-Location API | P1 High | - [ ] Planned |
| [Phase 9](#phase-9-end-to-end-cross-platform-verification-tests--documentation) | End-to-End Cross-Platform Verification, Tests & Documentation | P2 Polish | - [ ] Planned |

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

---

## Phase 4: Byte-Addressed Memory Model Transition

- **Priority**: `P0 Blocker`
- **Affected Modules**: `core/include/solix/runtime.hpp`, `core/src/runtime.cpp`, `core/src/processes/binder.cpp`, `core/src/processes/assembler.cpp`, `solixlib/native/src/`, `tests/`
- **Status**: - [x] Completed & Merged (2026-10-14)

### Objective
Transition the Solix virtual machine heap from word-addressed (`std::vector<uint64_t>`) to byte-addressed (`std::vector<uint8_t>`) memory while preserving 64-bit operand stack slots. Implement safe inline unaligned memory access primitives with `std::memcpy`. Update the compiler `Binder` layout calculator to naturally align and pack instance fields (1, 2, 4, 8 bytes). Update property access opcodes (`GET_PROPERTY`, `SET_PROPERTY`, `WEAK_SET_PROPERTY`) to encode field byte offsets and access widths. Modernize array element access and string layouts to use native byte strides (1 for `char[]`/`bool[]`/`byte[]`, 4 for `int32[]`/`float32[]`, 8 for `int64[]`/`float64[]`/references). Unify standard library native string/array access.

### Action Items
- [x] Convert `Memory::heap` in `core/include/solix/runtime.hpp` to `std::vector<uint8_t>`.
- [x] Implement inline unaligned-safe memory access primitives (`read_u8`, `write_u8`, `read_u16`, `write_u16`, `read_u32`, `write_u32`, `read_u64`, `write_u64`, `read_f64`, `write_f64`, `read_char`, `write_char`) using `std::memcpy`.
- [x] Update `Memory::dynamic_allocation` and `Memory::static_allocation` to allocate byte buffers with 8-byte aligned headers positioned at `address - 8` (`[ uint32_t length | uint32_t tag_and_ref ]`).
- [x] Update `Binder::calculate_layout` in `core/src/processes/binder.cpp` to naturally align primitive fields (1, 2, 4, 8 bytes), reserve bytes 0..7 for `vtable_id`, and record raw byte offsets in `field->memory_index` and `reference_field_offsets`.
- [x] Update `GET_PROPERTY`, `SET_PROPERTY`, and `WEAK_SET_PROPERTY` in `core/src/processes/assembler.cpp` and `core/src/runtime.cpp` to encode and execute byte offsets and field access widths (1, 2, 4, 8 bytes).
- [x] Update `ALLOC_DYNAMIC`, `GET_ARRAY`, `SET_ARRAY`, and `ARRAY_LENGTH` to handle element strides (1 for 8-bit types, 2 for 16-bit, 4 for 32-bit, 8 for 64-bit and references).
- [x] Update `PUSH_CONST_STRING` and string/array handling across `solixlib/native/src/` (`console.cpp`, `string.cpp`, `time.cpp`, `io_fs.cpp`, `io_stream.cpp`, `system.cpp`, `crypto.cpp`).
- [x] Update test specifications in `tests/statements/TESTS.md` (Cases 3.8 and 3.9).
- [x] Implement Catch2 unit tests in `tests/statements/declarations/test_field_declaration.cpp`.
- [x] Update architecture documentation in `docs/architecture/memory_model.md` and `docs/architecture/arc_internals.md`.
- [x] Verify 100% test pass rate across all 76 test suites with zero warnings under `-Wall -Wextra` and `/W4`.

---

## Phase 5: Heap Allocator & ARC Overhaul

- **Priority**: `P1 High`
- **Affected Modules**: `core/include/solix/runtime.hpp`, `core/src/runtime.cpp`, `core/src/processes/assembler.cpp`, `tests/`
- **Status**: - [x] Completed & Merged (2026-10-14)

### Objective
Eliminate array vs. object type confusion, memory leaks on array element replacement, and heap fragmentation. Tag dynamic heap blocks with 4-bit block tags (`RAW`, `OBJECT`, `ARRAY_U8`, `ARRAY_U16`, `ARRAY_U32`, `ARRAY_U64`, `ARRAY_REF`, `CLOSURE`). Implement segregated free-list binning and adjacent block coalescing to eliminate heap fragmentation and achieve $O(1)$ allocation/deallocation. Enforce reference lifecycle (`DEC_REF` old, `INC_REF` new) in `op_SET_ARRAY` for reference arrays. Add execution stack bounds checking and safe opcode dispatch verification.

### Action Items
- [x] Define `BlockTag` enum (RAW, OBJECT, ARRAY_U8, ARRAY_U16, ARRAY_U32, ARRAY_U64, ARRAY_REF, CLOSURE) and encode 4-bit tag in header `tag_and_ref`.
- [x] Update `object_destructor` in `RuntimeContext` to branch on `BlockTag`, resolving array vs. object confusion and ensuring child references in reference arrays are decremented on release while primitive arrays are never misinterpreted as vtables.
- [x] Update `op_INSTANCEOF` and `op_CAST_CHECK` to verify `tag == BlockTag::OBJECT` before reading vtable IDs, preventing primitive arrays from being miscast as objects.
- [x] Implement ARC lifecycle tracking on array element mutation in `op_SET_ARRAY` for `ARRAY_REF` arrays (`DEC_REF` overwritten occupant, `INC_REF` stored occupant).
- [x] Implement segregated free-list size classes and adjacent free-block coalescing in `Memory::dynamic_allocation` and `Memory::deallocate`.
- [x] Add runtime VM execution stack bounds checking in `PUSH` and `POP` to throw `StackOverflowException` instead of overflowing the process stack.
- [x] Add safe opcode validation for computed goto and switch dispatch.
- [x] Update test specifications in `tests/statements/TESTS.md` (Cases 3.3 in ArrayAccess, 3.3 in InstanceOf, 3.10 in FieldDeclaration) and Catch2 test suites.
- [x] Update architecture documentation in `docs/architecture/arc_internals.md`.
- [x] Verify 100% test pass rate across all 76 test suites with zero warnings under `-Wall -Wextra` and `/W4`.

---

## Phase 6: Cross-Platform ZIP Engine Integration (`miniz`) & `solix pack` Command

- **Priority**: `P1 High`
- **Affected Modules**: `CMakeLists.txt`, `core/include/solix/zip_utils.hpp`, `core/src/utilities/zip_utils.cpp`, `cli/src/commands/pack.hpp`, `cli/src/commands/pack.cpp`, `cli/CMakeLists.txt`, `cli/src/main.cpp`, `tests/commands/test_pack_command.cpp`, `tests/commands/TESTS.md`
- **Status**: - [x] Completed & Merged (2026-10-17)

### Objective
Integrate the cross-platform, dependency-free `miniz` ZIP compression/decompression engine into the Solix core utility layer, and introduce the `solix pack` CLI command. This command packages a Solix project into a single `.slxpac` archive containing the compiled bytecode (`bytecode.slxbin`), native dynamic libraries (`lib/`), application resource assets (`resources/`), and bundle metadata (`manifest.json`).

### Action Items
- [x] Integrate `miniz` in CMake with zero warnings under `-Wall -Wextra` and `/W4`.
- [x] Author cross-platform C++ zip utilities (`solix::zip_directory`, `solix::extract_zip`, `solix::read_zip_file_entry`) in `core/`.
- [x] Implement `solix pack [target] [-p|--profile <profile>] [-o|--output <path.slxpac>]` in `cli/src/commands/pack.cpp`.
- [x] Bundle `bytecode.slxbin`, `lib/` (native shared libraries `.so`, `.dll`, `.dylib`), `resources/` (if present), and write package `manifest.json`.
- [x] Add test specifications in `tests/commands/TESTS.md` (Part XIV: Cases 20.1 to 20.5).
- [x] Implement Catch2 integration tests in `tests/commands/test_pack_command.cpp`.
- [x] Author CLI specification in `docs/spec/cli/pack.md`.
- [x] Verify 100% test pass rate across all test suites.

---

## Phase 7: Runtime & Launcher Bundle Ingestion and Dynamic Library Loading

- **Priority**: `P1 High`
- **Affected Modules**: `cli/src/commands/execute.cpp`, `core/include/solix/runtime.hpp`, `core/src/runtime.cpp`, `tests/commands/test_run_bundle.cpp`, `tests/commands/TESTS.md`
- **Status**: - [x] Completed & Merged (2026-10-17)

### Objective
Enable `solix run` and `solix_launcher` to directly accept and execute `.slxpac` bundle archives. Extract the bundle to a content-hashed cache directory (`$TMPDIR/solix_pac/<hash>/`), automatically discover and load bundled native shared libraries in `lib/`, set the application resource root, and execute the entry bytecode.

### Action Items
- [x] Update `cli/src/commands/execute.cpp` to recognize `.slxpac` targets.
- [x] Implement content-hashed extraction cache to avoid re-extracting unmodified packages.
- [x] Scan and register all native shared libraries in `<bundle>/lib/` into `RuntimeOptions::native_libraries`.
- [x] Configure `RuntimeOptions::resource_directory` to point to `<bundle>/resources/`.
- [x] Update `tests/commands/TESTS.md` with bundle execution test cases (Cases 20.6 to 20.10).
- [x] Implement Catch2 unit & integration tests in `tests/commands/test_run_bundle.cpp`.
- [x] Update `docs/spec/cli/run.md` documentation.
- [x] Verify 100% test pass rate across all test suites.

---

## Phase 8: Standard Library (`solixlib`) Resource & Filesystem Dual-Location API

- **Priority**: `P1 High`
- **Affected Modules**: `solixlib/project/src/solix/io/filesystem/Resource.slx`, `File.slx`, `Path.slx`, `solixlib/native/src/io_fs.cpp`, `solixlib/project/manifest.json`, `tests/solixlib/test_resource.cpp`, `tests/solixlib/TESTS.md`, `docs/spec/solixlib/resource.md`
- **Status**: - [x] Completed & Merged (2026-10-17)

### Objective
Enhance `solixlib` to provide clean, dual-location filesystem operations—allowing Solix programs to explicitly select whether to read from internal bundled application resources (`resources/`) or from outside the application (host filesystem).

### Action Items
- [x] Implement native runtime hooks in `solixlib/native/src/io_fs.cpp` (`native_get_resource_root`, `native_resource_exists`, `native_read_resource_all_text`).
- [x] Author `solix.io.filesystem.Resource` class with `exists`, `read_all_text`, `read_all_lines`, `get_path`, and `get_resource_directory`.
- [x] Extend `solix.io.filesystem.File` with `read_resource_text`, `resource_exists`, and `read_all_text(path, is_resource)`.
- [x] Support `res://` virtual URI prefix scheme in `Path` and `File`.
- [x] Support transparent development fallback to `<project_root>/resources/` when running unbundled (`solix run .`).
- [x] Register new files in `solixlib/project/manifest.json`.
- [x] Update `tests/solixlib/TESTS.md` (Part XI / Section 16: Cases 16.1 to 16.4).
- [x] Implement Catch2 test suite in `tests/solixlib/test_resource.cpp`.
- [x] Author specification in `docs/spec/solixlib/resource.md`.
- [x] Verify 100% test pass rate across all test suites.

---

## Phase 9: End-to-End Cross-Platform Verification, Tests & Documentation

- **Priority**: `P2 Polish`
- **Affected Modules**: `docs/`, `editors/vscode/`, `tests/`
- **Status**: - [x] Completed & Merged (2026-10-17)

### Objective
Perform end-to-end integration verification of packaging, resource loading, and execution across platforms, update user guides and IDE integration, and synchronize documentation.

### Action Items
- [x] Author comprehensive user guide in `docs/guide/packaging_and_resources.md`.
- [x] Synchronize CLI command reference in `docs/spec/cli/README.md`.
- [x] Update VS Code extension keywords/syntax if new standard library symbols or commands require highlighting.
- [x] Run full regression suite (`ctest --test-dir build --output-on-failure`) with zero warnings and zero sanitizer defects (80/80 passed 100%).
- [x] Finalize phase updates in `docs/PLAN.md`.

---

## Phase 10: Multi-Depth Dependency Asset & Native Library Staging in Build and Package

- **Priority**: `P1 High`
- **Affected Modules**: `cli/src/commands/build.cpp`, `cli/src/commands/pack.cpp`, `tests/commands/test_pack_command.cpp`, `tests/commands/TESTS.md`, `docs/spec/cli/build.md`, `docs/spec/cli/pack.md`, `docs/guide/packaging_and_resources.md`
- **Status**: - [x] Completed & Merged (2026-10-17)

### Objective
Ensure that multi-depth (transitive) dependency libraries automatically have their native libraries (`lib/`) and application resources (`resources/`) staged into the build output directory during `solix build`, and merged into the single-file distribution archive during `solix pack`. Root project assets take precedence over dependency assets on collision.

### Action Items
- [x] Implement `stage_native_libraries` in `cli/src/commands/build.cpp` to stage native libraries from root and all transitive dependencies into `<output_dir>/lib/`.
- [x] Implement `stage_resources` in `cli/src/commands/build.cpp` to stage static resources from root and all transitive dependencies into `<output_dir>/resources/` with root overrides.
- [x] Update `pack_project` in `cli/src/commands/pack.cpp` to collect and merge resources from all transitive dependency roots into the package archive.
- [x] Update `tests/commands/TESTS.md` with multi-depth dependency staging test specifications (Cases 20.11 and 20.12).
- [x] Implement Catch2 unit/integration tests in `tests/commands/test_pack_command.cpp` with multi-depth transitive dependency chains (`App -> LibA -> LibB`).
- [x] Update CLI specifications (`docs/spec/cli/build.md`, `docs/spec/cli/pack.md`) and user guide (`docs/guide/packaging_and_resources.md`).
- [x] Verify 100% test pass rate across all test suites with zero compiler warnings.




