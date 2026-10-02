# Solix Standard Library: Primitives & Types (`solix.core`)

## Overview

The `solix.core` primitives module provides language-level contracts, boxed primitive wrappers, IEEE 754 floating-point utilities, the generic `Optional<T>` container for safe absent values, and `Any` for dynamic value encapsulation.

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
- `static is_lower_case(char c) -> bool`: Checks if character is `'a'`..`'z'`.
- `static to_upper_case(char c) -> char`: In-place case conversion.
- `static to_lower_case(char c) -> char`: In-place case conversion.

---

## 3. Container: `Optional<T>`

Generic container representing optional or absent values without null-pointer vulnerabilities.

### Constructors
- `new Optional<T>()`: Constructs empty optional (`has_value() == false`).
- `new Optional<T>(T val)`: Constructs present optional (`has_value() == true`).

### Query & Value Access
- `has_value() -> bool`: Returns `true` if a value is contained.
- `is_empty() -> bool`: Returns `true` if no value is present.
- `value() -> T`: Retrieves contained value. Throws `InvalidOperationException` if empty.
- `value_or(T fallback) -> T`: Returns contained value if present, or `fallback` if empty.
- `to_string() -> String`: Returns `"Optional.of(...)"` or `"Optional.empty"`.

---

## 4. Container: `Any`

Universal dynamic value encapsulation supporting primitive boxing, object wrapping, and type inspection.

### Constructors & Factories
- `new Any(int32 val)` / `Any.from_int(val)`
- `new Any(float64 val)` / `Any.from_double(val)`
- `new Any(bool val)` / `Any.from_bool(val)`
- `new Any(char val)` / `Any.from_char(val)`
- `new Any(String val)` / `Any.from_string(val)`
- `new Any(IStringable val)` / `Any.from_object(val)`

### Inspection & Unwrapping
- `is_null() -> bool`
- `is_int() -> bool`
- `is_double() -> bool`
- `is_bool() -> bool`
- `is_char() -> bool`
- `is_string() -> bool`
- `is_object() -> bool`
- `type_name() -> String`: Returns `"int32"`, `"float64"`, `"bool"`, `"char"`, `"String"`, `"Object"`, or `"null"`.
- `as_int() -> int32`: Unwraps int32. Throws `InvalidOperationException` if kind mismatch.
- `as_double() -> float64`: Unwraps float64. Throws `InvalidOperationException` if kind mismatch.
- `as_bool() -> bool`: Unwraps bool. Throws `InvalidOperationException` if kind mismatch.
- `as_char() -> char`: Unwraps char. Throws `InvalidOperationException` if kind mismatch.
- `as_string() -> String`: Unwraps String. Throws `InvalidOperationException` if kind mismatch.
- `as_object() -> IStringable`: Unwraps object reference. Throws `InvalidOperationException` if kind mismatch.
