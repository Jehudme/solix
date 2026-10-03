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

## 4. Primitives & Types (`solix.core.Primitives`, `Optional<T>`, Contracts)

- [x] **Case 4.1 [IMPLEMENTED]**: Boxed `Int` operations: `value()`, `to_string()`, `hash_code()`, `equals()`, `compare_to()`, `parse()`, `try_parse()`, and bitwise utilities (`count_leading_zeros`, `count_trailing_zeros`, `bit_count`, `reverse_bytes`) (`tests/solixlib/test_primitives.cpp`).
- [x] **Case 4.2 [IMPLEMENTED]**: Boxed `Double` operations, IEEE 754 constants (`NaN`, `POSITIVE_INFINITY`, `NEGATIVE_INFINITY`), queries (`is_nan`, `is_infinite`), and parsing (`tests/solixlib/test_primitives.cpp`).
- [x] **Case 4.3 [IMPLEMENTED]**: Boxed `Bool` and `Char` operations, character classification (`is_digit`, `is_letter`, `is_whitespace`, `is_upper_case`, `is_lower_case`), and casing transformations (`tests/solixlib/test_primitives.cpp`).
- [x] **Case 4.4 [IMPLEMENTED]**: `Optional<T>` value presence, unwrapping, and fallback (`has_value`, `is_empty`, `value`, `value_or`) (`tests/solixlib/test_primitives.cpp`).
- [x] **Case 4.5 [IMPLEMENTED]**: Negative: accessing `Optional.empty().value()` throws `InvalidOperationException` (`tests/solixlib/test_primitives.cpp`).
- [x] **Case 4.6 [IMPLEMENTED]**: `Optional<T>` functional operations: `if_present(consumer)` callback execution and `filter(predicate)` matching/discarding (`tests/solixlib/test_primitives.cpp`).
- [x] **Case 4.7 [IMPLEMENTED]**: Negative: `Int.parse()` and `Double.parse()` with non-numeric inputs throw `FormatException` (`tests/solixlib/test_primitives.cpp`).

---

## 5. Math & Random (`solix.math.Math`, `Random`)

- [x] **Case 5.1 [IMPLEMENTED]**: Mathematical constants (`PI`, `E`, `TAU`), basic bounds and signs (`abs`, `min`, `max`, `clamp`, `sign`, `copy_sign`) (`tests/solixlib/test_math.cpp`).
- [x] **Case 5.2 [IMPLEMENTED]**: Exponential, power, and logarithmic algorithms (`sqrt`, `cbrt`, `hypot`, `pow`, `exp`, `log`, `log10`, `log2`) (`tests/solixlib/test_math.cpp`).
- [x] **Case 5.3 [IMPLEMENTED]**: Trigonometric, degree/radian conversions, and rounding (`sin`, `cos`, `to_radians`, `to_degrees`, `floor`, `ceil`, `round`, `trunc`) (`tests/solixlib/test_math.cpp`).
- [x] **Case 5.4 [IMPLEMENTED]**: PRNG determinism with seed and distribution across integer ranges, floats, and booleans (`tests/solixlib/test_math.cpp`).
- [x] **Case 5.5 [IMPLEMENTED]**: Negative: Random bounds validation throws `IllegalArgumentException` on invalid/negative bounds or inverted ranges (`tests/solixlib/test_math.cpp`).
- [x] **Case 5.6 [IMPLEMENTED]**: Generic `Math` operations across numeric types (`int32`, `int64`, `float64`) using explicit and inferred templates for `abs<T>`, `min<T>`, `max<T>`, `clamp<T>`, `sign<T>`, and `copy_sign<T>` (`tests/solixlib/test_math.cpp`).

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

---

## 8. Lists & Linear Sequences (`solix.collections.List`, `LinkedList`, `Algorithms`)

