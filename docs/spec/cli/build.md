# `solix build` — Project Manifest Build

The `build` subcommand reads a project's manifest file (`solix.json`), resolves declared dependencies, applies profile configurations (`debug`, `release`, `test`), and compiles the project into final artifacts.

---

## Synopsis

```bash
solix build [options]
```

---

## Options & Flags

| Option | Shorthand | Type | Default | Description |
|---|---|---|---|---|
| `--profile` | `-p` | String | `debug` | Build profile name defined in `solix.json` (e.g., `debug`, `release`, `test`). |
| `--manifest` | `-m` | Path | `solix.json` | Path to `solix.json` or path to the project root directory containing it. |

---

## Build Pipeline Lifecycle

When `solix build` is executed:

```mermaid
flowchart TD
    A["Locate solix.json"] --> B["Parse & Validate Profiles"]
    B --> C["Extract Profile Settings<br>(output_directory, exe_filename, logs)"]
    C --> D["Transitive Dependency Walking<br>(type: source, type: project)"]
    D --> E["SemVer Conflict Resolution & Cycle Handling"]
    E --> F["Pre-Compilation Source Ingestion"]
    F --> G["Invoke Compiler Pipeline"]
    G --> H["Write Output Artifact<br>(e.g. build/debug/out.slxbin)"]
```

1. **Manifest Discovery**:
   - Resolves `--manifest` (or current directory `solix.json`).
   - Determines project root path from the manifest location.
2. **Profile Validation**:
   - Ensures `profiles` exists and contains the requested profile object.
   - Extracts output paths (`output_directory`, `exe_filename`, `asm_filename`).
   - Relative paths are resolved against the project root.
3. **Pre-Compilation Dependency Resolution**:
   - **Transitive Project Traversal**: Recursively scans `type: "project"` dependencies declared in `dependencies` and active profile's `compilation.additional_dependencies`. Projects are resolved either via explicit `path` or discovered in the local `$SOLIX_HOME` registry.
   - **Circular Dependency Handling**: Cycles (e.g. A depends on B and B depends on A) are recognized and de-duplicated gracefully.
   - **Semantic Version Conflict Resolution**:
     | Version Discrepancy | Behavior | Output |
     |---|---|---|
     | Different Major | Fatal Error; build aborts before compilation | `Error: Incompatible major versions for dependency '...'` |
     | Same Major, Different Minor | Warning; automatically selects highest minor version | `Warning: Project '...' has multiple minor versions (...)` |
     | Same Major and Minor, Different Patch | Clean resolution; silently selects highest patch version | None |
   - **Source Ingestion**: All unique source files across the root and all selected dependent projects are validated and read into in-memory buffers.
4. **Compilation & Artifact Emission**:
   - Executes compiler pipeline with configured entry point and log levels.
   - Emits bytecode binary to the configured profile output directory.

---

## Exit Codes

| Code | Condition |
|---|---|
| `0` | Project built successfully; artifacts written to profile directory. |
| `1` | Missing manifest file, JSON parse error, invalid profile, unresolved dependencies, incompatible major SemVer collision, missing dependency sources, or compiler error. |

---

## Examples

### 1. Build Default Profile
```bash
solix build
# Builds 'debug' profile into build/debug/out.slxbin
```

### 2. Build Release Profile
```bash
solix build -p release
# Builds 'release' profile into build/release/out.slxbin
```

### 3. Build Project from External Directory
```bash
solix build -m ../other_project
# Locates ../other_project/solix.json and builds artifacts relative to that project
```
