# `catch`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `catch` |
| Category | Control Flow |
| Context | Immediately after a `try` block or another `catch` clause |
| Related | [`try`](try.md), [`finally`](finally.md), [`throw`](throw.md) |

`catch` introduces an exception handler that is executed when an exception of a compatible type is thrown within the preceding `try` block. Each `catch` clause declares a single parameter — the caught exception object — whose type determines which exceptions it handles. Multiple `catch` clauses may follow a single `try` block, and they are tested in textual order from first to last; only the first matching clause runs.

## 2. Permitted Contexts (Syntax & Grammar)

```
CatchClause
    : 'catch' '(' TypeName Identifier ')' Block
    ;
```

- `catch` must immediately follow a `try` block or another `catch` clause.
- The exception parameter type must be a class type (not a primitive).
- The identifier names the caught exception object; it is in scope only within the `catch` block.
- Multiple `catch` clauses on a single `try` are permitted.

## 3. Semantics & Compiler Rules

- The runtime tests each `catch` clause in order. The first clause whose declared type is the same as or a supertype of the thrown exception is selected.
- More specific types must precede more general types; the compiler issues **W0303** when a `catch` is shadowed by a preceding broader handler.
- After the `catch` block completes normally, execution continues after the entire `try-catch-finally` statement (not back into the `try`).
- The exception variable is read-only; reassigning it is a compile error.
- A bare `throw;` inside `catch` re-throws the original exception, preserving its stack trace.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;
import solix.core.Exception;

public class SafeParse {
    public static int32 parseIntSafe(string s) {
        try {
            return parseInt(s);
        } catch (Exception e) {
            Console.println("Parse error: " + e.getMessage());
            return -1;
        }
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;
import solix.net.NetworkException;
import solix.io.IOException;
import solix.core.Exception;
import solix.net.HttpClient;

public class DataFetcher {
    public static string fetch(string url) {
        HttpClient client = new HttpClient();
        try {
            return client.get(url);
        } catch (NetworkException e) {
            Console.println("Network failure: " + e.getMessage());
            return "";
        } catch (IOException e) {
            Console.println("I/O failure: " + e.getMessage());
            return "";
        } catch (Exception e) {
            Console.println("Unexpected error: " + e.getMessage());
            throw;  // re-throw unhandled exceptions
        }
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| `catch` without a preceding `try` | **E0271** `'catch' without matching 'try'` |
| Broad catch before specific catch | **W0303** `'catch (Exception)' makes subsequent handlers unreachable` |
| Using a primitive type as the exception parameter | **E0272** `catch parameter must be a class type, not 'int32'` |
| Reassigning the caught exception variable | **E0273** `caught exception variable 'e' is read-only` |

## 6. Related Keywords & Guides

- [`try`](try.md) — the guarded block that `catch` belongs to
- [`finally`](finally.md) — cleanup that runs regardless of exception
- [`throw`](throw.md) — raises exceptions; bare `throw` inside `catch` re-throws
