# `Objects`

## 1. Class Overview

`solix.core.Objects` provides static utility methods for performing null-safe operations, equality testing, hash code computations, and mandatory reference validation. It acts as the foundational building block for generic containers and algorithms throughout the standard library.

- **Package**: `solix.core`
- **Import**: `import solix.core.Objects;` or `import solix.core.*;`

---

## 2. Method Reference

### Null Checks & Validation

| Method Signature | Description |
|------------------|-------------|
| `public static bool is_null<T>(T item)` | Returns `true` if `item == null`, otherwise `false`. |
| `public static bool non_null<T>(T item)` | Returns `true` if `item != null`, otherwise `false`. |
| `public static T require_non_null<T>(T item)` | Returns `item` if non-null; throws `NullPointerException` otherwise. |
| `public static T require_non_null_message<T>(T item, String message)` | Returns `item` if non-null; throws `NullPointerException` with custom error message. |

### Generic Equality & Hashing

| Method Signature | Description |
|------------------|-------------|
| `public static bool equals<T>(T first, T second)` | Null-safe equality comparison. Returns `true` if identical or `first.equals(second)`. |
| `public static int32 hash_code<T>(T item)` | Null-safe hash computation. Returns `0` if `item == null`, otherwise `item.hash_code()`. |

### Primitive Overloads

| Method Signature | Description |
|------------------|-------------|
| `public static int32 hash_code(int32 val)` | Returns `val` as hash code. |
| `public static int32 hash_code(int64 val)` | Casts 64-bit integer to 32-bit hash code. |
| `public static int32 hash_code(char val)` | Returns ASCII/numeric codepoint. |
| `public static int32 hash_code(bool val)` | Returns `1` for true, `0` for false. |
| `public static int32 hash_code(float64 val)` | Casts float to 32-bit integer hash code. |
| `public static bool equals(int32 a, int32 b)` | Tests primitive integer equality. |
| `public static bool equals(int64 a, int64 b)` | Tests primitive 64-bit integer equality. |
| `public static bool equals(char a, char b)` | Tests character equality. |
| `public static bool equals(bool a, bool b)` | Tests boolean equality. |
| `public static bool equals(float64 a, float64 b)` | Tests float equality. |

---

## 3. Code Examples

```solix
package solix.example;

import solix.core.Objects;
import solix.core.String;

public class UserSession {
    private String username;

    public UserSession(String username) {
        // Enforce non-null invariant at construction time
        this.username = Objects.require_non_null_message(username, "Username must not be null");
    }

    public bool equals(UserSession other) {
        if (other == null) { return false; }
        return Objects.equals(this.username, other.username);
    }

    public int32 hash_code() {
        return Objects.hash_code(this.username);
    }
}
```
