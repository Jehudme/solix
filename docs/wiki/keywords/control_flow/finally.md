# `finally`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `finally` |
| Category | Control Flow |
| Context | After the last `catch` clause of a `try` statement, or directly after a `try` block |
| Related | [`try`](try.md), [`catch`](catch.md), [`throw`](throw.md), [`return`](return.md) |

`finally` defines a block that is guaranteed to execute after the `try` block and any matching `catch` clause, regardless of whether an exception was thrown, caught, or not thrown at all. It is the standard mechanism for releasing resources — closing files, network connections, or locks — because it runs even when control leaves the `try` block via `return`, `break`, or `continue`.

## 2. Permitted Contexts (Syntax & Grammar)

```
FinallyClause
    : 'finally' Block
    ;
```

- `finally` must appear at most once per `try` statement, after all `catch` clauses.
- A `try` may be followed by `finally` alone (without any `catch`), or by one or more `catch` clauses followed optionally by `finally`.
- `finally` may not appear without a preceding `try`.

## 3. Semantics & Compiler Rules

- The `finally` block always executes before control finally leaves the enclosing method, even if a `return`, `break`, or uncaught exception occurred in `try` or `catch`.
- If the `finally` block itself throws an exception, that new exception replaces any exception propagating from `try`/`catch`.
- If `finally` contains a `return`, it overrides any `return` value set in the `try` block. The compiler issues **W0304** (`return in finally`).
- Variables declared in `try` or `catch` are not in scope within `finally`; declare them before the `try` block.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;
import solix.core.Exception;
import solix.io.Connection;

public class DatabaseOp {
    public static void runQuery(Connection conn, string sql) {
        try {
            conn.execute(sql);
        } catch (Exception e) {
            Console.println("Query failed: " + e.getMessage());
        } finally {
            conn.close();
        }
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;
import solix.io.FileWriter;
import solix.io.IOException;

public class ReportWriter {
    public static bool writeReport(string path, string content) {
        FileWriter writer = null;
        bool success = false;
        try {
            writer = new FileWriter(path);
            writer.write(content);
            success = true;
        } catch (IOException e) {
            Console.println("Write failed: " + e.getMessage());
        } finally {
            if (writer != null) {
                writer.close();
            }
        }
        return success;
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| `finally` without a preceding `try` | **E0275** `'finally' without matching 'try'` |
| Duplicate `finally` on one `try` | **E0276** `a 'try' statement may have only one 'finally' block` |
| `return` inside `finally` overriding `try` return | **W0304** `'return' inside 'finally' will override the pending return value` |
| Accessing `try`-scoped variable in `finally` | **E0122** `variable not in scope in 'finally'; declare before 'try'` |

## 6. Related Keywords & Guides

- [`try`](try.md) — the guarded block
- [`catch`](catch.md) — exception handlers
- [`throw`](throw.md) — raises exceptions
- [`return`](return.md) — interacts with `finally` when used inside `try`
