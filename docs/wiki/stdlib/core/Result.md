# `Result<TValue, TError>` — API Reference

**Package:** `solix.core`  
**Import:** `import solix.core.Result;`

---

## Overview

`Result<TValue, TError>` is a generic type that represents either a **successful** outcome carrying a value of type `TValue`, or an **error** outcome carrying a value of type `TError`. It is an alternative to exception-based error handling for operations where failure is routine and expected.

Using `Result` makes the possibility of failure visible in method signatures and forces callers to check the outcome explicitly.

> [!TIP]
> Use `Result` when a method can fail in ways that are expected as part of normal program flow (e.g., parsing user input). Use `throw` for truly exceptional conditions (programming errors, invariant violations).

---

## Type Parameters

| Parameter | Description |
|-----------|-------------|
| `TValue` | The type of the success value. |
| `TError` | The type of the error value. Often a `String` or a custom error class. |

---

## Constructors

| Signature | Description |
|-----------|-------------|
| `Result()` | Creates a failed (Err) Result with no values. |
| `Result(TValue value, TError error, bool is_success)` | Low-level constructor; used by factory methods. |

---

## Static Factory Methods

| Signature | Returns | Description |
|-----------|---------|-------------|
| `Result.ok(TValue value)` | `Result<TValue, TError>` | Creates a successful Result containing `value`. The error field is `null`. |
| `Result.err(TError error)` | `Result<TValue, TError>` | Creates a failed Result containing `error`. The value field is `null`. |

---

## State Queries

| Signature | Returns | Description |
|-----------|---------|-------------|
| `is_ok()` | `bool` | `true` if the Result holds a success value. |
| `is_err()` | `bool` | `true` if the Result holds an error. Equivalent to `!is_ok()`. |

---

## Value Retrieval

| Signature | Returns | Description | Throws |
|-----------|---------|-------------|--------|
| `unwrap()` | `TValue` | Returns the success value. | `IllegalStateException` if called on an Err Result |
| `unwrap_err()` | `TError` | Returns the error value. | `IllegalStateException` if called on an Ok Result |
| `unwrap_or(TValue fallback)` | `TValue` | Returns the success value if present, otherwise returns `fallback`. | — |

---

## String Conversion

| Signature | Returns | Description |
|-----------|---------|-------------|
| `to_string()` | `String` | Returns `"Result.Ok"` or `"Result.Err"`. |

---

## Code Examples

### Basic Pattern

```solix
import solix.core.Result;
import solix.core.String;
import solix.systems.Console;

public Result<int32, String> parse_positive(String input) {
    int32 value = input.to_int32();
    if (value <= 0) {
        return Result.err(new String("Value must be positive."));
    }
    return Result.ok(value);
}

public void main(char[][] args) {
    Result<int32, String> r = parse_positive(new String("42"));
    if (r.is_ok()) {
        Console.println(r.unwrap());           // 42
    } else {
        Console.errorln(r.unwrap_err());
    }

    Result<int32, String> bad = parse_positive(new String("-5"));
    int32 safe = bad.unwrap_or(0);
    Console.println(safe);                     // 0
}
```

### Error Type as a Custom Class

```solix
public class ParseError {
    public String input;
    public String reason;

    public ParseError(String input, String reason) {
        this.input  = input;
        this.reason = reason;
    }
}

public Result<float64, ParseError> parse_price(String text) {
    if (text == null || text.is_empty()) {
        return Result.err(new ParseError(text, new String("Empty input")));
    }
    float64 value = text.to_float64();
    if (value < 0.0) {
        return Result.err(new ParseError(text, new String("Negative price")));
    }
    return Result.ok(value);
}
```

### Unwrap with Default

```solix
Result<int32, String> result = compute();
int32 value = result.unwrap_or(-1);   // -1 if compute() failed
```

> [!NOTE]
> `Result` does not have chaining operations like `map` or `and_then`. Apply transformations manually by checking `is_ok()` and calling `unwrap()`.
