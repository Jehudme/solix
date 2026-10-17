# Solix Project Manifest (`manifest.json` / `solix.json`) Specification

The `manifest.json` file (with backward-compatible fallback to `solix.json`) is the declarative build and package descriptor for Solix projects and libraries. It defines package metadata, source dependencies, compilation profiles, and runtime parameters.

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

#### Project / Library Dependency (`type: "project"` or `type: "library"`)
A complete external or local Solix project or library dependency.

```json
{
  "type": "library",
  "name": "math_lib",
  "version": "1.2.0",
  "path": "../math_lib"
}
```

| Subfield | Type | Required | Description |
|---|---|---|---|
| `type` | String | **Yes** | Must be `"project"` or `"library"`. |
| `name` | String | **Yes** | Unique project/package name matching the target manifest (`manifest.json` or `solix.json`). |
| `version` | String | **Yes** | Semantic version requirement string (e.g. `1.2.0`). |
| `path` | String | No | Relative or absolute filesystem path to the project directory (or its `manifest.json` / `solix.json`). If omitted, Solix automatically looks up the package in `$SOLIX_HOME` via `PackageManager`. |
| `profile` | String | No | Explicit target build profile to resolve for this dependency (e.g. `"release"`). If omitted, inherits active profile or falls back to `"debug"`. |

#### Dependency Profile Resolution & Variable Propagation
When resolving project dependencies, Solix evaluates the target build profile for each candidate:
1. **Explicit Profile Selection**: If `"profile": "<name>"` is specified on the dependency object, that profile must exist in the target dependency manifest. If absent, build halts with:
   `Error: Dependency project '<name>' does not define requested profile '<profile>'. Available profiles: [...]`.
2. **Profile Inheritance**: If `"profile"` is omitted, the dependency inherits the active build profile name of the consuming project if defined in the dependency's manifest.
3. **Profile Fallback**: If the consuming project's active profile does not exist in the dependency, Solix falls back to `"debug"`; if `"debug"` is not defined, it falls back to the first available profile in the dependency manifest.
4. **Profile-Specific Variable & Dependency Propagation**:
   - `compilation.additional_dependencies`: Both `type: "source"` and `type: "project"` dependencies declared in the dependency's selected profile are ingested.
   - Transitive project dependencies declared within a dependency profile are discovered recursively.
   - Pre-compile command hooks (`pre_compile`) defined at the dependency's root and within its selected profile execute in topological order (deepest dependency first) prior to consuming project compilation.

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

#### Runtime Block (`profiles.<name>.runtime`)

| Subfield | Type | Default | Description |
|---|---|---|---|
| `heap_size` | Integer | `16777216` | Initial/maximum heap capacity in machine words. |
| `stack_size` | Integer | `1048576` | Stack capacity in machine words. |
| `arguments` | Array of Strings | `[]` | Default arguments supplied to VM execution. |
| `native_libraries` | Array of Strings | `[]` | Profile-specific shared libraries (`.dll`, `.so`, `.dylib`) to load before VM execution. |

### 4. Native Shared Libraries (`native_libraries`)

Root-level array of filesystem paths pointing to compiled C/C++ shared libraries (`.dll`, `.so`, `.dylib`). These libraries are automatically loaded and registered by `solix run` across all build profiles:

```json
{
  "native_libraries": [
    "lib/math_plugin.so",
    "lib/graphics.dll"
  ]
}
```
Relative paths are resolved relative to the directory containing `solix.json`. Additionally, any shared libraries placed in the project's `./lib/` directory or alongside the generated `.slxbin` are automatically discovered without requiring explicit manifest entries.

### 5. Lifecycle Command Hooks (`pre_compile`, `post_compile`)

Solix manifests support declarative pre-compile and post-compile lifecycle command hooks at both the root level and within individual profiles:

```json
{
  "pre_compile": [
    "cmake -B native/build -S native"
  ],
  "post_compile": [
    "echo \"Build finished!\""
  ],
  "profiles": {
    "release": {
      "pre_compile": [
        "python3 scripts/build_native.py",
        "cmake --build native/build --config Release"
      ],
      "post_compile": [
        "strip lib/libplugin.so"
      ]
    }
  }
}
```

#### Field Details:
| Field | Location | Type | Default | Description |
|---|---|---|---|---|
| `pre_compile` / `precompile` | Root or Profile | Array of Strings or String | `[]` | Shell commands to execute in `project_root` before bytecode compilation. |
| `post_compile` / `postcompile` | Root or Profile | Array of Strings or String | `[]` | Shell commands to execute in `project_root` after successful compilation and artifact staging. |

#### Execution Ordering & Semantics:
1. **Root `pre_compile`**: Executes first in order of declaration.
2. **Active Profile `pre_compile`**: Executes next in order of declaration.
3. **Compilation & Staging**: Compiler builds `.slx` bytecode and stages `lib/` and `resources/`.
4. **Active Profile `post_compile`**: Executes immediately after successful compilation.
5. **Root `post_compile`**: Executes last.
- **Fail-Fast Error Handling**: If any pre-compile or post-compile command exits with a non-zero code, the build process immediately terminates with exit code 1. Subsequent commands and compilation stages are skipped.


