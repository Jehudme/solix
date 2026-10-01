# `String` — API Reference

**Package:** `solix.core`  
**Import:** `import solix.core.String;`

---

## Overview

`String` is the fundamental text type in Solix. It is a heap-allocated, ARC-managed class that wraps an immutable-length `char[]` buffer. Once constructed, the character buffer's length cannot change; operations such as `concat` or `substring` always return a **new** `String` instance.

> [!NOTE]
> `String` objects are not thread-safe. Concurrent reads are safe; concurrent writes require external synchronization.

---

## Constructors

| Signature | Description |
|-----------|-------------|
| `String()` | Creates an empty string (length 0). |
| `String(char[] source_characters)` | Copies characters from a `char[]` array. Passing `null` produces an empty string. |
| `String(String source_string)` | Copy constructor — deep-copies another `String`. Passing `null` produces an empty string. |

---

## Method Reference

### Capacity & Size

| Signature | Returns | Description |
|-----------|---------|-------------|
| `size()` | `int32` | Returns the number of characters in the string. |
| `length()` | `int32` | Alias for `size()`. |
| `is_empty()` | `bool` | Returns `true` if `length() == 0`. |
| `raw()` | `char[]` | Returns a copy of the underlying character buffer. |

---

### Element Access

| Signature | Returns | Description | Throws |
|-----------|---------|-------------|--------|
| `char_at(int32 index)` | `char` | Returns the character at the given zero-based index. | `IndexOutOfBoundsException` if index < 0 or ≥ length |
| `set_char_at(int32 index, char ch)` | `void` | Replaces the character at the given index in-place. | `IndexOutOfBoundsException` if index is out of range |

---

### Comparison

| Signature | Returns | Description |
|-----------|---------|-------------|
| `equals(String other)` | `bool` | Character-by-character comparison. Returns `false` if `other` is `null`. |
| `equals(char[] other)` | `bool` | Compares this string's contents against a raw `char[]`. |
| `compare_to(String other)` | `int32` | Lexicographic comparison. Returns negative if this < other, 0 if equal, positive if this > other. |
| `compare_to_ignore_case(String other)` | `int32` | Case-insensitive lexicographic comparison. |
| `starts_with(String prefix)` | `bool` | Returns `true` if this string begins with the given prefix. |
| `ends_with(String suffix)` | `bool` | Returns `true` if this string ends with the given suffix. |
| `hash_code()` | `int32` | Polynomial rolling hash over the character buffer (multiplier 31). |

---

### Search

| Signature | Returns | Description |
|-----------|---------|-------------|
| `index_of(char ch)` | `int32` | First index of the character, or -1 if not found. |
| `index_of(String substring)` | `int32` | First index where `substring` starts, or -1. |
| `last_index_of(char ch)` | `int32` | Last index of the character, or -1. |
| `contains(String substring)` | `bool` | `true` if the substring appears anywhere in this string. |
| `contains(char ch)` | `bool` | `true` if the character appears anywhere in this string. |

---

### Transformation

| Signature | Returns | Description |
|-----------|---------|-------------|
| `concat(String other)` | `String` | Returns a new `String` that is this + other. |
| `concat(char[] chars)` | `String` | Returns a new `String` that is this + chars. |
| `substring(int32 start, int32 end)` | `String` | Returns characters in range [start, end). | 
| `replace(char old_char, char new_char)` | `String` | Returns a new string with all occurrences of `old_char` replaced. |
| `replace(String old_sub, String new_sub)` | `String` | Returns a new string with all occurrences of `old_sub` replaced. |
| `to_lower_case()` | `String` | Returns a new string with all ASCII letters lowercased. |
| `to_upper_case()` | `String` | Returns a new string with all ASCII letters uppercased. |
| `trim()` | `String` | Returns a new string with leading and trailing whitespace (space, tab, newline, carriage return) removed. |

---

### Conversion (Parse)

| Signature | Returns | Description |
|-----------|---------|-------------|
| `to_int32()` | `int32` | Parses the string as a signed 32-bit integer. Returns 0 on empty string; stops at first non-digit. |
| `to_int64()` | `int64` | Parses the string as a signed 64-bit integer via native utilities. |
| `to_float64()` | `float64` | Parses the string as a 64-bit floating-point value via native utilities. |

---

### Static Factory Methods

| Signature | Returns | Description |
|-----------|---------|-------------|
| `String.from_int32(int32 value)` | `String` | Converts an `int32` to its decimal string representation. |
| `String.from_int64(int64 value)` | `String` | Converts an `int64` to its decimal string representation. |
| `String.from_float64(float64 value)` | `String` | Converts a `float64` using full precision. |
| `String.from_float64(float64 value, int32 precision)` | `String` | Converts a `float64` with the specified number of decimal digits. |
| `String.from_bool(bool value)` | `String` | Returns `"true"` or `"false"`. |

---

### Operator Overloads

| Operator | Signature | Description |
|----------|-----------|-------------|
| `=` | `operator=(String other)` | Deep-copies content from `other`. Null-safe (produces empty string on null). |
| `=` | `operator=(char[] chars)` | Copies content from a `char[]`. |
| `+` | `operator+(String other)` | Equivalent to `concat(String)`. Returns a new String. |
| `+` | `operator+(char[] other)` | Equivalent to `concat(char[])`. Returns a new String. |

---

## Code Examples

```solix
import solix.core.String;
import solix.systems.Console;

public void main(char[][] args) {
    // Construction
    String hello = new String("Hello");
    String world = new String(", World!");

    // Concatenation
    String greeting = hello + world;
    Console.println(greeting);             // Hello, World!

    // Inspection
    Console.println(greeting.length());    // 13
    Console.println(greeting.char_at(0));  // (char)72 → H

    // Search
    Console.println(greeting.contains(new String("World")));  // true
    Console.println(greeting.index_of((char)44));              // 5 (comma)

    // Transformation
    Console.println(greeting.to_upper_case());   // HELLO, WORLD!
    Console.println(greeting.trim());            // (unchanged — no whitespace)
    Console.println(greeting.substring(0, 5));   // Hello

    // Parsing
    String numStr = new String("42");
    int32 n = numStr.to_int32();
    Console.println(n + 1);                      // 43

    // Formatting
    Console.println(String.from_float64(3.14159, 2));  // 3.14
}
```
