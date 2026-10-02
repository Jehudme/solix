# `solix compile` — Source Code Compilation

The `compile` subcommand drives the Solix front-end pipeline (Lexer, Parser, Binder, and Assembler) to translate one or more `.slx` source files into executable bytecode binaries (`.slxb`) or text assembly listings.

---

## Synopsis

```bash
solix compile <files>... [options]
```

---

## Arguments

| Argument | Type | Required | Description |
|---|---|---|---|
| `<files>...` | Positional Path(s) | **Yes** | One or more paths to `.slx` source files to compile. Must exist on disk. |

---

## Options & Flags

### Output Artifacts

| Option | Shorthand | Type | Default | Description |
|---|---|---|---|---|
| `--output` | `-o` | Path | `out.slxb` | Target path for the generated binary bytecode file. Parent directories are created automatically if they do not exist. |
| `--asm` | `-a` | Path | `""` (Disabled) | Optional path to emit human-readable disassembly text. |
| `--entry` | `-e` | String | `main` | Name of the top-level or static method serving as the program entry point. |

### Diagnostic & Logging Options

| Option | Type | Allowed Values | Default | Description |
|---|---|---|---|---|
| `--log-level` | String | `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERR`, `CRITICAL`, `OFF` | `INFO` | Minimum severity level for compiler log output. |
| `--flush-level` | String | `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERR`, `CRITICAL`, `OFF` | `ERR` | Threshold level triggering an immediate flush of log buffers. |
| `--sink-type` | String | `STDOUT`, `STDERR`, `BASIC_FILE`, `CONSOLE_AND_FILE` | `STDOUT` | Destination sink for compiler logs. |
| `--log-pattern` | String | Format String | (Default spdlog pattern) | Custom log formatting pattern. |
| `--log-file` | Path | File Path | `""` | File path for log writes when `sink-type` is `BASIC_FILE` or `CONSOLE_AND_FILE`. |
| `--flush-every` | Integer | Seconds | `0` | Periodic log flush interval in seconds (0 to disable periodic flush). |
| `--multithreaded` | Flag | Boolean | `false` | Enables multithreaded parallel lexing and parsing. |

---

## Exit Codes

| Code | Condition |
|---|---|
| `0` | Compilation succeeded; binary bytecode written to output destination. |
| `1` | Compilation failed due to missing files, syntax errors, or semantic binding errors. Any partially written output file is automatically removed. |

---

## Examples

### 1. Basic Compilation to Default Target
```bash
solix compile main.slx
# Produces out.slxb in current working directory
```

### 2. Custom Output Path and Disassembly
```bash
solix compile src/app.slx -o bin/app.slxb -a bin/app.s
# Compiles src/app.slx, writes bytecode to bin/app.slxb, and outputs assembly to bin/app.s
```

### 3. Multi-File Compilation with Custom Entry Point
```bash
solix compile math.slx main.slx -e start -o bin/math_app.slxb
# Compiles both files with 'start' as the entry point method
```

### 4. Detailed Diagnostic Trace to File
```bash
solix compile main.slx --log-level TRACE --sink-type BASIC_FILE --log-file compiler.log
```