- [x] **Case 8.1 [IMPLEMENTED]**: `List<T>` dynamic capacity expansion, indexing (`get`, `set`), `for_each` lambda traversal, and functional `filter` predicate (`tests/solixlib/test_list.cpp`).
- [x] **Case 8.2 [IMPLEMENTED]**: `List<T>` mutation operations (`insert`, `remove_at`, `remove`, `contains`, `index_of`), slicing (`sub_list`), in-place reversal, and string conversion (`tests/solixlib/test_list.cpp`).
- [x] **Case 8.3 [IMPLEMENTED]**: `LinkedList<T>` double-ended queue operations (`add_first`, `add_last`, `remove_first`, `remove_last`, `peek_first`, `peek_last`), `for_each` lambda iteration, and formatted string rendering (`tests/solixlib/test_list.cpp`).
- [x] **Case 8.4 [IMPLEMENTED]**: `LinkedList<T>` index-based access, mutation (`get`, `set`, `insert`, `remove_at`, `remove`) with bidirectional node traversal and `filter` (`tests/solixlib/test_list.cpp`).
- [x] **Case 8.5 [IMPLEMENTED]**: Generic `Algorithms` collection utilities (`swap<T>`, `reverse<T>`, `fill<T>`, comparator-based `sort<T>`, comparator-based `binary_search<T>`) (`tests/solixlib/test_list.cpp`).
- [x] **Case 8.6 [IMPLEMENTED]**: Negative: `List<T>` invalid negative capacity and out-of-bounds index access throw `IllegalArgumentException` and `IndexOutOfBoundsException` (`tests/solixlib/test_list.cpp`).
- [x] **Case 8.7 [IMPLEMENTED]**: Negative: `LinkedList<T>` empty deque operations throw `NoSuchElementException` and out-of-bounds indexing throws `IndexOutOfBoundsException` (`tests/solixlib/test_list.cpp`).

---

## 9. Maps & Associative Dictionaries (`solix.collections.Map`, `HashMap`, `TreeMap`, `KeyValuePair`)

- [x] **Case 9.1 [IMPLEMENTED]**: `HashMap<K, V>` generic insertion, retrieval (`get`, `get_or_default`), updating existing keys, key and value membership (`contains_key`, `contains_value`), and removal (`tests/solixlib/test_map.cpp`).
- [x] **Case 9.2 [IMPLEMENTED]**: `HashMap<K, V>` automatic load-factor rehashing across multi-key insertions verifying data preservation (`tests/solixlib/test_map.cpp`).
- [x] **Case 9.3 [IMPLEMENTED]**: `HashMap<K, V>` view collections (`keys()`, `values()`, `entries()`), `for_each` lambda iteration, and string representation (`tests/solixlib/test_map.cpp`).
- [x] **Case 9.4 [IMPLEMENTED]**: `TreeMap<K, V>` ordered insertion, boundary lookups (`first_key()`, `last_key()`), and in-order sorted key traversal (`tests/solixlib/test_map.cpp`).
- [x] **Case 9.5 [IMPLEMENTED]**: `TreeMap<K, V>` node removal (leaf, internal, root), dictionary clearing, `for_each` lambda iteration, and membership queries (`tests/solixlib/test_map.cpp`).
- [x] **Case 9.6 [IMPLEMENTED]**: Negative: `HashMap<K, V>.get()` with missing key throws `KeyNotFoundException` (`tests/solixlib/test_map.cpp`).
- [x] **Case 9.7 [IMPLEMENTED]**: Negative: `TreeMap<K, V>.get()` with missing key throws `KeyNotFoundException` and `first_key()` on empty map throws `NoSuchElementException` (`tests/solixlib/test_map.cpp`).

---

## 10. Sets (`solix.collections.Set`, `HashSet`, `TreeSet`)

- [x] **Case 10.1 [IMPLEMENTED]**: `HashSet<T>` distinct insertion, duplicate rejection, contains, remove, clear, and `for_each` lambda iteration (`tests/solixlib/test_set.cpp`).
- [x] **Case 10.2 [IMPLEMENTED]**: `HashSet<T>` set algebra: `union_with`, `intersect_with`, `difference_with` (`tests/solixlib/test_set.cpp`).
- [x] **Case 10.3 [IMPLEMENTED]**: `HashSet<T>` subset and superset relationships (`is_subset_of`, `is_superset_of`) (`tests/solixlib/test_set.cpp`).
- [x] **Case 10.4 [IMPLEMENTED]**: `TreeSet<T>` ordered uniqueness, boundary lookups (`first()`, `last()`), in-order iterator traversal, and `for_each` lambda iteration (`tests/solixlib/test_set.cpp`).
- [x] **Case 10.5 [IMPLEMENTED]**: `TreeSet<T>` node removal, duplicate rejections, and `union_with` (`tests/solixlib/test_set.cpp`).
- [x] **Case 10.6 [IMPLEMENTED]**: Negative: `TreeSet<T>` `first()` and `last()` on empty set throw `NoSuchElementException` (`tests/solixlib/test_set.cpp`).

---

## 11. Linear Collections (`solix.collections.Linear`: `Stack`, `Queue`, `Deque`, `PriorityQueue`, `CircularBuffer`, `BitSet`)

