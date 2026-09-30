# `inline`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `inline` |
| Category | Modifier |
| Context | Before method declarations |
| Related | [`static`](static.md), [`native`](native.md) |

`inline` is a compiler hint that requests the compiler to replace a call site with a copy of the method body, eliminating the overhead of a function call. It does not change the observable semantics of a program. The compiler may ignore the hint if the method body is too large, recursive, or otherwise unsuitable for inlining. `inline` is most valuable on small, frequently called methods such as trivial accessors, arithmetic helpers, and forwarding wrappers.

## 2. Permitted Contexts (Syntax & Grammar)

```
InlineMethod
    : Modifier* 'inline' ReturnType Identifier '(' ParameterList? ')' Block
    ;
```

- `inline` may only be applied to methods with a body; it cannot be applied to `abstract` or `native` methods.
- `inline` may be combined with access modifiers and with `static`.
- `inline` and `virtual` may be combined; the compiler inlines the call when the static type is known at compile time, but preserves virtual dispatch when it is not.

## 3. Semantics & Compiler Rules

- The compiler is free to honour or disregard the `inline` hint based on its cost model.
- `inline` methods must have a body; applying it to an abstract or native method raises **E0550**.
- Recursive `inline` methods are permitted; the compiler will inline to a configurable depth and fall back to a regular call thereafter.
- **W0313** is issued when the compiler determines that an `inline` method cannot be inlined (e.g., because the body is too large).
- Inlining does not affect type safety, visibility, or override semantics.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

public class Vector2 {
    public float64 x;
    public float64 y;

    public Vector2(float64 x, float64 y) {
        this.x = x;
        this.y = y;
    }

    public inline float64 length() {
        return solix.math.Math.sqrt(x * x + y * y);
    }

    public inline bool isZero() {
        return x == 0.0 && y == 0.0;
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

public class ByteUtils {
    public static inline int32 clamp(int32 value, int32 min, int32 max) {
        if (value < min) { return min; }
        if (value > max) { return max; }
        return value;
    }

    public static inline int32 lerp(int32 a, int32 b, float64 t) {
        return a + (int32)((b - a) * t);
    }

    public static inline bool inRange(int32 value, int32 lo, int32 hi) {
        return value >= lo && value <= hi;
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Applying `inline` to an `abstract` method | **E0550** `'inline' cannot be combined with 'abstract'` |
| Applying `inline` to a `native` method | **E0551** `'inline' cannot be combined with 'native'` |
| Method body too large to inline | **W0313** `'inline' hint ignored for 'bigMethod()': body exceeds inlining threshold` |

## 6. Related Keywords & Guides

- [`static`](static.md) — often combined with `inline` for utility methods
- [`native`](native.md) — mutually exclusive with `inline`
- [`abstract`](abstract.md) — mutually exclusive with `inline`
