# Standard Library (`solixlib`) Master Test Specification

This document is the authoritative test specification matrix for the Solix Standard Library (`solixlib`). It tracks all **Positive Test Scenarios** (valid usage, operations, outputs) and **Negative Test Scenarios** (expected runtime exceptions, boundary conditions, error handling) across all standard library submodules.

---

## Specification Protocol

Each test scenario is assigned a permanent identifier and status tag:
- `[IMPLEMENTED]`: Automated Catch2 / Solix test exists, executes in CI, and is currently passing.
- `[NOT IMPLEMENTED]`: Scenario is defined and planned, but not yet implemented.

---

## 1. Exceptions (`solix.exceptions`)

- [x] **Case 1.1 [IMPLEMENTED]**: Base `Exception` instantiation with message and `to_string()` retrieval (`tests/solixlib/test_exceptions.cpp`).
- [x] **Case 1.2 [IMPLEMENTED]**: Catching `IllegalArgumentException` via base `RuntimeException` and `Exception` polymorphism (`tests/solixlib/test_exceptions.cpp`).
- [x] **Case 1.3 [IMPLEMENTED]**: Throwing and catching `IndexOutOfBoundsException` with upper/lower bound details (`tests/solixlib/test_exceptions.cpp`).
- [x] **Case 1.4 [IMPLEMENTED]**: Throwing and catching `DivideByZeroException` during numeric zero division caught as `ArithmeticException` (`tests/solixlib/test_exceptions.cpp`).
- [x] **Case 1.5 [IMPLEMENTED]**: Throwing and catching `NullReferenceException` on dereferencing null objects / explicit throw (`tests/solixlib/test_exceptions.cpp`).
- [x] **Case 1.6 [IMPLEMENTED]**: Throwing and catching `InvalidOperationException` on invalid state transitions (`tests/solixlib/test_exceptions.cpp`).
- [x] **Case 1.7 [IMPLEMENTED]**: Throwing and catching `FormatException` on failed type parsing (`tests/solixlib/test_exceptions.cpp`).
- [x] **Case 1.8 [IMPLEMENTED]**: Throwing and catching `FileNotFoundException` as a specialization of `IOException` (`tests/solixlib/test_exceptions.cpp`).
- [x] **Case 1.9 [IMPLEMENTED]**: Exception chaining: verify `get_cause()` returns inner exception (`tests/solixlib/test_exceptions.cpp`).
- [x] **Case 1.10 [IMPLEMENTED]**: Parameter metadata verification for `ArgumentNullException` and `ArgumentOutOfRangeException` (`tests/solixlib/test_exceptions.cpp`).
- [x] **Case 1.11 [IMPLEMENTED]**: `SocketException` error code storage and retrieval (`tests/solixlib/test_exceptions.cpp`).
- [x] **Case 1.12 [IMPLEMENTED]**: `AssertionError` instantiation and catching via base `Exception` (`tests/solixlib/test_exceptions.cpp`).
- [x] **Case 1.13 [IMPLEMENTED]**: Nested `try-catch-finally` ensuring `finally` blocks execute during exception unwinding (`tests/solixlib/test_exceptions.cpp`).

---

## 2. Console (`solix.system.Console`)

- [x] **Case 2.1 [IMPLEMENTED]**: `Console.print()` and `Console.println()` with primitive integers, booleans, doubles, and characters (`tests/solixlib/test_console.cpp`).
- [x] **Case 2.2 [IMPLEMENTED]**: `Console.print()` and `Console.println()` with primitive character arrays (`char[]`) (`tests/solixlib/test_console.cpp`).
- [x] **Case 2.3 [IMPLEMENTED]**: `Console.error()` outputting in ANSI red (`\033[31m`) to standard error stream (`tests/solixlib/test_console.cpp`).
- [x] **Case 2.4 [IMPLEMENTED]**: `Console.warning()`, `Console.info()`, `Console.success()` outputting appropriate ANSI colors (`tests/solixlib/test_console.cpp`).
- [x] **Case 2.5 [IMPLEMENTED]**: `Console.input_int()` parsing valid integer from stdin (`tests/solixlib/test_console.cpp`).
- [x] **Case 2.6 [IMPLEMENTED]**: `Console.input_double()` parsing valid double from stdin (`tests/solixlib/test_console.cpp`).
- [x] **Case 2.7 [IMPLEMENTED]**: `Console.input_bool()` parsing boolean (`true`/`false`) from stdin (`tests/solixlib/test_console.cpp`).
- [x] **Case 2.8 [IMPLEMENTED]**: `Console.input_char()` reading single character from stdin (`tests/solixlib/test_console.cpp`).
- [x] **Case 2.9 [IMPLEMENTED]**: `Console.input_chars()` reading line into `char[]` buffer (`tests/solixlib/test_console.cpp`).
- [x] **Case 2.10 [IMPLEMENTED]**: Negative: `Console.input_int()` with non-numeric input throws `FormatException` (`tests/solixlib/test_console.cpp`).
- [x] **Case 2.11 [IMPLEMENTED]**: Negative: `Console.input_double()` with invalid text throws `FormatException` (`tests/solixlib/test_console.cpp`).
- [x] **Case 2.12 [IMPLEMENTED]**: Terminal control operations (`clear()`, `flush()`, `set_color()`, `reset_color()`, `set_cursor_position()`, `set_title()`) (`tests/solixlib/test_console.cpp`).
- [x] **Case 2.13 [IMPLEMENTED]**: Post-String upgrade: `Console.print()` and `Console.println()` with `String` and `IStringable` instances (`tests/solixlib/test_string.cpp`).
- [x] **Case 2.14 [IMPLEMENTED]**: Post-String upgrade: `Console.input() -> String` reading complete line (`tests/solixlib/test_string.cpp`).

