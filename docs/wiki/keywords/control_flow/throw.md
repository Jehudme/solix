# `throw`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `throw` |
| Category | Control Flow |
| Context | Statement position inside any method or constructor body |
| Related | [`try`](try.md), [`catch`](catch.md), [`finally`](finally.md) |

`throw` raises an exception, immediately halting normal control flow and transferring execution to the nearest enclosing `catch` clause whose declared type is compatible with the thrown object. If no compatible `catch` is found in the current method, the exception propagates up the call stack. When used as a bare `throw;` inside a `catch` block, it re-throws the currently active exception, preserving the original stack trace.

## 2. Permitted Contexts (Syntax & Grammar)

```
ThrowStatement
    : 'throw' Expression ';'   // throw a new or existing exception
    | 'throw' ';'              // re-throw; only valid inside a catch block
    ;
```

- The expression after `throw` must evaluate to an object whose type is `Exception` or a subclass.
- Bare `throw;` is only valid inside a `catch` block; using it elsewhere is a compile error.

## 3. Semantics & Compiler Rules

- `throw` is a statement with no value; it terminates the current method path. The compiler counts it as a definite exit for return-analysis purposes.
- Statements placed after an unconditional `throw` in the same block are unreachable and trigger **W0300**.
- Throwing `null` results in a runtime `NullPointerException`. The compiler issues **W0305** when the thrown expression is provably `null`.
- A `throw` inside a `try` block causes `finally` to run before the exception propagates.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.core.IllegalArgumentException;

public class Validator {
    public static void requirePositive(int32 value, string name) {
        if (value <= 0) {
            throw new IllegalArgumentException(name + " must be positive, got " + value);
        }
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;
import solix.core.Exception;
import solix.core.IllegalStateException;
import solix.io.FileReader;
import solix.io.IOException;

public class ConfigLoader {
    private bool loaded = false;
    private string configPath;

    public ConfigLoader(string path) {
        this.configPath = path;
    }

    public string load() {
        if (loaded) {
            throw new IllegalStateException("Config already loaded");
        }
        FileReader reader = null;
        try {
            reader = new FileReader(configPath);
            loaded = true;
            return reader.readAll();
        } catch (IOException e) {
            Console.println("Failed to read config: " + e.getMessage());
            throw;  // re-throw with original stack trace
        } finally {
            if (reader != null) {
                reader.close();
            }
        }
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Throwing a non-`Exception` object | **E0280** `thrown expression must be of type 'Exception' or a subclass` |
| Bare `throw;` outside a `catch` block | **E0281** `bare 'throw' is only permitted inside a 'catch' block` |
| Throwing `null` | **W0305** `thrown expression is always 'null'; this will cause a NullPointerException at runtime` |
| Code after `throw` in the same block | **W0300** `unreachable code after 'throw'` |

## 6. Related Keywords & Guides

- [`try`](try.md) — the guarded block that wraps code which might throw
- [`catch`](catch.md) — intercepts thrown exceptions
- [`finally`](finally.md) — runs after `throw` propagates
