# `const`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `const` |
| Category | Modifier |
| Context | Field declarations inside a class; top-level class-level declarations |
| Related | [`static`](static.md), [`final`](../control_flow/return.md) |

`const` declares a compile-time constant field. The value of a `const` field must be a compile-time constant expression — a literal or a combination of literals and other `const` values. Constants are implicitly `static` (shared across all instances) and may not be reassigned after initialisation. They are inlined by the compiler wherever they are referenced, producing zero runtime memory overhead.

## 2. Permitted Contexts (Syntax & Grammar)

```
ConstField
    : Modifier* 'const' PrimitiveType Identifier '=' ConstantExpression ';'
    ;
```

- `const` may be applied to class fields only (not local variables).
- The initialiser is required and must be a compile-time constant expression.
- Permitted types for `const` fields: all primitive types (`int8`, `int16`, `int32`, `int64`, `float32`, `float64`, `bool`, `char`) and `string`.
- `const` is implicitly `static`; writing `static const` is redundant but accepted.

## 3. Semantics & Compiler Rules

- The compiler substitutes every reference to a `const` field with its literal value at compile time.
- Attempting to assign to a `const` field after initialisation raises **E0570** (`assignment to const`).
- The initialiser expression must be evaluable at compile time; any reference to runtime values (e.g., `new`, method calls) raises **E0571** (`const initialiser must be a compile-time constant`).
- `const` fields do not occupy instance or class storage; they are embedded in the bytecode.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

public class Config {
    public const int32  MAX_RETRIES  = 3;
    public const string DEFAULT_HOST = "localhost";
    public const int32  DEFAULT_PORT = 8080;
    public const float64 PI         = 3.14159265358979;
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;

public class NetworkConstants {
    public const int32 HTTP_OK               = 200;
    public const int32 HTTP_NOT_FOUND        = 404;
    public const int32 HTTP_INTERNAL_ERROR   = 500;
    public const int32 MAX_CONNECTIONS       = 256;
    public const int32 TIMEOUT_MS            = 5000;
}

public class HttpHandler {
    public static string statusText(int32 code) {
        switch (code) {
            case NetworkConstants.HTTP_OK:             return "OK";
            case NetworkConstants.HTTP_NOT_FOUND:      return "Not Found";
            case NetworkConstants.HTTP_INTERNAL_ERROR: return "Internal Server Error";
            default:                                   return "Unknown";
        }
    }

    public static void main(string[] args) {
        Console.println(statusText(NetworkConstants.HTTP_NOT_FOUND));
        Console.println("Max connections: " + NetworkConstants.MAX_CONNECTIONS);
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Assigning to a `const` field | **E0570** `cannot assign to 'const' field 'MAX_RETRIES'` |
| Non-constant initialiser | **E0571** `'const' initialiser for 'VALUE' must be a compile-time constant` |
| `const` local variable | **E0572** `'const' is not valid for local variable declarations; use a class field` |
| `const` on a reference type (non-string) | **E0573** `'const' may only be applied to primitive types and 'string'` |

## 6. Related Keywords & Guides

- [`static`](static.md) — `const` is implicitly `static`
- [`case`](../control_flow/case.md) — `const` fields may be used as `case` labels