- [x] **Case 11.1 [IMPLEMENTED]**: `Stack<T>` LIFO operations (`push`, `peek`, `pop`, `size`, `is_empty`, `clear`), `for_each` lambda iteration, and array conversion (`tests/solixlib/test_linear_collections.cpp`).
- [x] **Case 11.2 [IMPLEMENTED]**: `Queue<T>` FIFO operations (`enqueue`, `peek`, `dequeue`, `size`, `is_empty`), `for_each` lambda iteration, and order preservation (`tests/solixlib/test_linear_collections.cpp`).
- [x] **Case 11.3 [IMPLEMENTED]**: `Deque<T>` double-ended operations (`push_front`/`push_back`, `peek_front`/`peek_back`, `pop_front`/`pop_back`), `for_each` lambda iteration (`tests/solixlib/test_linear_collections.cpp`).
- [x] **Case 11.4 [IMPLEMENTED]**: `PriorityQueue<T>` binary heap with comparator lambda (`int32(*)(T, T)`), extraction order, peek, and `for_each` (`tests/solixlib/test_linear_collections.cpp`).
- [x] **Case 11.5 [IMPLEMENTED]**: `CircularBuffer<T>` ring buffer semantics, overwrite mode vs non-overwrite mode, wrapping around capacity boundaries, and `for_each` (`tests/solixlib/test_linear_collections.cpp`).
- [x] **Case 11.6 [IMPLEMENTED]**: `BitSet` bit manipulation: `set`, `get`, `clear`, `flip`, bitwise `and`, `or`, `xor`, and `cardinality` (`tests/solixlib/test_linear_collections.cpp`).
- [x] **Case 11.7 [IMPLEMENTED]**: Negative: `Stack<T>.pop()` and `peek()` on empty stack throw `InvalidOperationException` (`tests/solixlib/test_linear_collections.cpp`).
- [x] **Case 11.8 [IMPLEMENTED]**: Negative: `Queue<T>.dequeue()` and `peek()` on empty queue throw `InvalidOperationException` (`tests/solixlib/test_linear_collections.cpp`).
- [x] **Case 11.9 [IMPLEMENTED]**: Negative: `CircularBuffer<T>.read()` on empty buffer throws `InvalidOperationException` (`tests/solixlib/test_linear_collections.cpp`).
- [x] **Case 11.10 [IMPLEMENTED]**: Negative: `BitSet` negative bit index throws `IndexOutOfBoundsException` and null operand throws `IllegalArgumentException` (`tests/solixlib/test_linear_collections.cpp`).

---

## 12. Filesystem (`solix.io.filesystem`: `Path`, `File`, `Directory`)

- [x] **Case 12.1 [IMPLEMENTED]**: `Path` manipulation: `combine`, `get_directory_name`, `get_file_name`, `get_extension`, `get_file_name_without_extension`, `is_absolute`, `get_temp_path`, and `normalize` (`tests/solixlib/test_filesystem.cpp`).
- [x] **Case 12.2 [IMPLEMENTED]**: `File` text I/O: writing, existence check, reading all text, appending text, and size retrieval (`tests/solixlib/test_filesystem.cpp`).
- [x] **Case 12.3 [IMPLEMENTED]**: `File` line reading: `read_all_lines` splitting text by line separators into a `List` (`tests/solixlib/test_filesystem.cpp`).
- [x] **Case 12.4 [IMPLEMENTED]**: `File` operations: copying, moving, and deleting files (`tests/solixlib/test_filesystem.cpp`).
- [x] **Case 12.5 [IMPLEMENTED]**: `Directory` operations: creation, existence, listing files, listing directories, and recursive/non-recursive deletion (`tests/solixlib/test_filesystem.cpp`).
- [x] **Case 12.6 [IMPLEMENTED]**: Negative: `File.read_all_text()` on non-existent file throws `FileNotFoundException` (`tests/solixlib/test_filesystem.cpp`).
- [x] **Case 12.7 [IMPLEMENTED]**: Negative: `File.delete()` on non-existent file throws `FileNotFoundException` (`tests/solixlib/test_filesystem.cpp`).
- [x] **Case 12.8 [IMPLEMENTED]**: Negative: `Directory.delete()` on non-empty directory without recursive flag throws `IOException` (`tests/solixlib/test_filesystem.cpp`).
- [x] **Case 12.9 [IMPLEMENTED]**: Negative: `Directory.list_files()` on non-existent directory throws `DirectoryNotFoundException` (`tests/solixlib/test_filesystem.cpp`).
