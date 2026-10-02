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

Installs a local Solix project directory into the `$SOLIX_HOME` repository.

### Synopsis
```bash
solix install [path] [options]
```

### Arguments & Options
| Argument / Option | Type | Default | Description |
|---|---|---|---|
| `[path]` | Path | `.` (Current directory) | Path to project folder containing `solix.json`. |
| `--force`, `-f` | Flag | `false` | Reinstall and overwrite if already installed. |

### Behavior & Validations
1. Validates that `solix.json` exists in the target directory and contains valid `project` and `version` fields.
2. Computes the 16-hex hash ID.
3. If already present in `installed.json` and `--force` is not set, halts with an error.
4. Copies the project directory into `$SOLIX_HOME/installed/<id>/`.
5. Updates `$SOLIX_HOME/installed.json` with installation timestamp and metadata.

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

Lists all installed Solix projects in a formatted summary table.

### Synopsis
```bash
solix list
```

### Output Format
If no projects are installed:
```
No Solix projects are currently installed.
```

When projects are present:
```
Installed Solix Projects (2):

NAME                     VERSION     ID                INSTALLED PATH
--------------------------------------------------------------------------------
math_lib                 1.0.0       8df34a2e57b901fc  /home/user/.solix/installed/8df34a2e57b901fc
logger_util              2.1.0       c40b8e192aa3f7d1  /home/user/.solix/installed/c40b8e192aa3f7d1
```

---

## `solix details`

Displays extensive metadata, file sizes, dependencies, and configured profiles for an installed package.

### Synopsis
```bash
solix details <name> [version]
```

### Arguments
| Argument | Type | Required | Description |
|---|---|---|---|
| `<name>` | String | **Yes** | Name of the installed package. |
| `[version]` | String | No | Version of the package. Optional if only one version is installed. |

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
