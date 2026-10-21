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

## 2. Primitive Utilities (`Primitives`)

In Solix v1.1.0, redundant boxed primitive classes (`Int`, `Double`, `Bool`, `Char`, `Byte`, `Short`, `Long`, `UByte`, `UShort`, `UInt`, `ULong`, `Float`) were pruned in favor of Solix's zero-cost native primitive types and the unified `Primitives` static utility class.

### `Primitives`
Class `solix.core.Primitives` provides native bit manipulation, floating-point queries/constants, character inspection/case-folding, and string conversion utilities:

#### Integer & Bitwise Primitives
- `static count_leading_zeros(int32 val) -> int32`: Fast native leading zero count (`clz`).
- `static count_trailing_zeros(int32 val) -> int32`: Fast native trailing zero count (`ctz`).
- `static bit_count(int32 val) -> int32`: Population count (`popcount`).
- `static reverse_bytes(int32 val) -> int32`: Byte reversal (`bswap32`).
- `MIN_INT`: `-2147483648`
- `MAX_INT`: `2147483647`

#### Floating-Point Utilities (IEEE 754)
- `static is_nan(float64 d) -> bool`: Returns `true` if `d` is quiet/signaling NaN.
- `static is_infinite(float64 d) -> bool`: Returns `true` if `d` is positive or negative infinity.
- `static nan() -> float64`: Returns IEEE 754 quiet NaN.
- `static positive_infinity() -> float64`: Returns positive infinity ($+\infty$).
- `static negative_infinity() -> float64`: Returns negative infinity ($-\infty$).
- `static min_double() -> float64`: Smallest normalized positive `float64` (`2.2250738585072014e-308`).
- `static max_double() -> float64`: Maximum finite `float64` (`1.7976931348623157e+308`).
- Constants: `NaN`, `POSITIVE_INFINITY`, `NEGATIVE_INFINITY`, `MIN_DOUBLE`, `MAX_DOUBLE`.

#### Character Utilities
- `static is_digit(char c) -> bool`: Returns `true` if `c` is `'0'`..`'9'`.
- `static is_letter(char c) -> bool`: Returns `true` if `c` is ASCII `'a'`..`'z'` or `'A'`..`'Z'`.
- `static is_whitespace(char c) -> bool`: Returns `true` if `c` is `' '`, `'\t'`, `'\n'`, or `'\r'`.
- `static is_upper_case(char c) -> bool`: Returns `true` if `c` is uppercase.
- `static is_lower_case(char c) -> bool`: Returns `true` if `c` is lowercase.
- `static to_upper_case(char c) -> char`: Converts lowercase ASCII character to uppercase.
- `static to_lower_case(char c) -> char`: Converts uppercase ASCII character to lowercase.

#### Parsing Utilities
- `static parse_int(String s) -> int32`: Parses decimal string to 32-bit signed integer.
- `static parse_double(String s) -> float64`: Parses string representation to 64-bit float.
- `static parse_bool(String s) -> bool`: Parses `"true"` or `"false"` to boolean.

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

