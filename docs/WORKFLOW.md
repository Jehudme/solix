# Solix Engineering Workflow & Development Lifecycle

This document defines the mandatory engineering process and quality standards for all development on the Solix programming language, compiler, runtime, launcher, and toolchain.

---

## Universal Scope & Core Invariants

The rules and requirements described herein apply universally to **all language statements, language features, compiler/runtime internals, CLI commands, and the official Standard Library (`solixlib`)**.

Whenever any construct, feature, command, or standard library module is added, modified, or refactored:
1. **Mandatory Test Updates**: Tests must be created or updated covering **ALL positive scenarios** (valid usage, syntax, arguments, options, flags, runtime output guarantees) and **ALL negative scenarios** (syntax errors, invalid types, missing arguments, unhandled faults, out-of-bounds inputs, exit codes, thrown exceptions).
2. **Mandatory Documentation Updates**: Formal specifications, developer guides, and references must be created or updated in lockstep with code changes. Standard library modules must have dedicated specification markdown files in `docs/spec/solixlib/`.
3. **Mandatory Phase Identification**: In every phase, the engineering plan ([`PLAN.md`](PLAN.md)) must explicitly enumerate:
   - The specific code modules, classes, and interfaces being modified or introduced.
   - The specific test suites and test cases (positive and negative) required.
   - The specific documentation files and sections to be created or updated.

---

## Step-by-Step Development Lifecycle

Every phase, feature, or bugfix must strictly follow this sequential lifecycle:

### 1. Dedicated Branching
Create a dedicated feature branch off `master`:
```bash
git checkout -b phase-N-descriptive-name
```
Never commit directly to `master`.

### 2. Implementation
Implement code changes according to the architectural design, keeping code modular, self-contained, and cross-platform portable.

### 3. Commit Implementation
Commit the code changes before proceeding to tests or documentation:
```bash
git commit -m "feat/fix: <description>"
```

### 4. Test & Documentation Specification Analysis
Before writing test code or documentation:
- **Analyze Scope**: Determine all affected statements, language constructs, CLI commands, or standard library (`solixlib`) modules.
- **Identify Test Matrix**: Formulate comprehensive positive and negative test scenarios for every function, exercising OOP inheritance, interface dispatch, and exception handling.
- **Update Master Test Specifications**:
  - For language statements/constructs: update [`tests/statements/TESTS.md`](../tests/statements/TESTS.md).
  - For CLI commands and toolchain subcommands: update [`tests/commands/TESTS.md`](../tests/commands/TESTS.md).
  - For standard library (`solixlib`) submodules: update [`tests/solixlib/TESTS.md`](../tests/solixlib/TESTS.md).
  - Every newly added test scenario must initially be tagged with `[NOT IMPLEMENTED]`.
- **Identify Documentation Deliverables**:
  - For standard library (`solixlib`) submodules: formal specification markdown files under `docs/spec/solixlib/` (e.g. `console.md`, `string.md`, `exceptions.md`, `filesystem.md`).
  - For CLI commands: formal command specifications in `docs/spec/cli/` and user guides in `docs/guide/`.
  - For language statements: syntax and bytecode mechanics in `docs/spec/statements/` and keyword entries in `docs/wiki/keywords/`.
  - For architecture/runtime: internals in `docs/architecture/` and formal ISA/type rules in `docs/spec/`.

### 5. Commit Test Specifications
Commit the updated test specification document with `[NOT IMPLEMENTED]` tags:
```bash
git commit -m "test(spec): define test specifications in TESTS.md"
```

### 6. Test Suite Implementation & Verification
- Implement Catch2 unit & integration tests:
  - Language statements: organized cleanly under `tests/statements/` (`modules/`, `declarations/`, `control_flow/`, `expressions/`).
  - CLI subcommands: organized under `tests/commands/` using isolated sandbox fixtures (`TempDir`, `run_cli`).
  - Standard library (`solixlib`) submodules: organized under `tests/solixlib/` with dedicated test suites per module (verifying positive execution, return values, and expected exceptions thrown).
- Ensure tests verify both positive execution results (artifacts, output strings, exit code 0) and negative failure conditions (error messages, invalid argument detection, non-zero exit codes, expected exceptions caught).
- As test scenarios are implemented and verified to pass, update their tags in the corresponding `TESTS.md` from `[NOT IMPLEMENTED]` to `[IMPLEMENTED]`.

### 7. Documentation Implementation & Updates
- Author or update all identified documentation artifacts:
  - Standard library: ensure every submodule has a comprehensive markdown spec in `docs/spec/solixlib/`.
  - Ensure documentation contains real, working examples matching the current codebase.
  - Remove all legacy, obsolete, or purged features (e.g. legacy stdlib assumptions).
  - Update top-level documentation indexes (`docs/spec/README.md`, `docs/guide/README.md`, etc.).

### 8. Incremental Commit of Tests and Documentation
Commit test suites and documentation incrementally:
```bash
git commit -m "test(<subsystem>): implement Catch2 test suite for <construct>"
git commit -m "docs(<subsystem>): add formal specification and guide for <feature>"
```

### 9. Full Test Suite Verification
Run the complete regression test suite across all statements and CLI commands:
```bash
ctest --test-dir build --output-on-failure
```
Ensure 100% pass rate before requesting merge.

### 10. Non-Fast-Forward Merge
Merge the feature branch back into `master` using non-fast-forward merge (`--no-ff`) to preserve the feature's commit history:
```bash
git checkout master
git merge --no-ff phase-N-descriptive-name -m "Merge branch 'phase-N-descriptive-name' into master"
git branch -d phase-N-descriptive-name
```
Update [`docs/PLAN.md`](PLAN.md) to record the phase as completed.
