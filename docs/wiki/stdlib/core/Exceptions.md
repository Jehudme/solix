# `Exceptions` — API Reference

**Package:** `solix.core`  
**Import:** `import solix.core.Exceptions;` (or `import solix.core.*;`)

---

## Overview

The Solix exception system is built around a hierarchy of classes all rooted at `Exception`. Every exception carries a message, an optional cause (chained exception), and an optional integer error code. Exceptions are thrown with `throw` and caught with `try/catch/finally`.

---

## Exception Class Hierarchy

```
Exception
├── RuntimeException
│   ├── NullPointerException
│   ├── IndexOutOfBoundsException
│   ├── TypeCastException
│   ├── InvalidArgumentException
│   ├── IllegalStateException
│   ├── NoSuchElementException
│   ├── EmptyCollectionException
│   ├── KeyNotFoundException
│   ├── DuplicateKeyException
│   ├── UnsupportedOperationException
│   ├── FormatException
│   └── ArithmeticException
│       ├── DivideByZeroException
│       ├── OverflowException
│       └── UnderflowException
└── IOException
```

---

## Base Class: `Exception`

### Fields (protected)

| Field | Type | Description |
|-------|------|-------------|
| `detailed_message` | `String` | The human-readable error message. |
| `underlying_cause` | `Exception` | The exception that caused this one, or `null`. |
| `error_code` | `int32` | Application-defined error code (default 0). |

### Constructors

| Signature | Description |
|-----------|-------------|
| `Exception()` | Default message: `"An exception has occurred."` |
| `Exception(char[] message)` | Message from raw character array. |
| `Exception(String message)` | Message from a `String`. |
| `Exception(char[] message, Exception cause)` | Message + cause chaining. |
| `Exception(String message, Exception cause)` | Message + cause chaining. |
| `Exception(char[] message, int32 code)` | Message + error code. |
| `Exception(String message, int32 code)` | Message + error code. |
| `Exception(char[] message, Exception cause, int32 code)` | All three. |
| `Exception(String message, Exception cause, int32 code)` | All three. |

### Virtual Methods

| Signature | Returns | Description |
|-----------|---------|-------------|
| `get_message()` | `String` | Returns `detailed_message`. |
| `message()` | `char[]` | Returns `detailed_message` as a raw `char[]`, or `"Unknown Exception"` if null. |
| `get_cause()` | `Exception` | Returns `underlying_cause`. |
| `get_error_code()` | `int32` | Returns `error_code`. |
| `to_string()` | `String` | Returns `"Exception: <message>"` or `"Exception"`. |
| `print_stack_trace()` | `void` | Prints exception and cause chain to `stderr`. |

---

## `RuntimeException`

Extends `Exception`. Base class for all exceptions that represent programming errors recoverable at runtime.

Constructors mirror `Exception`: `()`, `(char[])`, `(String)`, `(char[], Exception)`, `(String, Exception)`.

`to_string()` returns `"RuntimeException: <message>"`.

---

## Standard Runtime Exceptions

| Class | Default Message | Typical Cause |
|-------|-----------------|---------------|
| `NullPointerException` | `"Null pointer dereference occurred."` | Dereferencing a `null` reference |
| `IndexOutOfBoundsException` | — | Array/string index out of range |
| `TypeCastException` | `"Invalid type cast attempted."` | Failed `instanceof`/cast check |
| `InvalidArgumentException` | `"An invalid argument was provided."` | Bad parameter value |
| `IllegalStateException` | `"Illegal state for this operation."` | Method called in wrong state |
| `NoSuchElementException` | `"The requested element does not exist."` | `Optional.get()` on empty; iterator exhausted |
| `EmptyCollectionException` | `"Operation on empty collection."` | `pop`, `peek` on empty collection |
| `KeyNotFoundException` | `"The specified key was not found."` | `HashMap.get()` with missing key |
| `DuplicateKeyException` | `"A duplicate key was detected."` | `put_if_absent` when key exists |
| `UnsupportedOperationException` | `"The requested operation is not supported."` | Calling an unsupported method |
| `FormatException` | `"Invalid format specification encountered."` | Parsing malformed input |

### `IndexOutOfBoundsException` Constructors

```solix
IndexOutOfBoundsException()
IndexOutOfBoundsException(char[] message)
IndexOutOfBoundsException(String message)
IndexOutOfBoundsException(int32 index, int32 size)  // Most common — auto-generates message
```

---

## Arithmetic Exceptions

| Class | Extends | Default Message |
|-------|---------|-----------------|
| `ArithmeticException` | `RuntimeException` | `"An arithmetic calculation error occurred."` |
| `DivideByZeroException` | `ArithmeticException` | `"Division by zero error."` |
| `OverflowException` | `ArithmeticException` | `"Arithmetic operation caused an overflow."` |
| `UnderflowException` | `ArithmeticException` | `"Arithmetic operation caused an underflow."` |

---

## `IOException`

Extends `Exception` (not `RuntimeException` — it is a checked-style exception).

| Signature | Description |
|-----------|-------------|
| `IOException()` | Default: `"An input/output error has occurred."` |
| `IOException(char[] message)` | From char[]. |
| `IOException(String message)` | From String. |
| `IOException(char[] message, Exception cause)` | With cause. |
| `IOException(String message, Exception cause)` | With cause. |

---

## Code Examples

### Throwing and Catching

```solix
import solix.core.Exceptions;
import solix.systems.Console;

public void safe_divide(int32 a, int32 b) {
    try {
        if (b == 0) {
            throw new DivideByZeroException("Divisor cannot be zero.");
        }
        Console.println(a / b);
    } catch (DivideByZeroException ex) {
        Console.errorln(ex.get_message());
    }
}
```

### Chained Exceptions

```solix
try {
    load_config();
} catch (IOException ex) {
    throw new RuntimeException("Application startup failed.", ex);
}
```

### Custom Exception

```solix
public class ValidationException extends RuntimeException {
    public ValidationException(String field, String reason) {
        this.detailed_message = new String("Validation failed for '")
            .concat(field)
            .concat(new String("': "))
            .concat(reason);
        this.underlying_cause = (Exception)null;
        this.error_code = 0;
    }

    public override String to_string() {
        return new String("ValidationException: ").concat(this.detailed_message);
    }
}
```
