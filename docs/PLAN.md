# Solix Engineering Master Plan — Post-v1.1.0 Integration Cycle

This document tracks all planned, in-progress, and completed engineering phases for the next Solix development and integration cycle following the v1.0.0 General Availability release.

> **Historical Archive**: Phases 1 through 58 of the initial v1.0.0 GA development cycle are archived at [`docs/archive/PLAN_v1.0.0.md`](archive/PLAN_v1.0.0.md).

---

## Phase Index & Status

| Phase | Title | Priority | Status |
|---|---|---|---|
| [Phase 1](#phase-1-post-v100-repository-hygiene-and-integration-baseline) | Post-v1.0.0 Repository Hygiene & Integration Baseline | P0 Blocker | In Progress |

---

## Phase 1: Post-v1.0.0 Repository Hygiene & Integration Baseline

- **Priority**: `P0 Blocker`
- **Affected Modules**: `.gitignore`, `solixlib/project/lib/`, `editors/vscode/`, `docs/PLAN.md`, `docs/archive/`
- **Status**: - [x] Completed & Merged

### Objective
Purge non-source binary artifacts, update repository ignore rules, and establish a clean integration baseline starting at Phase 1 for post-v1.0.0 engineering:
1. **Binary Artifact Purge**:
   - Remove tracked MSVC symbol database (`solixlib/project/lib/solixlib_native.pdb`).
   - Remove tracked packaged extension bundle (`editors/vscode/solix-1.0.0.vsix`) so the repository only tracks source files, with releases building the `.vsix` on demand via npm/vsce.
2. **Repository Ignore Rules Hardening**:
   - Update `.gitignore` to reject all Windows MSVC debug and symbol files (`*.pdb`, `*.ilk`, `*.exp`, `*.lib`).
   - Restore comprehensive ignore pattern for `*.vsix`.
3. **Plan Reset & Archiving**:
   - Archive the complete v1.0.0 development history (Phases 1–58) to `docs/archive/PLAN_v1.0.0.md`.
   - Reset `docs/PLAN.md` to begin Phase 1 of the new cycle.

### Action Items
- [x] Archive previous master plan to `docs/archive/PLAN_v1.0.0.md`.
- [x] Reset `docs/PLAN.md` for the new integration cycle.
- [x] Purge tracked `solixlib_native.pdb` from git.
- [x] Purge tracked `solix-1.0.0.vsix` from git.
- [x] Update `.gitignore` with `*.pdb`, `*.ilk`, `*.exp`, `*.lib`, and `*.vsix`.
- [x] Clean in-source build artifacts and ensure a clean working tree.
- [x] Verify full regression test suite passes 100%.

### Acceptance Criteria
- Zero untracked or unwanted binary files in repository.
- `docs/PLAN.md` reset to Phase 1 with previous plan archived.
- Working tree clean and all regression tests pass.
