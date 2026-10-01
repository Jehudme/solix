# Getting Started with Solix

Solix is a statically typed, object-oriented programming language with Automatic Reference Counting (ARC) memory management. This guide walks you through installing the toolchain, understanding the project layout, and writing your first program.

---

## Table of Contents

1. [Building from Source](#building-from-source)
2. [Project Structure](#project-structure)
3. [CLI Commands](#cli-commands)
4. [Hello, World!](#hello-world)
5. [Command-Line Arguments](#command-line-arguments)
6. [Exit Codes](#exit-codes)

---

## Building from Source

### Prerequisites

| Tool | Minimum Version |
|------|----------------|
| CMake | 3.20 |
| C++ compiler (GCC or Clang) | C++20 support |
| Git | Any recent version |

### Clone and Build

```bash
git clone https://github.com/yourorg/solix.git
cd solix
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

After a successful build, the `solix` binary is located at `build/solix`.

### (Optional) Install System-Wide

```bash
sudo cmake --install .
```

This installs the `solix` binary to `/usr/local/bin`.

---

## Project Structure

```
solix/
├── language/                  # Compiler and runtime (C++ source)
│   ├── include/               # Public headers (compilation, runtime)
│   └── src/
│       ├── processes/         # Lexer, Parser, Binder, Assembler
│       └── utilities/         # AST nodes, opcodes, diagnostics
├── launcher/                  # CLI front-end
├── tests/                     # Unit and integration tests (Catch2)
└── docs/                      # Documentation (this directory)
```

A Solix source file uses the `.slx` extension. Each file belongs to exactly one **package**, declared at the top of the file:

```solix
package com.example.myapp;
```

---

## CLI Commands

The `solix` executable exposes two primary subcommands.

### `solix compile`

Compiles one or more `.slx` source files into a bytecode binary.

```
solix compile [options] <source-files...>
```

| Option | Description |
|--------|-------------|
| `-o <file>` | Output file path for the compiled binary (default: `out.slxb`) |
| `--asm <file>` | Emit human-readable assembly alongside the binary |
| `--lib <dir>` | Additional library directory to include on the search path |

**Example:**

```bash
solix compile -o hello.slxb hello.slx
```

### `solix run`

Compiles and immediately executes source files in a single step.

```
solix run [options] <source-files...> [-- <program-args...>]
```

| Option | Description |
|--------|-------------|
| `--lib <dir>` | Additional library directory |
| `--` | Separator; everything after this is passed to the Solix program as `args` |

**Example:**

```bash
solix run main.slx -- Alice 42
```

---

## Hello, World!

Every Solix program defines an execution entry point. The following signatures are supported:
- `static int32 main()` — directly returns an integer exit code.
- `static void main(char[][] args)` — receives command-line arguments.
- `static int32 main(char[][] args)` — receives command-line arguments and returns an integer exit code.

**`hello.slx`**

```solix
package com.example;

import solix.systems.Console;

static void main(char[][] args) {
    Console.println("Hello, World!");
}
```

Run it:

```bash
solix run hello.slx
```

Output:

```
Hello, World!
```

### What is `char[][]`?

In Solix there are no built-in string literals that map directly to a `String` object at the `main` boundary. Command-line arguments are passed as an **array of character arrays** — `char[][]` — where each `char[]` is one argument string.

```solix
import solix.systems.Console;
import solix.core.String;

static void main(char[][] args) {
    Console.print("Argument count: ");
    Console.println(args.length);

    for (int32 i = 0; i < args.length; i++) {
        String arg = new String(args[i]);
        Console.println(arg);
    }
}
```

---

## Command-Line Arguments

| Index | Content |
|-------|---------|
| `args[0]` | First user-supplied argument (the program name is **not** included) |
| `args[1]` | Second argument |
| … | … |
| `args[args.length - 1]` | Last argument |

Each element is a `char[]` which can be wrapped in a `String` for convenient manipulation:

```solix
import solix.core.String;
import solix.systems.Console;

static void main(char[][] args) {
    if (args.length == 0) {
        Console.println("No arguments provided.");
        return;
    }

    String name = new String(args[0]);
    Console.print("Hello, ");
    Console.print(name);
    Console.println("!");
}
```

Run:

```bash
solix run greet.slx -- Alice
# Hello, Alice!
```

---

## Exit Codes

Solix programs exit with a code that reflects their outcome.

| Code | Meaning |
|------|---------|
| `0` | Program completed successfully |
| `1` | Unhandled exception terminated the program |
| `2` | Compilation failed (syntax or type error) |

The runtime prints the unhandled exception's `to_string()` to `stderr` before exiting with code `1`.

To exit deliberately with a specific code, define `main` with return type `int32`:

```solix
static int32 main() {
    return 42;
}
```
