# Solix Standard Library: Primitives & Types (`solix.core`)

## Overview

The `solix.core` primitives module provides language-level contracts, boxed primitive wrappers, IEEE 754 floating-point utilities, and the generic `Optional<T>` container for safe absent values.

---

## 1. Contracts & Interfaces

### `IComparable`
```solix
package solix.core;

public interface IComparable {
    int32 compare_to(Any other);
}
```

### `IEquatable`
```solix
package solix.core;

public interface IEquatable {
    bool equals(Any other);
}
```

### `ICloneable`
```solix
package solix.core;

public interface ICloneable {
    Any clone();
}
```

### `IHashable`
```solix
package solix.core;

public interface IHashable {
    int32 hash_code();
}
```

---

## 2. Boxed Primitives

### `Int`
Boxed 32-bit signed integer.
- `public Int(int32 val)`
- `value() -> int32`: Retrieves underlying integer.
- `to_string() -> String`: Converts to string representation.
- `hash_code() -> int32`: Returns integer value.
- `equals(Int other) -> bool`: Value equality.
- `compare_to(Int other) -> int32`: Returns `-1`, `0`, or `1`.
- `static parse(String s) -> int32`: Parses decimal string; throws `FormatException` on invalid format.
- `static try_parse(String s, Int out_val) -> bool`: Safe parsing without throwing exceptions.
- `static count_leading_zeros(int32 v) -> int32`: Native `__builtin_clz`.
- `static count_trailing_zeros(int32 v) -> int32`: Native `__builtin_ctz`.
- `static bit_count(int32 v) -> int32`: Native `__builtin_popcount`.
- `static reverse_bytes(int32 v) -> int32`: Native `__builtin_bswap32`.
- `MIN_VALUE`: `-2147483648`
- `MAX_VALUE`: `2147483647`

### `Double`
Boxed 64-bit IEEE 754 floating-point number.
- `public Double(float64 val)`
- `value() -> float64`: Retrieves underlying float64.
- `to_string() -> String`: Formats float64 to string.
- `equals(Double other) -> bool`: Value equality.
- `compare_to(Double other) -> int32`: Returns `-1`, `0`, or `1`.
- `static parse(String s) -> float64`: Parses string to double; throws `FormatException` on error.
- `static is_nan(float64 d) -> bool`: Returns `true` if `d` is quiet NaN.
- `static is_infinite(float64 d) -> bool`: Returns `true` if `d` is $+ \infty$ or $- \infty$.
- Constants: `NaN`, `POSITIVE_INFINITY`, `NEGATIVE_INFINITY`, `MIN_VALUE`, `MAX_VALUE`.

### `Bool`
Boxed boolean wrapper.
- `public Bool(bool val)`
- `value() -> bool`: Retrieves underlying boolean.
- `to_string() -> String`: Returns `"true"` or `"false"`.
- `hash_code() -> int32`: Returns `1231` for true, `1237` for false.
- `equals(Bool other) -> bool`
- `compare_to(Bool other) -> int32`
- `static parse(String s) -> bool`: Parses `"true"` / `"false"`.

### `Char`
Boxed UTF-8 code point / character wrapper.
- `public Char(char val)`
- `value() -> char`: Retrieves character.
- `to_string() -> String`: Converts character to single-character string.
- `hash_code() -> int32`: Returns numeric codepoint.
- `static is_digit(char c) -> bool`: Checks if character is `'0'`..`'9'`.
- `static is_letter(char c) -> bool`: Checks if character is ASCII alphabetic.
- `static is_whitespace(char c) -> bool`: Checks if character is `' '`, `'\t'`, `'\n'`, or `'\r'`.
- `static is_upper_case(char c) -> bool`: Checks if character is `'A'`..`'Z'`.
- `static to_upper_case(char c) -> char`: In-place case conversion.
- `static to_lower_case(char c) -> char`: In-place case conversion.

