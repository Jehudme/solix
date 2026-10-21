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
| `target` | Path or Identifier | `.` (current directory) | Path to project directory, project manifest (`manifest.json` / `solix.json`), or installed package identifier (`<name>@<version>` or `<name>`). |

---

## Options & Flags

| Option | Shorthand | Type | Default | Description |
|---|---|---|---|---|
| `--profile` | `-p` | String | `release` | Build profile to use for packaging uninstalled projects (e.g. `release`, `debug`). |
| `--output` | `-o` | Path | `<output_dir>/<name>.slxpac` | Explicit output destination file path for the generated `.slxpac` bundle. |
| `--help` | `-h` | Flag | — | Displays the command usage and available options. |

---

## Archive Structure (`.slxpac`)

The `.slxpac` format is a standard ZIP archive containing:

```text
<app_name>.slxpac
├── manifest.json            # Reduced package metadata & runtime entry configuration
├── bytecode.slxbin          # Main compiled application bytecode (for apps)
├── lib/                     # Platform-native shared libraries
│   ├── solixlib_native.so
│   └── <custom_plugin>.so
└── resources/               # Application assets, configs, and bundled data
    ├── config.json
    └── ...
```

### Internal `manifest.json`

The package includes an internal `manifest.json` metadata file generated during packaging that can be inspected directly with `solix details <bundle.slxpac>`:

```json
{
  "name": "my_application",
  "project": "my_application",
  "version": "1.0.0",
  "type": "app",
  "installed": true,
  "entry": "bytecode.slxbin"
}
```

---

## Packaging Logic & Asset Ingestion

1. **Clean Staging Architecture**: Staging reuses the `PackageManager` clean distribution pipeline (the same foundation as `solix archive`), building the target project (or taking the installed application assets) into a temporary staging tree and archiving it into a single `.slxpac` container.
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

### Package an Already Installed Application
```bash
solix pack my_app@1.0.0 -o dist/my_app.slxpac
```
