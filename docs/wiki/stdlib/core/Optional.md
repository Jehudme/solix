# `Optional<T>` — API Reference

**Package:** `solix.core`  
**Import:** `import solix.core.Optional;`  
**Source:** [`Optional.slx`](../../../../launcher/rsc/lib/solix/core/Optional.slx)

---

## Overview

`Optional<T>` is a generic container that either holds a single non-null value of type `T`, or holds nothing (the *empty* state). It is the idiomatic way to express optionality without raw `null` references, making the absence of a value explicit in the type signature.

> [!TIP]
> Prefer `Optional<T>` over returning `null` from a method when the absent case is a legitimate and expected outcome. Reserve `null`-returning methods for internal or performance-critical code.

---

## Type Parameters

| Parameter | Description |
|-----------|-------------|
| `T` | The type of the contained value. Must be a reference type. |

---

## Constructors

| Signature | Description |
|-----------|-------------|
| `Optional()` | Creates an empty Optional (no value present). |
| `Optional(T value)` | Creates a present Optional containing `value`. Throws `NullPointerException` if `value` is `null`. |
| `Optional(T value, bool has_value)` | Low-level constructor; sets both stored value and presence flag directly. Used by factory methods. |

---

## Static Factory Methods

| Signature | Returns | Description | Throws |
|-----------|---------|-------------|--------|
| `Optional.of(T value)` | `Optional<T>` | Creates a present Optional. | `NullPointerException` if `value` is `null` |
| `Optional.of_nullable(T value)` | `Optional<T>` | Creates a present Optional if `value != null`, otherwise returns empty. | — |
| `Optional.empty()` | `Optional<T>` | Creates an empty Optional. | — |

---

## State Queries

| Signature | Returns | Description |
|-----------|---------|-------------|
| `is_present()` | `bool` | `true` if a value is present. |
| `is_empty()` | `bool` | `true` if no value is present. Equivalent to `!is_present()`. |

---

## Value Retrieval

| Signature | Returns | Description | Throws |
|-----------|---------|-------------|--------|
| `get()` | `T` | Returns the contained value. | `NoSuchElementException` if empty |
| `or_else(T fallback)` | `T` | Returns the value if present, otherwise returns `fallback`. | — |
| `or_default(T default_value)` | `T` | Alias for `or_else`. Returns `default_value` if empty. | — |

---

## Equality and String Conversion

| Signature | Returns | Description |
|-----------|---------|-------------|
| `equals(Optional<T> other)` | `bool` | Two Optionals are equal if both are empty, or both are present and their values are `==`. |
| `to_string()` | `String` | Returns `"Optional[present]"` or `"Optional.empty"`. |

---

## Code Examples

### Basic Usage

```solix
import solix.core.Optional;
import solix.core.String;
import solix.systems.Console;

public Optional<String> find_user(int32 id) {
    if (id == 1) {
        return Optional.of(new String("Alice"));
    }
    return Optional.empty();
}

public void main(char[][] args) {
    Optional<String> user = find_user(1);
    if (user.is_present()) {
        Console.println(user.get());   // Alice
    }

    Optional<String> missing = find_user(99);
    String name = missing.or_else(new String("Unknown"));
    Console.println(name);             // Unknown
}
```

### Nullable Input

```solix
import solix.core.Optional;
import solix.core.String;

public Optional<String> parse_name(char[] raw) {
    if (raw == null || raw.length == 0) {
        return Optional.empty();
    }
    return Optional.of(new String(raw));
}
```

### Safe Chaining Pattern

```solix
Optional<String> value = get_config_value("timeout");
int32 timeout = 30;   // default
if (value.is_present()) {
    timeout = value.get().to_int32();
}
```

> [!NOTE]
> `Optional<T>` does not support chaining transforms such as `map` or `flatMap`. Apply conditional logic manually using `is_present()` / `get()`.
