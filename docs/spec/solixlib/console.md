# Standard Library Specification: `solix.system.Console`

The `solix.system.Console` class provides standard input, standard output, standard error, colorized diagnostics, and terminal control primitives for the Solix programming language.

---

## 1. Class Declaration

```solix
package solix.system;

import solix.exceptions.*;

public class Console { ... }
```

---

## 2. API Reference

### 2.1 Standard Output (`print`, `println`)
Prints primitive values and character arrays to standard output (`std::cout`).

| Method | Description |
|---|---|
| `static void print(char[] val)` | Prints character array to stdout without trailing newline. |
| `static void print(int32 val)` | Prints 32-bit signed integer to stdout. |
| `static void print(int64 val)` | Prints 64-bit signed integer to stdout. |
| `static void print(float64 val)` | Prints 64-bit float to stdout. |
| `static void print(bool val)` | Prints `true` or `false` to stdout. |
| `static void print(char val)` | Prints single character to stdout. |
| `static void println(char[] val)` | Prints character array followed by newline. |
| `static void println(int32 val)` | Prints 32-bit integer followed by newline. |
| `static void println(int64 val)` | Prints 64-bit integer followed by newline. |
| `static void println(float64 val)` | Prints float followed by newline. |
| `static void println(bool val)` | Prints boolean followed by newline. |
| `static void println(char val)` | Prints single character followed by newline. |
| `static void println()` | Prints newline to stdout. |

### 2.2 Colorized Diagnostics
Emits formatted diagnostic text wrapped in standard ANSI escape codes.

| Method | Color | Stream | Description |
|---|---|---|---|
| `static void error(char[] val)` / `error(int32 val)` | Red (`\033[31m`) | `stderr` | Prints fatal/severe error diagnostic. |
| `static void warning(char[] val)` / `warning(int32 val)` | Yellow (`\033[33m`) | `stdout` | Prints warning diagnostic. |
| `static void info(char[] val)` / `info(int32 val)` | Cyan (`\033[36m`) | `stdout` | Prints informational status message. |
| `static void success(char[] val)` / `success(int32 val)` | Green (`\033[32m`) | `stdout` | Prints success message. |

### 2.3 Standard Input (`input_*`)
Reads and parses input from standard input (`std::cin`).

| Method | Return Type | Description |
|---|---|---|
| `static char[] input_chars()` | `char[]` | Reads entire line of input up to `\n` or `\r\n`. |
| `static char input_char()` | `char` | Reads a single character from stdin. |
| `static int32 input_int()` | `int32` | Reads line and parses integer. Throws `FormatException` on parse failure. |
| `static float64 input_double()` | `float64` | Reads line and parses float. Throws `FormatException` on parse failure. |
| `static bool input_bool()` | `bool` | Reads line and parses `"true"`, `"false"`, `"1"`, or `"0"`. Throws `FormatException` on parse failure. |

### 2.4 Terminal Window & Cursor Control

| Method | Description |
|---|---|
| `static void clear()` | Clears the terminal screen via ANSI code `\033[2J\033[H`. |
| `static void flush()` | Flushes both standard output and standard error streams. |
| `static void set_color(int32 ansi_code)` | Sets current terminal text color using ANSI SGR code. |
| `static void reset_color()` | Resets terminal colors and attributes via `\033[0m`. |
| `static void set_cursor_position(int32 row, int32 col)` | Sets terminal cursor to specified row and column. |
| `static void set_title(char[] title)` | Sets terminal window title using OSC sequence. |

---

## 3. Platform Interoperability

- **POSIX (Linux / macOS)**: Uses standard ANSI terminal escape sequences and POSIX I/O streams.
- **Windows**: Automatically initializes Windows Virtual Terminal Processing (`ENABLE_VIRTUAL_TERMINAL_PROCESSING`) via `SetConsoleMode` on standard output and error handles, enabling full ANSI color and cursor sequence support across Windows 10 and 11 terminals.
