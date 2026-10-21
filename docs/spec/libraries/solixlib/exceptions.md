# Standard Library Specification: `solix.exceptions`

The `solix.exceptions` package provides the foundational, unified object-oriented exception hierarchy for the Solix programming language standard library (`solixlib`). All standard library submodules, runtime assertions, error propagation mechanisms, and user-defined exceptions build on this hierarchy.

All 21 standard exception types are consolidated and encapsulated within `solix/exceptions/Exceptions.slx`, allowing lightweight unified compilation and single-import usability (`import solix.exceptions.*;` or importing specific exception classes).

---

## 1. Class Inheritance Hierarchy

```text
Exception
├── RuntimeException
│   ├── IllegalArgumentException
│   │   ├── ArgumentNullException
│   │   └── ArgumentOutOfRangeException
│   ├── IndexOutOfBoundsException
│   ├── NullReferenceException
│   ├── ArithmeticException
│   │   ├── DivideByZeroException
│   │   └── OverflowException
│   ├── InvalidOperationException
│   ├── FormatException
│   ├── NotSupportedException
│   ├── TimeoutException
│   ├── NoSuchElementException
│   └── KeyNotFoundException
├── IOException
│   ├── FileNotFoundException
│   ├── DirectoryNotFoundException
│   └── SocketException
└── AssertionError
```

---

## 2. Base Types

### `solix.exceptions.Exception`
The root class for all recoverable and unrecoverable errors in Solix.

- **Fields**:
  - `public char[] message`: Explanatory message describing the error.
  - `public Exception cause`: The underlying inner exception that triggered this error, or `null`.
- **Constructors**:
  - `public Exception()`: Initializes with empty message and `null` cause.
  - `public Exception(char[] message)`: Initializes with the specified message.
  - `public Exception(char[] message, Exception cause)`: Initializes with message and chained inner cause.
- **Methods**:
  - `public char[] get_message()`: Returns the exception message.
  - `public Exception get_cause()`: Returns the chained inner exception, or `null`.
  - `public char[] to_string()`: Formats the exception as human-readable text.

---

## 3. Runtime Exceptions

### `solix.exceptions.RuntimeException`
Base class for unchecked runtime errors and programming bugs.

- **Constructors**:
  - `public RuntimeException()`
  - `public RuntimeException(char[] message)`
  - `public RuntimeException(char[] message, Exception cause)`

### `solix.exceptions.IllegalArgumentException`
Thrown when a method or function argument is invalid.
- Extends: `RuntimeException`

### `solix.exceptions.ArgumentNullException`
Thrown when an argument that must not be null is passed as `null`.
- Extends: `IllegalArgumentException`
- **Fields**: `public char[] param_name`
- **Methods**: `public char[] get_param_name()`

### `solix.exceptions.ArgumentOutOfRangeException`
Thrown when an argument value falls outside allowable bounds.
- Extends: `IllegalArgumentException`
- **Fields**: `public char[] param_name`
- **Methods**: `public char[] get_param_name()`

### `solix.exceptions.IndexOutOfBoundsException`
Thrown when indexing an array, list, string, or buffer beyond valid range.
- Extends: `RuntimeException`
- **Fields**:
  - `public int32 index`
  - `public int32 lower_bound`
  - `public int32 upper_bound`
- **Methods**:
  - `public int32 get_index()`
  - `public int32 get_lower_bound()`
  - `public int32 get_upper_bound()`

### `solix.exceptions.NullReferenceException`
Thrown when dereferencing or invoking a member on a null reference.
- Extends: `RuntimeException`

### `solix.exceptions.ArithmeticException`
Base class for arithmetic computation faults.
- Extends: `RuntimeException`

### `solix.exceptions.DivideByZeroException`
Thrown when integer or numeric division by zero occurs.
- Extends: `ArithmeticException`

### `solix.exceptions.OverflowException`
Thrown when numeric calculations exceed minimum or maximum value bounds.
- Extends: `ArithmeticException`

### `solix.exceptions.InvalidOperationException`
Thrown when an operation is invalid for the current state of an object (e.g. mutating a collection while iterating, reading from a closed stream).
- Extends: `RuntimeException`

### `solix.exceptions.FormatException`
Thrown when string-to-number or data format conversions fail to parse correctly.
- Extends: `RuntimeException`

### `solix.exceptions.NotSupportedException`
Thrown when an invoked method or capability is unsupported on this platform or architecture.
- Extends: `RuntimeException`

### `solix.exceptions.TimeoutException`
Thrown when an operation exceeds an allotted deadline or timeout.
- Extends: `RuntimeException`

### `solix.exceptions.NoSuchElementException`
Thrown when attempting to access an element from an empty collection or queue.
- Extends: `RuntimeException`

### `solix.exceptions.KeyNotFoundException`
Thrown when looking up a key that does not exist in an associative map.
- Extends: `RuntimeException`

---

## 4. I/O Exceptions

### `solix.exceptions.IOException`
Base class for filesystem, stream, and network I/O failures.
- Extends: `Exception`

### `solix.exceptions.FileNotFoundException`
Thrown when an attempt to open or access a file fails because the file does not exist.
- Extends: `IOException`
- **Fields**: `public char[] file_path`
- **Methods**: `public char[] get_file_path()`

### `solix.exceptions.DirectoryNotFoundException`
Thrown when a directory path does not exist.
- Extends: `IOException`
- **Fields**: `public char[] dir_path`
- **Methods**: `public char[] get_dir_path()`

### `solix.exceptions.SocketException`
Thrown when a network socket or connection error occurs.
- Extends: `IOException`
- **Fields**: `public int32 error_code`
- **Methods**: `public int32 get_error_code()`

---

## 5. Assertion Errors

### `solix.exceptions.AssertionError`
Thrown by language assertion expressions (`assert cond;`) when a verified condition fails.
- Extends: `Exception`

---

## 6. Catching and Polymorphism Semantics

1. Catch clauses in Solix evaluate polymorphic inheritance from top to bottom.
2. Catching a base type like `RuntimeException` catches all derived exceptions (`IllegalArgumentException`, `IndexOutOfBoundsException`, etc.).
3. Placing a more general catch block before a specific catch block results in a compilation error:
   ```solix
   try { ... }
   catch (Exception e) { ... }
   catch (IOException e) { ... } // Compiler error: Unreachable catch clause
   ```
4. `finally` blocks are guaranteed to execute during stack unwinding regardless of whether an exception was caught or rethrown.
