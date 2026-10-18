# Solix Local Package Management — `install`, `uninstall`, `list`, `details`

Solix provides local package management commands to install, inspect, and manage reusable Solix projects across the user environment.

---

## Table of Contents

1. [Storage Architecture & Hash IDs](#storage-architecture--hash-ids)
2. [`solix install`](#solix-install)
3. [`solix uninstall`](#solix-uninstall)
4. [`solix list`](#solix-list)
5. [`solix details`](#solix-details)

---

## Storage Architecture & Hash IDs

All package management operations center around the **`SOLIX_HOME`** directory:

```
$SOLIX_HOME/
├── installed.json           # Central registry of all installed packages
└── installed/               # Package installations by unique hash ID
    ├── 1a2b3c4d5e6f7890/    # Folder name equals the 16-character hash ID
    │   ├── solix.json
    │   └── src/
    └── 9876543210fedcba/
```

### Deterministic SHA-256 Hash Identifiers
Package IDs are computed deterministically from the combination of package name and version:
$$\text{Package Key} = \text{name} + \text{"@"} + \text{version}$$
$$\text{ID} = \text{SHA256}(\text{Package Key})[0..15]$$

This ensures:
- Unique package folders for each version.
- Re-installation produces identical IDs.
- Fast, collision-free lookup.

---

## `solix install`

Installs a local Solix project directory into the `$SOLIX_HOME` repository or an export destination.

### Synopsis
```bash
solix install [path] [options]
```

### Arguments & Options
| Argument / Option | Type | Default | Description |
|---|---|---|---|
| `[path]` | Path | `.` (Current directory) | Path to project folder, manifest (`solix.json`/`manifest.json`), or `.slxpac` package bundle. |
| `-p`, `--profile` | String | `release` | Compilation profile to use during build before installation. |
| `-o`, `--output` | Path | `""` | Optional export directory path. If specified, stages the prepared distribution directory to this path without registering into the global registry. |
| `--archive`, `--as-is` | Flag | `false` | Installs an already prepared distribution directory or bundle directly without invoking the compiler or build scripts. |
| `--alias`, `--as` | String | `""` | Installs or stages the project under an alternative package name. |
| `--force`, `-f` | Flag | `false` | Reinstall and overwrite if already installed. |

### Behavior & Clean Staging
1. Supports installing directly from project directories, manifest files, or pre-built `.slxpac` package bundles.
2. If given a `.slxpac` package bundle:
   - Extracts bundle contents into a staging location and copies into `$SOLIX_HOME/installed/<id>/` without rebuilding.
3. If `--archive` (or `--as-is`) is set:
   - Copies the distribution directory directly to `$SOLIX_HOME/installed/<id>/` without running builds or pre-compile hooks.
4. Otherwise:
   - Pre-installs any local path dependencies (with collision warnings if already present in the registry).
   - Builds the project using the requested profile (default: `release`).
   - If project is a **Library** (`"type": "library"`):
     - Copies its own `src/` directory (excluding any local testing `main.slx`).
     - Stages native shared libraries in `lib/` and merged assets in `resources/`.
     - Emits a clean, reduced `manifest.json` with `"installed": true`, stripping build profiles, commands, and bytecode, while retaining normalized external package dependencies.
   - If project is a **Plugin** (`"type": "plugin"`):
     - Stages `src/` (if present), native shared libraries in `lib/`, and merged assets in `resources/`.
     - Emits a clean, reduced `manifest.json` with `"type": "plugin"`.
   - If project is an **Application** (`"type": "app"`):
     - Stages compiled `bytecode.slxbin` at the root.
     - Stages native shared libraries in `lib/` and merged assets in `resources/`.
     - Strips source code (`src/`), build profiles, and commands.
     - Emits a clean, reduced `manifest.json` with `"installed": true` and `"entry": "bytecode.slxbin"`.
5. If `-o, --output` is specified, writes to the given directory without registering in `$SOLIX_HOME/installed.json`. Otherwise, installs to `$SOLIX_HOME/installed/<id>/` and updates the registry.

---

## `solix archive`

Builds the project and exports a clean, simplified distribution directory containing only the essentials (without registering into `$SOLIX_HOME`).

### Synopsis
```bash
solix archive [path] [options]
```

### Arguments & Options
| Argument / Option | Type | Default | Description |
|---|---|---|---|
| `[path]` | Path | `.` | Target project path. |
| `-p`, `--profile` | String | `release` | Build profile to use. |
| `-o`, `--output` | Path | (Required) | Destination directory for the exported distribution. |
| `--alias`, `--as` | String | `""` | Export staged distribution under an alternative project name. |
| `-f`, `--force` | Flag | `false` | Overwrite destination directory if it already exists. |

---

## `solix uninstall`

Removes an installed project from the local registry and deletes its storage directory.

### Synopsis
```bash
solix uninstall <name> <version>
```

### Arguments
| Argument | Type | Required | Description |
|---|---|---|---|
| `<name>` | String | **Yes** | Exact name of the project to uninstall. |
| `<version>` | String | **Yes** | Exact semantic version to uninstall. |

### Behavior
1. Computes the deterministic hash ID from `<name>` and `<version>`.
2. Verifies the entry exists in `$SOLIX_HOME/installed.json`.
3. Recursively deletes `$SOLIX_HOME/installed/<id>/`.
4. Removes the record from `installed.json`.

---

## `solix list`

Lists installed Solix projects with project type, filtering, and sorting.

### Synopsis
```bash
solix list [options]
```

### Options & Flags
| Option | Shorthand | Type | Default | Description |
|---|---|---|---|---|
| `--type` | `-t` | String | `""` | Filter by project type (`app`, `library`, `plugin`). |
| `--filter` | `-f` | String | `""` | Case-insensitive query to filter by name, description, or ID. |
| `--sort`, `--order` | `-s` | String | `name` | Sort field: `name`, `version`, `type`, `date`. |
| `--reverse` | | Flag | `false` | Reverse sort order. |
| `--json` | | Flag | `false` | Output list as raw JSON array. |

### Output Format
If no projects are installed:
```
No Solix projects are currently installed.
```

When projects are present:
```
Installed Solix Projects (3):

NAME                     VERSION     TYPE        ID                INSTALLED PATH
------------------------------------------------------------------------------------------------
math_lib                 1.0.0       library     8df34a2e57b901fc  /home/user/.solix/installed/8df34a2e57b901fc
logger_util              2.1.0       library     c40b8e192aa3f7d1  /home/user/.solix/installed/c40b8e192aa3f7d1
my_app                   1.0.0       app         7f8e9a0b1c2d3e4f  /home/user/.solix/installed/7f8e9a0b1c2d3e4f
```

---

## `solix details`

Displays extensive metadata, file sizes, dependencies, and configured profiles for an installed package or a `.slxpac` package bundle.

### Synopsis
```bash
solix details <target> [version] [options]
```

Target can be specified as:
- An installed package name (e.g. `solix details math_lib 1.0.0` or `solix details math_lib@1.0.0`)
- A `.slxpac` package bundle path (e.g. `solix details my_app.slxpac`)

### Scripting Query Flags
| Flag | Description |
|---|---|
| `-v`, `--version` | Print only the project version string. |
| `-n`, `--name` | Print only the project name. |
| `-t`, `--type` | Print only the project type (`app`, `library`, `plugin`). |
| `-d`, `--description` | Print only the project description. |
| `-a`, `--author` | Print only the project author. |
| `-l`, `--license` | Print only the project license. |
| `--id` | Print only the computed project ID. |
| `--path` | Print only the installed filesystem path. |
| `--entry` | Print only the entry point binary. |
| `--json` | Print full project metadata as formatted JSON. |

### Ambiguity Handling
If `[version]` is omitted:
- If exactly **one** version is installed, that version's details are displayed automatically.
- If **multiple** versions are installed, the command lists the available versions with their IDs and asks the user to specify a version.

### Sample Output
```
=================================================================
Project:      math_lib v1.0.0
ID:           8df34a2e57b901fc
Installed At: 2026-10-02 02:40:00
Path:         /home/user/.solix/installed/8df34a2e57b901fc
Original:     /home/user/Projects/math_lib
Disk Size:    42.5 KB
Description:  Fast mathematics utilities
Author:       Jane Doe <jane@example.com>
License:      MIT
Tags:         math, algorithms

Dependencies (1):
  - Type: source, Path: src/main.slx

Defined Profiles:
  [debug]
    Output Directory: build/debug
    Exe Filename:     out.slxbin
    Entry Point:      main
    Multithreaded:    false
  [release]
    Output Directory: build/release
    Exe Filename:     out.slxbin
    Entry Point:      main
    Multithreaded:    true
=================================================================
```

---

## Exit Codes

| Code | Condition |
|---|---|
| `0` | Command succeeded. |
| `1` | Missing package, manifest validation error, collision conflict, or filesystem error. |

---

## CPack Multi-OS Packaging & Bundling

In addition to user-level project package management (`solix install/uninstall`), Solix integrates CMake's **CPack** to build native release packages and standalone bundles for Linux, macOS, and Windows:

### Target Generators
- **Linux**:
  - `DEB`: Native `.deb` package for Debian and Ubuntu.
  - `RPM`: Native `.rpm` package for Fedora, CentOS, RHEL.
  - `TGZ`: Standalone portable tarball (`solix-<version>-Linux.tar.gz`).
- **macOS**:
  - `productbuild`: Native `.pkg` installer.
  - `TGZ`: Standalone portable tarball (`solix-<version>-Darwin.tar.gz`).
- **Windows**:
  - `NSIS`: Native `.exe` setup wizard with automatic `PATH` configuration.
  - `ZIP`: Standalone portable zip archive (`solix-<version>-win64.zip`).

### Bundled Components
1. **Binaries**: `bin/solix` (CLI launcher) and `bin/solix-lsp` (Language Server Protocol daemon).
2. **Standard Library**: `share/solix/solixlib/` containing all standard modules and compiled native library `libsolixlib_native`.
3. **IDE Integration**: `share/solix/vscode/solix-0.1.0.vsix` bundled extension for automated editor installation.
4. **Documentation**: `share/doc/solix/` containing `LICENSE` and `README.md`.

