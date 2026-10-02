# Solix Project Manifest (`solix.json`) Specification

The `solix.json` file is the declarative build and package descriptor for Solix projects. It defines package metadata, source dependencies, compilation profiles, and runtime parameters.

---

## Schema Overview

A complete `solix.json` manifest structure:

```json
{
  "project": "my_app",
  "version": "1.0.0",
  "author": "Alice Dev <alice@example.com>",
  "description": "High-performance processing application",
  "license": "Apache-2.0",
  "tags": ["network", "fast"],
  "dependencies": [
    {
      "type": "source",
      "path": "src/main.slx"
    },
    {
      "type": "source",
      "path": "src/utils.slx"
    }
  ],
  "profiles": {
    "debug": {
      "output_directory": "build/debug",
      "relative_paths": true,
      "exe_filename": "out.slxbin",
      "asm_filename": "out.slxasm",
      "compilation": {
        "entry_point": "main",
        "multithreaded": false,
        "additional_dependencies": [],
        "logs": {
          "level": "debug",
          "flush_level": "debug",
          "filename": "logs/debug.log",
          "sink": "STDOUT",
          "pattern": "[%^%-8l%$] [%-12n] %v"
        }
      },
      "runtime": {
        "heap_size": null,
        "stack_size": null,
        "arguments": []
      }
    },
    "release": {
      "output_directory": "build/release",
      "relative_paths": true,
      "exe_filename": "out.slxbin",
      "asm_filename": "out.slxasm",
      "compilation": {
        "entry_point": "main",
        "multithreaded": true,
        "additional_dependencies": [],
        "logs": {
          "level": "warning",
          "flush_level": "warning",
          "filename": "logs/release.log",
          "sink": "STDOUT",
          "pattern": "[%^%-8l%$] [%-12n] %v"
        }
      },
      "runtime": {
        "heap_size": null,
        "stack_size": null,
        "arguments": []
      }
    }
  }
}
```

---

## Field Specifications

### 1. Root Metadata Fields

| Field | Type | Required | Description |
|---|---|---|---|
| `project` | String | **Yes** | Project or package identifier name. |
| `version` | String | **Yes** | Semantic version string (e.g. `0.1.0`, `1.2.3`). |
| `author` | String / `null` | No | Author name and/or contact email. |
| `description` | String / `null` | No | Human-readable project description. |
| `license` | String / `null` | No | SPDX license identifier (e.g. `MIT`, `Apache-2.0`). |
| `tags` | Array of Strings | No | Descriptive keywords and tags for package discovery. |

### 2. Dependencies (`dependencies`)

An array of dependency objects required for building the project. Solix supports two dependency resolver types:

#### Source Dependency (`type: "source"`)
Direct source file dependency.

```json
{
  "type": "source",
  "path": "src/helper.slx"
}
```

| Subfield | Type | Required | Description |
|---|---|---|---|
| `type` | String | **Yes** | Must be `"source"`. |
| `path` | String | **Yes** | Relative or absolute path to the dependent `.slx` source file. |

#### Project Dependency (`type: "project"`)
A complete external or local Solix project dependency.

```json
{
  "type": "project",
  "name": "math_lib",
  "version": "1.2.0",
  "path": "../math_lib"
}
```

| Subfield | Type | Required | Description |
|---|---|---|---|
| `type` | String | **Yes** | Must be `"project"`. |
| `name` | String | **Yes** | Unique project/package name matching the target `solix.json`. |
| `version` | String | **Yes** | Semantic version requirement string (e.g. `1.2.0`). |
| `path` | String | No | Relative or absolute filesystem path to the project directory (or its `solix.json`). If omitted, Solix automatically looks up the package in `$SOLIX_HOME` via `PackageManager`. |

#### Dependency Resolution & SemVer Rules
Before any compilation begins, Solix performs graph walking and semantic version resolution across all direct and transitive project dependencies:
- **Transitive Discovery**: If project A depends on project B, and project B depends on project C, all three projects and their source trees are discovered and linked.
- **Circular / Mutual Dependencies**: Circular project dependencies (e.g. A depends on B and B depends on A) are recognized and de-duplicated gracefully without error or infinite recursion.
- **SemVer Conflict Resolution**:
  - **Different Major Versions**: Halts immediately with a pre-compilation **Error**. Breaking API changes across major versions cannot be resolved automatically.
  - **Same Major, Different Minor Versions**: Outputs a **Warning** and automatically selects the candidate with the highest minor version.
  - **Same Major and Minor, Different Patch Versions**: Silently and cleanly selects the candidate with the highest patch version.
- **Pre-Compilation Verification**: If any required manifest path or source file cannot be found anywhere in the dependency graph, `solix build` aborts with an informative error before invoking the compiler.

### 3. Build Profiles (`profiles`)

A dictionary mapping profile names (`debug`, `release`, `test`, or user-defined profiles) to profile configurations:

| Subfield | Type | Default | Description |
|---|---|---|---|
| `output_directory` | String | `build/<profile>` | Directory where compiled artifacts are stored. |
| `relative_paths` | Boolean | `true` | Whether paths should be computed relative to project root. |
| `exe_filename` | String | `out.slxbin` | Filename for the generated executable bytecode artifact. |
| `asm_filename` | String | `""` | Filename for emitted disassembly listing (disabled if empty). |
| `compilation` | Object | (Required) | Compilation flags, log configuration, and entry method. |
| `runtime` | Object | (Optional) | Default VM execution parameters (stack, heap, default args). |

#### Compilation Block (`profiles.<name>.compilation`)

| Subfield | Type | Default | Description |
|---|---|---|---|
| `entry_point` | String | `"main"` | Entry method name to execute upon start. |
| `multithreaded` | Boolean | `false` | Enable multithreaded compiler stages. |
| `additional_dependencies` | Array | `[]` | Profile-specific dependencies (e.g. test utilities in `test` profile). |
| `logs.level` | String | `"info"` | Compiler log severity filter (`trace`, `debug`, `info`, `warning`, `error`, `critical`, `off`). |
| `logs.flush_level` | String | `"error"` | Log flush threshold. |
| `logs.sink` | String | `"STDOUT"` | Destination sink (`STDOUT`, `STDERR`, `BASIC_FILE`, `CONSOLE_AND_FILE`). |
| `logs.filename` | String | `""` | Log file path if a file sink is chosen. |
| `logs.pattern` | String | standard | Spdlog format pattern. |
