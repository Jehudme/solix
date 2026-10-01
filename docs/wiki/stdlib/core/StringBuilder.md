# `StringBuilder` — API Reference

**Package:** `solix.core`  
**Import:** `import solix.core.StringBuilder;`

---

## Overview

`StringBuilder` is a mutable, dynamically resizing character buffer. Unlike `String`, which creates new objects for every modification, `StringBuilder` mutates its internal buffer in-place, making it the preferred choice for building strings through repeated appends or insertions.

The internal buffer starts at a minimum capacity of 16 characters and grows by doubling whenever the capacity is exceeded.

> [!TIP]
> Use `StringBuilder` whenever you need to concatenate strings in a loop. Repeated `String +` operations allocate a new `String` on every iteration; `StringBuilder.append` amortizes allocation cost.

---

## Constructors

| Signature | Description |
|-----------|-------------|
| `StringBuilder()` | Creates an empty builder with initial capacity 16. |
| `StringBuilder(int32 initial_capacity)` | Creates an empty builder with the specified initial capacity (minimum 16). | Throws `InvalidArgumentException` if capacity < 0. |
| `StringBuilder(String initial_string)` | Creates a builder pre-filled with the content of `initial_string`. `null` produces an empty builder. |
| `StringBuilder(char[] initial_characters)` | Creates a builder pre-filled with the given characters. `null` produces an empty builder. |

---

## Method Reference

### Capacity & Size

| Signature | Returns | Description |
|-----------|---------|-------------|
| `length()` | `int32` | Number of characters currently in the buffer. |
| `size()` | `int32` | Alias for `length()`. |
| `capacity()` | `int32` | Current allocated capacity of the internal buffer. |
| `is_empty()` | `bool` | `true` if length is 0. |
| `clear()` | `void` | Resets the length to 0 without reallocating. |

---

### Append Methods

All `append` methods return `this`, enabling method chaining.

| Signature | Returns | Description |
|-----------|---------|-------------|
| `append(char ch)` | `StringBuilder` | Appends a single character. |
| `append(String text)` | `StringBuilder` | Appends the content of a `String`. |
| `append(char[] chars)` | `StringBuilder` | Appends a raw character array. |
| `append(bool value)` | `StringBuilder` | Appends `"true"` or `"false"`. |
| `append(int8 value)` | `StringBuilder` | Appends the decimal representation. |
| `append(int16 value)` | `StringBuilder` | Appends the decimal representation. |
| `append(int32 value)` | `StringBuilder` | Appends the decimal representation. |
| `append(int64 value)` | `StringBuilder` | Appends the decimal representation. |
| `append(uint8 value)` | `StringBuilder` | Appends the decimal representation. |
| `append(uint16 value)` | `StringBuilder` | Appends the decimal representation. |
| `append(uint32 value)` | `StringBuilder` | Appends the decimal representation. |
| `append(uint64 value)` | `StringBuilder` | Appends the decimal representation. |
| `append(float32 value)` | `StringBuilder` | Appends the floating-point representation. |
| `append(float64 value)` | `StringBuilder` | Appends the floating-point representation. |
| `append_line()` | `StringBuilder` | Appends a newline character (`'\n'`). |
| `append_line(String text)` | `StringBuilder` | Appends text followed by a newline. |
| `append_line(char[] chars)` | `StringBuilder` | Appends a char[] followed by a newline. |

---

### Insertion Methods

| Signature | Returns | Description | Throws |
|-----------|---------|-------------|--------|
| `insert(int32 offset, char ch)` | `StringBuilder` | Inserts a character at the given offset. | `IndexOutOfBoundsException` if offset is out of range |
| `insert(int32 offset, String text)` | `StringBuilder` | Inserts a `String` at the given offset. | `IndexOutOfBoundsException` |
| `insert(int32 offset, char[] chars)` | `StringBuilder` | Inserts a `char[]` at the given offset. | `IndexOutOfBoundsException` |

---

### Deletion Methods

| Signature | Returns | Description | Throws |
|-----------|---------|-------------|--------|
| `delete_char_at(int32 index)` | `StringBuilder` | Removes the character at the specified index. | `IndexOutOfBoundsException` |
| `delete_range(int32 start, int32 end)` | `StringBuilder` | Removes characters in the range [start, end). | `IndexOutOfBoundsException` if bounds are invalid |

---

### Transformation Methods

| Signature | Returns | Description | Throws |
|-----------|---------|-------------|--------|
| `reverse()` | `StringBuilder` | Reverses the character sequence in-place. | — |
| `replace(int32 start, int32 end, String replacement)` | `StringBuilder` | Deletes [start, end) and inserts `replacement` at `start`. | `IndexOutOfBoundsException` |

---

### Conversion Methods

| Signature | Returns | Description | Throws |
|-----------|---------|-------------|--------|
| `to_string()` | `String` | Returns a new `String` containing the current buffer contents. | — |
| `to_char_array()` | `char[]` | Returns a copy of the current buffer as a `char[]`. | — |
| `substring(int32 start, int32 end)` | `String` | Returns a new `String` for the range [start, end). | `IndexOutOfBoundsException` |

---

## Code Examples

### Basic String Building

```solix
import solix.core.StringBuilder;
import solix.core.String;
import solix.systems.Console;

public void main(char[][] args) {
    StringBuilder sb = new StringBuilder();
    sb.append("Hello")
      .append(", ")
      .append("World")
      .append("!")
      .append_line();

    Console.print(sb.to_string());   // Hello, World!\n
}
```

### Building in a Loop

```solix
import solix.core.StringBuilder;
import solix.core.String;
import solix.systems.Console;

public String join_numbers(int32[] nums, String separator) {
    StringBuilder sb = new StringBuilder();
    for (int32 i = 0; i < nums.length; i++) {
        sb.append(nums[i]);
        if (i < nums.length - 1) {
            sb.append(separator);
        }
    }
    return sb.to_string();
}

public void main(char[][] args) {
    int32[] nums = {1, 2, 3, 4, 5};
    String result = join_numbers(nums, new String(", "));
    Console.println(result);   // 1, 2, 3, 4, 5
}
```

### Insertion and Deletion

```solix
StringBuilder sb = new StringBuilder("Hello World");
sb.insert(5, new String(","));   // "Hello, World"
sb.delete_char_at(11);           // "Hello, Worl"
sb.delete_range(7, 11);          // "Hello, "
sb.append("there!");             // "Hello, there!"
Console.println(sb.to_string());
```

### Reverse a String

```solix
StringBuilder sb = new StringBuilder("abcdef");
sb.reverse();
Console.println(sb.to_string());   // fedcba
```
