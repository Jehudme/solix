# Subcommand Specification: `solix ide`

The `solix ide` command provides IDE integration, editor tooling management, and automated extension installation across supported editors.

---

## Table of Contents

1. [Synopsis](#synopsis)
2. [Description](#description)
3. [Subcommands & Options](#subcommands--options)
4. [Supported Editors](#supported-editors)
5. [Bundled Extension Discovery](#bundled-extension-discovery)
6. [Standard Library Auto-Discovery](#standard-library-auto-discovery)
7. [Exit Codes](#exit-codes)
8. [Examples](#examples)

---

## Synopsis

```bash
solix ide install [options]
```

Global help:
```bash
solix ide --help
solix ide install --help
```

---

## Description

The `solix ide` suite provides convenient developer tooling management. The primary action, `solix ide install`, automatically locates the bundled Solix IDE extension (`solix-0.1.0.vsix`) provided in the toolchain installation and invokes the appropriate editor CLI to install it without manual drag-and-drop or marketplace configuration.

---

## Subcommands & Options

### `solix ide install`

Installs the bundled Solix VS Code / Cursor extension into detected or specified IDEs.

| Option | Short | Description | Default |
|---|---|---|---|
| `--editor <name>` | `-e` | Specific editor binary to target (`code`, `code-insiders`, `cursor`, `codium`) | Scan all supported editors |
| `--help` | `-h` | Display usage and help message | — |

---

## Supported Editors

The command natively supports any VS Code-compatible CLI:

- **`code`**: Visual Studio Code (Standard)
- **`code-insiders`**: Visual Studio Code Insiders
- **`cursor`**: Cursor AI Editor
- **`codium`**: VSCodium (Open-source VS Code)

When run without `--editor`, `solix ide install` iterates through all supported editor binaries and installs the extension into every editor found in the system `PATH`. If none are detected, it gracefully informs the user and prints the exact path of the bundled `.vsix` file for manual installation.

When run with `--editor <name>`, the command targets only the specified binary. If the binary is not found in `PATH`, the command exits with an error (exit code 1).

---

## Bundled Extension Discovery

The toolchain dynamically locates the bundled extension relative to the running `solix` executable:

1. **Installed layout**: `<prefix>/share/solix/vscode/solix-0.1.0.vsix`
2. **Development layout**: `<repo>/editors/vscode/solix-0.1.0.vsix`

---

## Standard Library Auto-Discovery

Complementing IDE extension management, the Solix toolchain automatically discovers and registers the standard library (`solixlib`) relative to the executable path:

1. **Installed layout**: `<prefix>/share/solix/solixlib`
2. **Development layout**: `<repo>/solixlib/project`

When a project references `solixlib` as a dependency and `$SOLIX_HOME` does not yet contain it, the toolchain automatically seeds and installs the bundled standard library into the user's local package repository on first use.

---

## Exit Codes

| Exit Code | Meaning |
|---|---|
| `0` | Success (extensions installed or help printed) |
| `1` | Bundled extension not found, or specified editor binary not found in `PATH` |

---

## Examples

### Install Extension into All Detected IDEs

```bash
solix ide install
```

Output:
```text
Installing Solix extension into code...
  ✓ Extension successfully installed for code.
Installing Solix extension into cursor...
  ✓ Extension successfully installed for cursor.
```

### Install Extension into a Specific Editor

```bash
solix ide install --editor cursor
```

### Handle Non-Existent Editor Gracefully

```bash
solix ide install --editor non_existent_editor
```

Output:
```text
Error: Specified editor binary 'non_existent_editor' was not found in PATH.
```
Exits with code 1.