### `Byte`, `Short`, `Long`
Signed integer wrappers for `int8`, `int16`, and `int64`.
- `public Byte(int8 val)`, `public Short(int16 val)`, `public Long(int64 val)`
- `value()`: Retrieves underlying primitive.
- `to_string() -> String`: String representation.
- `hash_code() -> int32`: Integer hash code.
- `equals(other) -> bool`: Value equality.
- `compare_to(other) -> int32`: Ordinal comparison.
- `static parse(String s)`: Parses string to numeric value; throws `FormatException` on error.
- Arithmetic and relational operator overloads: `+`, `-`, `*`, `/`, `==`, `!=`, `<`, `<=`, `>`, `>=`.
- Bounds constants: `MIN_VALUE`, `MAX_VALUE`.

### `UByte`, `UShort`, `UInt`, `ULong`
Unsigned integer wrappers for `uint8`, `uint16`, `uint32`, and `uint64`.
- `public UByte(uint8 val)`, `public UShort(uint16 val)`, `public UInt(uint32 val)`, `public ULong(uint64 val)`
- `value()`: Retrieves underlying unsigned primitive.
- `to_string() -> String`: String representation.
- `hash_code() -> int32`: Integer hash code.
- `equals(other) -> bool`: Value equality.
- `compare_to(other) -> int32`: Ordinal comparison.
- Arithmetic and relational operator overloads: `+`, `-`, `*`, `/`, `==`, `!=`, `<`, `<=`, `>`, `>=`.
- Bounds constants: `MIN_VALUE`, `MAX_VALUE`.

### `Float`
Boxed 32-bit IEEE 754 floating-point number.
- `public Float(float32 val)`
- `value() -> float32`: Retrieves underlying float32.
- `to_string() -> String`: String representation.
- `equals(Float other) -> bool`: Value equality.
- `compare_to(Float other) -> int32`: Ordinal comparison.
- Arithmetic and relational operator overloads: `+`, `-`, `*`, `/`, `==`, `!=`, `<`, `<=`, `>`, `>=`.
- Bounds constants: `MIN_VALUE`, `MAX_VALUE`.

---

## 3. Container: `Optional<T>`

Generic container representing optional or absent values without null-pointer vulnerabilities.

### Constructors
- `new Optional<T>()`: Constructs empty optional (`has_value() == false`).
- `new Optional<T>(T val)`: Constructs present optional (`has_value() == true`).
- `static of<T>(T val) -> Optional<T>`: Constructs present optional.
- `static empty<T>() -> Optional<T>`: Constructs empty optional.

### Query, Value Access & Functional Operations
- `has_value() -> bool`: Returns `true` if a value is contained.
- `is_empty() -> bool`: Returns `true` if no value is present.
- `value() -> T`: Retrieves contained value. Throws `InvalidOperationException` if empty.
- `value_or(T fallback) -> T`: Returns contained value if present, or `fallback` if empty.
- `if_present(void(*)(T) consumer) -> void`: Executes `consumer` callback with the contained value if present.
- `filter(bool(*)(T) predicate) -> Optional<T>`: Returns `this` if present and `predicate(value)` is true, or empty Optional otherwise.
- `map<U>(U(*)(T) mapper) -> Optional<U>`: Transforms contained value if present, returning empty Optional otherwise.
- `flat_map<U>(Optional<U>(*)(T) mapper) -> Optional<U>`: Monadic chaining; evaluates mapper returning `Optional<U>`.
- `to_string() -> String`: Returns `"Optional.of(...)"` or `"Optional.empty"`.

---

## 4. Friendly Primitive Aliases (`Primitives.slx`)

Package `solix.core` provides friendly synonyms for core primitive scalar types defined in `src/solix/core/Primitives.slx`. Importing `solix.core.*` exposes these aliases for streamlined typing:

| Friendly Alias | Underlying Primitive | Description |
|---|---|---|
| `int` | `int32` | 32-bit signed integer |
| `long` | `int64` | 64-bit signed integer |
| `short` | `int16` | 16-bit signed integer |
| `byte` | `int8` | 8-bit signed integer |
| `ubyte` | `uint8` | 8-bit unsigned integer |
| `ushort` | `uint16` | 16-bit unsigned integer |
| `uint` | `uint32` | 32-bit unsigned integer |
| `ulong` | `uint64` | 64-bit unsigned integer |
| `float` | `float32` | 32-bit single-precision float |
| `double` | `float64` | 64-bit double-precision float |

### Example Usage
```solix
import solix.core.*;

int add(int a, int b) {
    return a + b;
}

double multiply(double a, double b) {
    return a * b;
}
```

