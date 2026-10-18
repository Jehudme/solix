# Solix CLI & Toolchain Specification

The Solix toolchain provides a unified command-line executable (`solix`, aliased as `solix_launcher`) designed for building, compiling, executing, scaffolding, and managing Solix projects and local packages across Linux, macOS, and Windows.

---

## Table of Contents

1. [Architecture & Philosophy](#architecture--philosophy)
2. [Global Invocation Syntax](#global-invocation-syntax)
3. [Subcommand Matrix](#subcommand-matrix)
4. [Environment Variables](#environment-variables)
5. [Standard Exit Codes](#standard-exit-codes)
6. [Detailed Subcommand Specifications](#detailed-subcommand-specifications)

---

## Architecture & Philosophy

The Solix CLI is designed around three core principles:

1. **Subcommand Modularity**: Every task is encapsulated in a dedicated, self-contained subcommand with clear option validation, deterministic error handling, and structured diagnostics.
2. **Deterministic Artifact Placement**: Source code compilation and project builds emit output binaries into explicit, predictable locations without implicit system mutations.
3. **Isolated Package Management**: Package operations (`install`, `uninstall`, `list`, `details`) operate within an isolated, cross-platform user storage location (`SOLIX_HOME`) governed by a single authoritative manifest (`installed.json`) and deterministic SHA-256 package identifiers.

---

## Global Invocation Syntax

```bash
solix <subcommand> [options] [arguments...]
```

To display toolchain help or help for a specific subcommand:
```bash
solix --help
solix <subcommand> --help
```

---

## Subcommand Matrix

| Subcommand | Description | Specification Link |
|---|---|---|
| [`compile`](compile.md) | Compiles one or more `.slx` source files into bytecode (`.slxbin`) or executes | [compile.md](compile.md) |
| [`run`](run.md) | Executes compiled bytecode (`.slxbin`), Solix project, or package bundle (`.slxpac`) | [run.md](run.md) |
| [`build`](build.md) | Builds project artifacts from `solix.json` profiles and dependencies | [build.md](build.md) |
| [`pack`](pack.md) | Packages project or installed application into an archive (`.slxpac`) | [pack.md](pack.md) |
| [`create`](new.md) / [`new`](new.md) | Scaffolds a new Solix project (`app`, `library`, `plugin`) with standard structure | [new.md](new.md) |
| [`install`](package.md#solix-install) | Installs a local Solix project or package bundle into the package repository | [package.md#solix-install](package.md#solix-install) |
| [`archive`](package.md#solix-archive) | Builds and exports a clean, prepared project distribution directory | [package.md#solix-archive](package.md#solix-archive) |
| [`uninstall`](package.md#solix-uninstall) | Removes an installed project by name and version | [package.md#solix-uninstall](package.md#solix-uninstall) |
| [`list`](package.md#solix-list) | Lists installed projects with TYPE column, filtering, and sorting | [package.md#solix-list](package.md#solix-list) |
| [`details`](package.md#solix-details) | Displays project metadata, scripting query tags, or inspects `.slxpac` bundles | [package.md#solix-details](package.md#solix-details) |
| `version` | Displays toolchain version string (also via `-v` / `--version`) | [README.md](#global-invocation-syntax) |

---

## Environment Variables

| Variable | Description | Default Value |
|---|---|---|
| `SOLIX_HOME` | Root directory for local package installations and registry metadata | Windows: `%LOCALAPPDATA%\solix`<br>macOS: `~/Library/Application Support/solix`<br>Linux: `~/.solix` (or `$XDG_DATA_HOME/solix`) |
| `SOLIX_RESOURCE_ROOT` | Root directory for bundled application resources | Auto-configured when executing `.slxpac` archives; falls back to `<cwd>/resources` |

When `SOLIX_HOME` is defined, the package manager redirects all registry writes (`$SOLIX_HOME/installed.json`) and installed package directories (`$SOLIX_HOME/installed/<id>/`) to that folder. This provides full test isolation and sandbox reproducibility.

---

## Standard Exit Codes

| Exit Code | Classification | Meaning |
|---|---|---|
| `0` | Success | Command completed successfully with no errors |
| `1` | Error | Syntax error, semantic error, file not found, collision conflict, or unhandled runtime fault |
| `N` (`N > 1`) | User / Application Exit | Non-zero exit code explicitly returned by the Solix program's `main()` method |

---

## Detailed Subcommand Specifications

- [**compile Subcommand**](compile.md): Single/multi-file compiler, assembly dumping, log level controls.
- [**run Subcommand**](run.md): VM runtime execution, `.slxpac` bundle ingestion, memory limits, program argument passing.
- [**build Subcommand**](build.md): Profile-driven multi-file building, dependency resolution.
- [**pack Subcommand**](pack.md): Single-file distribution archive (`.slxpac`) generation, resource bundling, native library packaging.
- [**new Subcommand**](new.md): Project scaffolding, custom templates, collision protection.
- [**package Management (`install`, `uninstall`, `list`, `details`)**](package.md): Local project lifecycle, deterministic SHA-256 IDs, registry inspection.
- [**Project Manifest (`manifest.json`) Specification**](manifest.md): Complete schema documentation for project manifests.
