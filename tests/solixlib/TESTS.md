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

- [ ] **Case 2.1 [NOT IMPLEMENTED]**: `Console.print()` and `Console.println()` with primitive integers, booleans, doubles, and characters.
- [ ] **Case 2.2 [NOT IMPLEMENTED]**: `Console.print()` and `Console.println()` with primitive character arrays (`char[]`).
- [ ] **Case 2.3 [NOT IMPLEMENTED]**: `Console.error()` outputting in ANSI red (`\033[31m`) to standard error stream.
- [ ] **Case 2.4 [NOT IMPLEMENTED]**: `Console.warning()` outputting in ANSI yellow (`\033[33m`) to standard stream.
- [ ] **Case 2.5 [NOT IMPLEMENTED]**: `Console.input_int()` parsing valid integer from stdin.
- [ ] **Case 2.6 [NOT IMPLEMENTED]**: `Console.input_double()` parsing valid double from stdin.
- [ ] **Case 2.7 [NOT IMPLEMENTED]**: `Console.input_bool()` parsing boolean (`true`/`false`) from stdin.
- [ ] **Case 2.8 [NOT IMPLEMENTED]**: `Console.input_char()` reading single character from stdin.
- [ ] **Case 2.9 [NOT IMPLEMENTED]**: `Console.input_chars()` reading line into `char[]` buffer.
- [ ] **Case 2.10 [NOT IMPLEMENTED]**: Negative: `Console.input_int()` with non-numeric input throws `FormatException`.
- [ ] **Case 2.11 [NOT IMPLEMENTED]**: Negative: `Console.input_double()` with invalid text throws `FormatException`.
- [ ] **Case 2.12 [NOT IMPLEMENTED]**: Post-String upgrade: `Console.print()` and `Console.println()` with `String` and `IStringable` instances.
- [ ] **Case 2.13 [NOT IMPLEMENTED]**: Post-String upgrade: `Console.input() -> String` reading complete line.
