# `solix run` — Bytecode Execution

The `run` subcommand initializes the Solix Virtual Machine (VM) and executes compiled bytecode (`.slxb` or `.slxbin`).

---

## Synopsis

```bash
solix run <file> [options] [args...]
```

---

## Arguments

| Argument | Type | Required | Description |
|---|---|---|---|
| `<file>` | Positional Path | **Yes** | Path to the compiled Solix bytecode binary file. Must exist and be non-empty. |
| `[args...]` | Strings | No | Optional arguments passed directly to the executing Solix program. |

---

## Options & Flags

| Option | Shorthand | Type | Default | Description |
|---|---|---|---|---|
| `--stack` | `-s` | Integer | `1048576` (1 MWord) | Call stack capacity in 64-bit machine words. |
| `--heap` | `-p` | Integer | `16777216` (16 MWords) | Flat object heap capacity in 64-bit machine words. |

---

## Program Argument Passing

Arguments specified after the bytecode file are passed to the program's entry method.

In the VM runtime:
- Strings are allocated on the flat heap.
- The entry method receives an array containing pointers to each argument string.

```bash
solix run app.slxb foo bar 123
```

---

## Exit Codes & Fault Handling

| Exit Code | Condition |
|---|---|
| `0` | Bytecode executed successfully and the entry method returned 0. |
| `1` | VM execution aborted due to missing file, empty file, unhandled exception (e.g. `NullPointer`, division by zero), stack overflow, or heap exhaustion. |
| `N` (`N > 0`) | The entry method returned non-zero integer `N`, which is propagated directly to the host shell. |

---

## Examples

### 1. Execute Compiled Application
```bash
solix run out.slxb
```

### 2. Pass Arguments to Program
```bash
solix run bin/processor.slxb input.dat --verbose
```

### 3. Run with Expanded Memory Limits
```bash
solix run memory_heavy.slxb -s 4194304 -p 67108864
# Executes with 4 MWords stack and 64 MWords heap
```