---

## 3. String & StringBuilder (`solix.core.String`, `StringBuilder`, `IStringable`)

- [x] **Case 3.1 [IMPLEMENTED]**: `String` basic operations: `length()`, `is_empty()`, `char_at()`, and out-of-bounds checks (`tests/solixlib/test_string.cpp`).
- [x] **Case 3.2 [IMPLEMENTED]**: Substring operations: `substring(start, end)`, `substring(start)`, parameter validation (`tests/solixlib/test_string.cpp`).
- [x] **Case 3.3 [IMPLEMENTED]**: Search & inspection: `index_of()`, `contains()`, `starts_with()`, `ends_with()` (`tests/solixlib/test_string.cpp`).
- [x] **Case 3.4 [IMPLEMENTED]**: Transformations: `to_upper_case()`, `to_lower_case()`, `trim()`, `replace()` (`tests/solixlib/test_string.cpp`).
- [x] **Case 3.5 [IMPLEMENTED]**: Split and join: `split()`, `join()` (`tests/solixlib/test_string.cpp`).
- [x] **Case 3.6 [IMPLEMENTED]**: Numeric parsing: `parse_int()`, `parse_double()`, `parse_bool()` with positive and negative inputs (`tests/solixlib/test_string.cpp`).
- [x] **Case 3.7 [IMPLEMENTED]**: Static conversions: `value_of()` for integer, float, bool, and char (`tests/solixlib/test_string.cpp`).
- [x] **Case 3.8 [IMPLEMENTED]**: `StringBuilder` capacity growth, chaining, insertions, deletions, reversals (`tests/solixlib/test_string.cpp`).
- [x] **Case 3.9 [IMPLEMENTED]**: Interoperability with `Console`: printing `String` and custom `IStringable` (`tests/solixlib/test_string.cpp`).

---

## 4. Primitives & Types (`solix.core.Primitives`, `Optional<T>`, `Any`, Contracts)

- [x] **Case 4.1 [IMPLEMENTED]**: Boxed `Int` operations: `value()`, `to_string()`, `hash_code()`, `equals()`, `compare_to()`, `parse()`, `try_parse()`, and bitwise utilities (`count_leading_zeros`, `count_trailing_zeros`, `bit_count`, `reverse_bytes`) (`tests/solixlib/test_primitives.cpp`).
- [x] **Case 4.2 [IMPLEMENTED]**: Boxed `Double` operations, IEEE 754 constants (`NaN`, `POSITIVE_INFINITY`, `NEGATIVE_INFINITY`), queries (`is_nan`, `is_infinite`), and parsing (`tests/solixlib/test_primitives.cpp`).
- [x] **Case 4.3 [IMPLEMENTED]**: Boxed `Bool` and `Char` operations, character classification (`is_digit`, `is_letter`, `is_whitespace`, `is_upper_case`, `is_lower_case`), and casing transformations (`tests/solixlib/test_primitives.cpp`).
- [x] **Case 4.4 [IMPLEMENTED]**: `Optional<T>` value presence, unwrapping, and fallback (`has_value`, `is_empty`, `value`, `value_or`) (`tests/solixlib/test_primitives.cpp`).
- [x] **Case 4.5 [IMPLEMENTED]**: Negative: accessing `Optional.empty().value()` throws `InvalidOperationException` (`tests/solixlib/test_primitives.cpp`).
- [x] **Case 4.6 [IMPLEMENTED]**: `Any` dynamic container boxing (`int`, `double`, `bool`, `char`, `String`), type inspection (`is_*`, `type_name`), equality, and unwrapping (`as_*`) (`tests/solixlib/test_primitives.cpp`).
- [x] **Case 4.7 [IMPLEMENTED]**: Negative: invalid `Any` unwrapping type mismatch throws `InvalidOperationException` (`tests/solixlib/test_primitives.cpp`).
- [x] **Case 4.8 [IMPLEMENTED]**: Negative: `Int.parse()` and `Double.parse()` with non-numeric inputs throw `FormatException` (`tests/solixlib/test_primitives.cpp`).

