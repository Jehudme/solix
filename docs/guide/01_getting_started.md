# Getting Started with Solix

Solix is a statically typed, object-oriented programming language designed for high performance and deterministic memory management through Automatic Reference Counting (ARC).

This guide walks you through building the toolchain, scaffolding a new project, building and running applications, and managing local packages with the `solix` CLI.

---

## Table of Contents

1. [Prerequisites & Building from Source](#prerequisites--building-from-source)
2. [CLI Toolchain Overview](#cli-toolchain-overview)
3. [Your First Project (`solix new`)](#your-first-project-solix-new)
4. [Building Your Project (`solix build`)](#building-your-project-solix-build)
5. [Running Your Application (`solix run`)](#running-your-application-solix-run)
6. [Compiling Individual Files (`solix compile`)](#compiling-individual-files-solix-compile)
7. [Local Package Management (`install`, `list`, `details`, `uninstall`)](#local-package-management)
8. [Exit Codes & Return Values](#exit-codes--return-values)

---

## Prerequisites & Building from Source

### Prerequisites

| Tool | Minimum Version | Purpose |
|---|---|---|
| CMake | 3.20 | Build configuration and target generation |
| C++ Compiler | C++20 (GCC 11+, Clang 13+, MSVC 2019+) | Language engine compilation |
| Git | Recent version | Fetching dependencies |

### Clone and Build

```bash
git clone https://github.com/yourorg/solix.git
cd solix
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

After building, the unified CLI executable is located at:
- **`build/launcher/solix`** (with symlink/alias **`build/launcher/solix_launcher`**)

You can optionally install it system-wide:
```bash
sudo cmake --install .
```

---

## CLI Toolchain Overview

The `solix` executable provides a cohesive set of subcommands:

```bash
solix <subcommand> [options] [arguments...]
```

| Subcommand | Function | Reference |
|---|---|---|
| `new` | Scaffolds a new project with directory structure and manifest | [docs/spec/cli/new.md](../spec/cli/new.md) |
| `build` | Builds project artifacts from `solix.json` profiles | [docs/spec/cli/build.md](../spec/cli/build.md) |
| `run` | Executes compiled bytecode binaries on the Solix VM | [docs/spec/cli/run.md](../spec/cli/run.md) |
| `compile` | Directly compiles source files into bytecode | [docs/spec/cli/compile.md](../spec/cli/compile.md) |
| `install` | Installs a local project into `$SOLIX_HOME` | [docs/spec/cli/package.md](../spec/cli/package.md) |
| `uninstall` | Uninstalls an installed project by name and version | [docs/spec/cli/package.md](../spec/cli/package.md) |
| `list` | Lists all installed projects in tabular format | [docs/spec/cli/package.md](../spec/cli/package.md) |
| `details` | Shows comprehensive metadata for an installed project | [docs/spec/cli/package.md](../spec/cli/package.md) |

---

## Your First Project (`solix new`)

To create a new Solix project:

```bash
solix new hello_solix
cd hello_solix
```

This generates a standard project structure:

```
hello_solix/
├── solix.json             # Declarative project and build manifest
└── src/
    └── main.slx           # Starter source code with entry method
```

### Understanding `src/main.slx`

```solix
static int32 main() {
    return 0;
}
```

Every executable Solix program defines an entry point (default name `main`). The return value of `main()` is propagated directly to the host operating system shell as the process exit code.

---

## Building Your Project (`solix build`)

Build the default `debug` profile:

```bash
solix build
```

This resolves project dependencies from `solix.json` and outputs the executable bytecode binary to `build/debug/out.slxbin`.

To compile an optimized `release` profile:

```bash
solix build -p release
```

Output: `build/release/out.slxbin`.

---

## Running Your Application (`solix run`)

Execute the compiled bytecode on the Solix Virtual Machine:

```bash
solix run build/debug/out.slxbin
```

You can pass arguments to your program after the bytecode file:

```bash
solix run build/debug/out.slxbin arg1 arg2 123
```

---

## Compiling Individual Files (`solix compile`)

If you want to compile standalone source files without a manifest:

```bash
solix compile src/main.slx -o app.slxb
```

To emit disassembly text alongside the bytecode:

```bash
solix compile src/main.slx -o app.slxb -a app.s
```

---

## Local Package Management

Solix includes local package management commands for sharing and inspecting reusable libraries on your machine.

### 1. Install a Project
From your project directory:

```bash
solix install .
```

The package manager computes a deterministic 16-hex SHA-256 identifier (e.g., `8df34a2e57b901fc`) from your project name and version, copies the project into `$SOLIX_HOME/installed/<id>/`, and records it in `$SOLIX_HOME/installed.json`.

### 2. List Installed Packages
```bash
solix list
```

Output:
```
Installed Solix Projects (1):

NAME                     VERSION     ID                INSTALLED PATH
--------------------------------------------------------------------------------
hello_solix              0.1.0       8df34a2e57b901fc  /home/user/.solix/installed/8df34a2e57b901fc
```

### 3. Inspect Package Details
```bash
solix details hello_solix
```

Displays package author, version, disk usage, build profiles, and declared dependencies.

### 4. Uninstall a Package
To remove an installed package, specify both the name and version:

```bash
solix uninstall hello_solix 0.1.0
```

---

## Exit Codes & Return Values

| Exit Code | Meaning |
|---|---|
| `0` | Success / program returned 0 |
| `1` | Error (syntax, semantic, missing file, or unhandled runtime fault) |
| `N` (`N > 0`) | Custom integer exit code returned by `main()` |

For example, a program returning 42:

```solix
static int32 main() {
    return 42;
}
```

When compiled and executed:
```bash
solix compile check.slx -o check.slxb
solix run check.slxb
echo $?
# 42
```
