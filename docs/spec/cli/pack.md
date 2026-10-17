# `solix pack` — Single-File Application Packaging (`.slxpac`)

The `pack` subcommand compiles a Solix project and packages all distribution artifacts—including compiled bytecode, bundled native dynamic libraries (`.so`, `.dll`, `.dylib`), and application resource assets—into a single `.slxpac` package archive.

---

## Synopsis

```bash
solix pack [target] [options]
```

---

## Positional Arguments

| Argument | Type | Default | Description |
|---|---|---|---|
| `target` | Path or Identifier | `.` (current directory) | Path to project directory or project manifest (`manifest.json` / `solix.json`). |

---

## Options & Flags

| Option | Shorthand | Type | Default | Description |
|---|---|---|---|---|
| `--profile` | `-p` | String | `release` | Build profile to use for packaging (e.g. `release`, `debug`). |
| `--output` | `-o` | Path | `<output_dir>/<name>.slxpac` | Explicit output destination file path for the generated `.slxpac` bundle. |
| `--help` | `-h` | Flag | — | Displays the command usage and available options. |

---

## Archive Structure (`.slxpac`)

The `.slxpac` format is a standard ZIP archive containing:

```text
<app_name>.slxpac
├── manifest.json            # Package metadata & runtime entry configuration
├── bytecode.slxbin          # Main compiled application bytecode
├── lib/                     # Platform-native shared libraries
│   ├── solixlib_native.so
│   └── <custom_plugin>.so
└── resources/               # Application assets, configs, and bundled data
    ├── config.json
    └── ...
```

### Internal `manifest.json`

The package includes an internal `manifest.json` metadata file generated during packaging:

```json
{
  "name": "my_application",
  "version": "1.0.0",
  "format_version": 1,
  "entry": "bytecode.slxbin",
  "resources_dir": "resources",
  "native_lib_dir": "lib",
  "profile": "release",
  "created_by": "solix pack"
}
```

---

## Packaging Logic & Asset Ingestion

1. **Compilation & Lifecycle Execution**: Invokes `build_project` under the chosen profile (defaulting to `release`), executing declared root and profile `pre_compile` hooks (e.g. building native C/C++ libraries with CMake), Solix bytecode compilation, transitive asset and native library staging, and `post_compile` hooks.
2. **Bytecode**: The output binary is packaged at the root as `bytecode.slxbin`.
3. **Transitive Native Shared Libraries**: Automatically scans the project's `lib/` directory, all transitive dependency projects' `lib/` directories across any depth, and the compiler output directory for shared library files (`.so` on Linux, `.dll` on Windows, `.dylib` on macOS), placing them into `lib/`.
4. **Transitive Resources**: Recursively collects and merges static assets from `resources/` across all transitive dependency projects and the root project into `resources/`. Root project assets override dependency assets on path collision.

---

## Examples

### Package Current Directory to Default Output
```bash
solix pack
```
Produces `build/release/<name>.slxpac`.

### Package with Custom Output Path and Profile
```bash
solix pack -p debug -o dist/my_app.slxpac
```

### Package Another Project Directory
```bash
solix pack path/to/project -o dist/app.slxpac
```
