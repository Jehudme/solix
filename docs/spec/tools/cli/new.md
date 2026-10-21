# `solix create` (alias: `solix new`) — Project Scaffolding

The `create` subcommand (also available via backward-compatible alias `new`) creates and initializes a new Solix project structure with pre-configured build profiles (`debug`, `release`, `test`), a starter source file, standard library dependency, and a complete `manifest.json` manifest.

---

## Synopsis

```bash
solix create <path> [options]
solix new <path> [options]
```

---

## Arguments

| Argument | Type | Required | Description |
|---|---|---|---|
| `<path>` | Positional Path | **Yes** | Destination folder for the project. Created if it does not exist. |

---

## Options & Flags

| Option | Shorthand | Type | Default | Description |
|---|---|---|---|---|
| `--name` | `-n` | String | (Target folder basename) | Package / project name. |
| `--version` | `-v` | String | `0.1.0` | Initial semantic version. |
| `--author` | `-a` | String | `""` | Author name or email. |
| `--description` | `-d` | String | `""` | Human-readable project description. |
| `--license` | `-l` | String | `""` | License identifier (e.g. `MIT`, `Apache-2.0`). |
| `--tag` | `-t` | String | `[]` | Project tag/keyword (can be specified multiple times). |
| `--type` | `-T` | String | `app` | Project type: `app` (executable application), `library` (reusable library), or `plugin` (native extension module). |
| `--entry` | `-e` | String | `main` | Default entry point method name in starter source. |
| `--template` | | Path | `""` | Path to a custom template folder to copy from disk. |
| `--force` | `-f` | Flag | `false` | Proceed and overwrite if target directory exists and is not empty. |

---

## Scaffolding Modes

### 1. In-Memory Self-Contained Scaffolding (Default)

When `--template` is omitted, the CLI scaffolds an isolated, self-contained project according to `--type`:

#### Application (`-T app`, default)
```
<project_root>/
├── manifest.json          # Manifest with "type": "app", profiles & dependencies
└── src/
    └── main.slx           # Starter source printing Hello, World! with main()
```

#### Library (`-T library`)
```
<project_root>/
├── manifest.json          # Manifest with "type": "library", profiles & dependencies
└── src/
    └── lib.slx            # Starter library module
```

#### Plugin (`-T plugin`)
```
<project_root>/
├── manifest.json          # Manifest with "type": "plugin", profiles & dependencies
└── src/
    └── plugin.slx         # Starter plugin module
```

### 2. Custom Template Directory Mode

When `--template <path>` is supplied:
1. Validates that the template folder exists.
2. Recursively copies all directories and files into the destination.
3. If the template contains a `solix.json`, automatically injects the specified project name, version, author, description, license, tags, and entry point.

---

## Collision Protection Rules

1. If the destination path exists as a regular file, the command aborts with an error.
2. If the destination directory exists and is **not empty**, the command aborts unless `--force` (`-f`) is specified.
3. If `--force` is set, existing files are preserved or overwritten without deleting unspecified files.

---

## Exit Codes

| Code | Condition |
|---|---|
| `0` | Project successfully initialized. |
| `1` | Invalid destination, non-empty directory collision, non-existent template path, or filesystem error. |

---

## Examples

### 1. Create Default Project
```bash
solix new my_app
# Generates my_app/solix.json and my_app/src/main.slx
```

### 2. Create Library with Custom Metadata
```bash
solix new math_utils \
  -n "solix-math" \
  -v "1.0.0" \
  -a "Solix Team <dev@solix.org>" \
  -d "Optimized math primitives" \
  -l "MIT" \
  -t math -t fast \
  -e run
```

### 3. Overwrite Existing Non-Empty Folder
```bash
solix new ./existing_dir --force
```

### 4. Scaffold from Custom Template
```bash
solix new api_service --template /path/to/my_service_template
```