---

## 5. Math & Random (`solix.math.Math`, `Random`)

- [x] **Case 5.1 [IMPLEMENTED]**: Mathematical constants (`PI`, `E`, `TAU`), basic bounds and signs (`abs`, `min`, `max`, `clamp`, `sign`, `copy_sign`) (`tests/solixlib/test_math.cpp`).
- [x] **Case 5.2 [IMPLEMENTED]**: Exponential, power, and logarithmic algorithms (`sqrt`, `cbrt`, `hypot`, `pow`, `exp`, `log`, `log10`, `log2`) (`tests/solixlib/test_math.cpp`).
- [x] **Case 5.3 [IMPLEMENTED]**: Trigonometric, degree/radian conversions, and rounding (`sin`, `cos`, `to_radians`, `to_degrees`, `floor`, `ceil`, `round`, `trunc`) (`tests/solixlib/test_math.cpp`).
- [x] **Case 5.4 [IMPLEMENTED]**: PRNG determinism with seed and distribution across integer ranges, floats, and booleans (`tests/solixlib/test_math.cpp`).
- [x] **Case 5.5 [IMPLEMENTED]**: Negative: Random bounds validation throws `IllegalArgumentException` on invalid/negative bounds or inverted ranges (`tests/solixlib/test_math.cpp`).

---

## 6. Chrono & Time (`solix.time.Duration`, `Instant`, `DateTime`, `Stopwatch`)

- [x] **Case 6.1 [IMPLEMENTED]**: `Duration` creation (`nanos`, `micros`, `millis`, `seconds`, `minutes`, `hours`, `days`), conversions, arithmetic (`plus`, `minus`), comparisons, and formatting (`tests/solixlib/test_time.cpp`).
- [x] **Case 6.2 [IMPLEMENTED]**: `Instant` monotonic timestamps, ordering (`is_before`, `is_after`), duration differences (`duration_until`), and offset adjustments (`tests/solixlib/test_time.cpp`).
- [x] **Case 6.3 [IMPLEMENTED]**: `DateTime` UTC & Local component decomposition (year, month, day, hour, minute, second, day-of-week, day-of-year), leap year validation, and ISO 8601 formatting (`tests/solixlib/test_time.cpp`).
- [x] **Case 6.4 [IMPLEMENTED]**: `Stopwatch` lifecycle (start, stop, reset, restart, elapsed duration querying) (`tests/solixlib/test_time.cpp`).
- [x] **Case 6.5 [IMPLEMENTED]**: Sleep duration validation via native sleep binding (`tests/solixlib/test_time.cpp`).
- [x] **Case 6.6 [IMPLEMENTED]**: Negative: invalid `DateTime` date bounds validation throws `IllegalArgumentException` (`tests/solixlib/test_time.cpp`).

---

## 7. Collections Core (`solix.collections.Core`)

- [x] **Case 7.1 [IMPLEMENTED]**: `IIterator` step-through navigation and contract validation (`has_next()`, `next()`) (`tests/solixlib/test_collections_core.cpp`).
- [x] **Case 7.2 [IMPLEMENTED]**: `ICollection` and `IReadOnlyCollection` lifecycle: `size()`, `is_empty()`, `contains()`, `to_array()`, `clear()`, and `to_string()` formatting via `Collections.to_string()` (`tests/solixlib/test_collections_core.cpp`).
- [x] **Case 7.3 [IMPLEMENTED]**: Polymorphic interface dispatch and `Console.print` integration across `ICollection`, `IIterable`, and `IStringable` (`tests/solixlib/test_collections_core.cpp`).
- [x] **Case 7.4 [IMPLEMENTED]**: `IList` contract index-based access, mutation, insertion, element removal, and index lookups (`tests/solixlib/test_collections_core.cpp`).
- [x] **Case 7.5 [IMPLEMENTED]**: `IDeque` contract double-ended queue operations (`add_first`, `add_last`, `remove_first`, `remove_last`, `peek_first`, `peek_last`) and string representation (`tests/solixlib/test_collections_core.cpp`).
- [x] **Case 7.6 [IMPLEMENTED]**: Negative: Calling `next()` on exhausted iterator throws `NoSuchElementException` (`tests/solixlib/test_collections_core.cpp`).

