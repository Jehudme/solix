# `Console`

## 1. Class Overview

`solix.systems.Console` provides static native bridge methods for standard input, output, formatting, and console interactions. Output operations are implemented as native C++ hooks into `stdout` and `stderr` for high-performance terminal printing.

- **Package**: `solix.systems`
- **Import**: `import solix.systems.Console;` or `import solix.systems.*;`

---

## 2. Enumerations

### `ConsoleColor`

```solix
public enum ConsoleColor {
    BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, RESET
}
```

---

## 3. Method Reference

### Basic Printing

| Method Signature | Description |
|------------------|-------------|
| `public static native void println()` | Prints a newline character (`\n`) to `stdout`. |
| `public static native void print(bool val)` | Prints a boolean value (`true` or `false`). |
| `public static native void print(char val)` | Prints a single character. |
| `public static native void print(int8 val)` | Prints a signed 8-bit integer. |
| `public static native void print(int16 val)` | Prints a signed 16-bit integer. |
| `public static native void print(int32 val)` | Prints a signed 32-bit integer. |
| `public static native void print(int64 val)` | Prints a signed 64-bit integer. |
| `public static native void print(uint8 val)` | Prints an unsigned 8-bit integer. |
| `public static native void print(uint16 val)` | Prints an unsigned 16-bit integer. |
| `public static native void print(uint32 val)` | Prints an unsigned 32-bit integer. |
| `public static native void print(uint64 val)` | Prints an unsigned 64-bit integer. |
| `public static native void print(float32 val)` | Prints a 32-bit float. |
| `public static native void print(float64 val)` | Prints a 64-bit float. |
| `public static native void print(char[] val)` | Prints a character array / string literal. |
| `public static native void print(String val)` | Prints a `solix.core.String` instance. |

### Line Printing (`println`)

All `print(...)` overloads have a matching `println(...)` counterpart that automatically appends a terminating newline (`\n`):

- `public static native void println(bool val)`
- `public static native void println(char val)`
- `public static native void println(int32 val)`
- `public static native void println(int64 val)`
- `public static native void println(float64 val)`
- `public static native void println(char[] val)`
- `public static native void println(String val)`

### Array Printing

`Console` provides overloads to print entire primitive arrays formatted with bracket notation:

- `public static native void print(int32[] val)`
- `public static native void println(int32[] val)`
- `public static native void print(float64[] val)`
- `public static native void println(float64[] val)`

---

## 4. Code Examples

```solix
package solix.example;

import solix.systems.Console;

public class Main {
    public static void main(char[][] args) {
        Console.println("Welcome to Solix!");

        int32 score = 100;
        float64 ratio = 0.75;
        Console.println(score);
        Console.println(ratio);

        int32[] items = new int32[3];
        items[0] = 1; items[1] = 2; items[2] = 3;
        Console.println(items);
    }
}
```
