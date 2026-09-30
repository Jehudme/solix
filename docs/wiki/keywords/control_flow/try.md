# `try`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `try` |
| Category | Control Flow |
| Context | Statement position inside method or constructor bodies |
| Related | [`catch`](catch.md), [`finally`](finally.md), [`throw`](throw.md) |

`try` begins a guarded block in which exceptions may be caught and handled. Any code that could raise an exception should be placed inside a `try` block. A `try` block must be followed by at least one `catch` clause, a `finally` clause, or both. If an exception is thrown inside the `try` block, the runtime searches the `catch` clauses in order for the first one whose declared type is compatible with the thrown exception.

## 2. Permitted Contexts (Syntax & Grammar)

```
TryStatement
    : 'try' Block ( CatchClause+ FinallyClause? | FinallyClause )
    ;

CatchClause
    : 'catch' '(' TypeName Identifier ')' Block
    ;

FinallyClause
    : 'finally' Block
    ;
```

- A `try` block must be followed by at least one `catch` or a `finally` (or both).
- `catch` clauses are checked in declaration order; only the first matching clause runs.
- The `finally` clause, if present, always runs whether or not an exception was thrown.

## 3. Semantics & Compiler Rules

- If no exception is thrown, `catch` clauses are skipped entirely; `finally` still runs.
- If an exception is thrown and a `catch` matches, that handler runs; `finally` still runs afterward.
- If an exception is thrown but no `catch` matches, the exception propagates up the call stack; `finally` still runs.
- A `return` or `break` inside a `try` block does not bypass `finally`.
- Re-throwing inside `catch` is done with `throw;` (bare throw, see [`throw`](throw.md)).
- Checked exceptions are not a feature of Solix; all exceptions are unchecked.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;
import solix.core.Exception;

public class SafeDivide {
    public static int32 divide(int32 a, int32 b) {
        try {
            return a / b;
        } catch (Exception e) {
            Console.println("Division failed: " + e.getMessage());
            return 0;
        }
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;
import solix.io.FileReader;
import solix.io.IOException;
import solix.core.Exception;

public class FileSummary {
    public static string readFirstLine(string path) {
        FileReader reader = null;
        try {
            reader = new FileReader(path);
            return reader.readLine();
        } catch (IOException e) {
            Console.println("I/O error: " + e.getMessage());
            return "";
        } catch (Exception e) {
            Console.println("Unexpected error: " + e.getMessage());
            return "";
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
| `try` block with neither `catch` nor `finally` | **E0270** `'try' block must be followed by 'catch' or 'finally'` |
| Catching a more general type before a more specific one | **W0303** `'catch (Exception)' shadows earlier handler for 'IOException'` |
| Variable declared in `try` used in `catch` or `finally` | **E0122** `'reader' may not be in scope here; it was declared inside 'try'` |

## 6. Related Keywords & Guides

- [`catch`](catch.md) — handles specific exception types
- [`finally`](finally.md) — cleanup block that always executes
- [`throw`](throw.md) — raises an exception
