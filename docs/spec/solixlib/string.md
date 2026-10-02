# Solix Standard Library: String & StringBuilder (`solix.core`)

## Overview

The `solix.core` string module provides high-performance text manipulation facilities for Solix programs. It consists of:
1. `IStringable`: Universal interface declaring `to_string() -> String`.
2. `String`: Immutable UTF-8 sequence object backed by native primitives and `char[]`.
3. `StringBuilder`: Dynamically resizable heap buffer for sequential text construction.

---

## 1. Interface: `solix.core.IStringable`

```solix
package solix.core;

interface IStringable {
    to_string() -> String;
}
```

Any class implementing `IStringable` can be passed seamlessly to standard input/output subsystems, formatters, and collection stringifiers.

---

## 2. Class: `solix.core.String`

`String` instances are immutable.

### Constructors
- `new String()`: Constructs an empty string.
- `new String(char[] characters)`: Constructs a string by copying characters from the provided array.

### Query Methods
- `length() -> int32`: Returns the number of characters.
- `is_empty() -> bool`: Returns `true` if `length() == 0`.
- `char_at(int32 index) -> char`: Returns character at zero-based index. Throws `IndexOutOfBoundsException` if out of bounds.
- `to_char_array() -> char[]`: Returns a copy of the underlying characters.
- `to_string() -> String`: Returns `this`.

### Comparison & Search
- `equals(String other) -> bool`: Returns `true` if characters match exactly.
- `index_of(char ch) -> int32`: Returns zero-based index of first occurrence, or `-1` if not found.
- `index_of(char ch, int32 from_index) -> int32`: Searches starting from `from_index`.
- `contains(char ch) -> bool`: Returns `true` if `index_of(ch) >= 0`.
- `starts_with(String prefix) -> bool`: Returns `true` if the string begins with `prefix`.
- `ends_with(String suffix) -> bool`: Returns `true` if the string ends with `suffix`.

### Substring & Transformations
- `substring(int32 start, int32 end) -> String`: Returns substring `[start, end)`. Throws `IndexOutOfBoundsException` on invalid indices.
- `substring(int32 start) -> String`: Returns substring from `start` to `length()`.
- `concat(String other) -> String`: Returns newly allocated string combining both.
- `to_upper_case() -> String`: Returns uppercase version via native ASCII/UTF-8 transformation.
- `to_lower_case() -> String`: Returns lowercase version.
- `trim() -> String`: Strips leading and trailing whitespace characters.
- `replace(char target, char replacement) -> String`: Returns string with all instances of `target` replaced.
- `split(char delimiter) -> String[]`: Splits string into an array of substrings on `delimiter`.

### Static Helpers & Conversions
- `static join(String delimiter, String[] elements) -> String`: Joins array of strings with delimiter.
- `static value_of(int32 val) -> String`: Converts integer to decimal string.
- `static value_of(int64 val) -> String`: Converts 64-bit integer to decimal string.
- `static value_of(float64 val) -> String`: Converts floating-point value to string.
- `static value_of(bool val) -> String`: Returns `"true"` or `"false"`.
- `static value_of(char val) -> String`: Converts single character to `String`.
- `static parse_int(String s) -> int32`: Parses decimal integer. Throws `FormatException` on error.
- `static parse_double(String s) -> float64`: Parses floating-point number. Throws `FormatException` on error.
- `static parse_bool(String s) -> bool`: Parses boolean (`"true"` / `"false"`). Throws `FormatException` on error.

---

## 3. Class: `solix.core.StringBuilder`

A mutable character buffer designed for efficient concatenation.

### Constructors
- `new StringBuilder()`: Initializes buffer with default capacity (16).
- `new StringBuilder(int32 initial_capacity)`: Initializes with specified initial capacity.
- `new StringBuilder(String initial_text)`: Initializes with content of `initial_text`.

### Methods
- `length() -> int32`: Current number of characters stored.
- `capacity() -> int32`: Current allocated capacity.
- `append(String s) -> StringBuilder`: Appends string content; returns `this` for chaining.
- `append(char ch) -> StringBuilder`: Appends single character.
- `append(int32 val) -> StringBuilder`: Appends decimal integer.
- `append(float64 val) -> StringBuilder`: Appends floating-point value.
- `append(bool val) -> StringBuilder`: Appends `"true"` or `"false"`.
- `append_line(String s) -> StringBuilder`: Appends string followed by newline `\n`.
- `insert(int32 index, String s) -> StringBuilder`: Inserts string at specified index.
- `insert(int32 index, char ch) -> StringBuilder`: Inserts character at index.
- `delete(int32 start, int32 end) -> StringBuilder`: Deletes range `[start, end)`.
- `delete_char_at(int32 index) -> StringBuilder`: Deletes character at index.
- `reverse() -> StringBuilder`: Inverts buffer in-place.
- `clear() -> StringBuilder`: Clears buffer (resets length to 0).
- `to_string() -> String`: Produces immutable `String` representation of current buffer.
