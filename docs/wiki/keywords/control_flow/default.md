# `default`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `default` |
| Category | Control Flow |
| Context | Inside a `switch` block only |
| Related | [`switch`](switch.md), [`case`](case.md), [`break`](break.md) |

`default` labels the fallback branch of a `switch` statement. When the `switch` discriminant does not match any `case` constant, execution transfers to the `default` block. Although `default` may appear anywhere within the `switch` body, it is conventional to place it last. A `switch` statement may contain at most one `default` label.

## 2. Permitted Contexts (Syntax & Grammar)

```
DefaultClause
    : 'default' ':' Statement*
    ;
```

- `default` is only valid inside a `switch` block.
- The colon is mandatory; there is no expression after `default`.
- Like `case`, the `default` block must be terminated by `break`, `return`, or `throw`.
- When switching on an `enum` type, omitting `default` and not covering all enum members triggers **W0302** (`non-exhaustive switch`).

## 3. Semantics & Compiler Rules

- At most one `default` label per `switch` is allowed; duplicates are a compile error **E0243**.
- The compiler checks exhaustiveness when the switch discriminant is an enum type. If all enum members are covered by explicit `case` labels, `default` is optional but still permitted.
- `default` does not fall through from preceding `case` blocks. Each block is independent.
- If `default` is not the last clause and no `break`/`return`/`throw` is present, **E0240** is issued.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;

public class HttpStatus {
    public static string describe(int32 code) {
        switch (code) {
            case 200: return "OK";
            case 301: return "Moved Permanently";
            case 404: return "Not Found";
            case 500: return "Internal Server Error";
            default:  return "Unknown Status";
        }
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;

public enum LogLevel { DEBUG, INFO, WARN, ERROR, FATAL }

public class Logger {
    public static string prefix(LogLevel level) {
        switch (level) {
            case LogLevel.DEBUG: return "[DBG]";
            case LogLevel.INFO:  return "[INF]";
            case LogLevel.WARN:  return "[WRN]";
            case LogLevel.ERROR: return "[ERR]";
            case LogLevel.FATAL: return "[FTL]";
            default:             return "[???]";
        }
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| More than one `default` in a `switch` | **E0243** `switch statement may have at most one 'default' label` |
| `default` outside a `switch` | **E0244** `'default' label not inside a 'switch' statement` |
| Missing `break`/`return`/`throw` | **E0240** `'default' block must end with 'break', 'return', or 'throw'` |
| Enum switch without `default` and missing members | **W0302** `switch on enum 'LogLevel' does not handle: FATAL` |

## 6. Related Keywords & Guides

- [`switch`](switch.md) — the enclosing statement
- [`case`](case.md) — specific value-labelled branches
- [`break`](break.md) — exits the `default` block
