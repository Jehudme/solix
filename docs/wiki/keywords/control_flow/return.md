# `return`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `return` |
| Category | Control Flow |
| Context | Inside any method or constructor body |
| Related | [`throw`](throw.md), [`break`](break.md) |

`return` exits the currently executing method and optionally passes a value back to the call site. In `void` methods, `return` may be used without an expression to exit early; the compiler inserts an implicit return at the end of a `void` method if one is not present. In non-`void` methods, every code path must end with a `return` (or `throw`) carrying an expression of the declared return type.

## 2. Permitted Contexts (Syntax & Grammar)

```
ReturnStatement
    : 'return' Expression? ';'
    ;
```

- A `return` with an expression is required in all non-`void` methods.
- A `return` without an expression is required (or implicit) in `void` methods; providing an expression in a `void` method is a compile error.
- Constructors do not have a return type; `return` in a constructor must be bare.

## 3. Semantics & Compiler Rules

- The expression type must be assignment-compatible with the method's declared return type. Implicit narrowing is prohibited; widening conversions are permitted.
- The compiler performs definite-return analysis. If a non-`void` method has a path that reaches the end of the body without a `return` or `throw`, **E0260** is issued.
- Statements following an unconditional `return` in the same block are unreachable and generate **W0300**.
- Returning from inside a `try` block still executes any `finally` block before control actually leaves the method.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

public class MathUtils {
    public static int32 absolute(int32 x) {
        if (x < 0) {
            return -x;
        }
        return x;
    }

    public static void printSign(int32 x) {
        if (x == 0) {
            return;  // early exit from void method
        }
        // further processing
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;

public class StringUtils {
    public static bool isPalindrome(string s) {
        if (s == null || s.length() == 0) {
            return true;
        }
        int32 lo = 0;
        int32 hi = s.length() - 1;
        while (lo < hi) {
            if (s.charAt(lo) != s.charAt(hi)) {
                return false;
            }
            lo = lo + 1;
            hi = hi - 1;
        }
        return true;
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Non-`void` method with a reachable path that does not return | **E0260** `missing return statement; method 'foo' must return 'int32'` |
| Returning a value from a `void` method | **E0261** `'void' method cannot return a value` |
| Return type mismatch | **E0262** `cannot convert 'float64' to 'int32' in return` |
| Code after `return` in the same block | **W0300** `unreachable code after 'return'` |

## 6. Related Keywords & Guides

- [`throw`](throw.md) — exits a method by raising an exception
- [`finally`](finally.md) — executes even after a `return` inside `try`
