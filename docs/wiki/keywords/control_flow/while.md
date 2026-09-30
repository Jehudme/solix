# `while`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `while` |
| Category | Control Flow |
| Context | Statement position inside method or constructor bodies |
| Related | [`do`](do.md), [`for`](for.md), [`break`](break.md), [`continue`](continue.md) |

`while` begins a pre-condition loop. The condition expression is evaluated before each iteration; if it is `true` the body executes, then the condition is tested again. If the condition is `false` on the first evaluation, the body is never executed. `while` is the idiomatic choice when the number of iterations is not known in advance.

## 2. Permitted Contexts (Syntax & Grammar)

```
WhileStatement
    : 'while' '(' Expression ')' Block
    ;
```

- The condition must be parenthesised and must be of type `bool`.
- The body must be a braced block `{ }`.

## 3. Semantics & Compiler Rules

- The condition is re-evaluated at the start of every iteration.
- A `break` statement inside the body exits the loop immediately; a `continue` skips to the next condition check.
- If the condition is the literal `true`, the compiler emits **W0310** (`infinite loop detected`) unless the body contains a `break` or `throw` on every path.
- Variables declared inside the loop body are scoped to each iteration and are re-initialised on each pass.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;

public class Counter {
    public static void countDown(int32 from) {
        while (from > 0) {
            Console.println(from);
            from = from - 1;
        }
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;
import solix.io.BufferedReader;

public class LineProcessor {
    public static int32 countNonEmpty(BufferedReader reader) {
        int32 count = 0;
        string line = reader.readLine();
        while (line != null) {
            if (line.length() > 0) {
                count = count + 1;
            }
            line = reader.readLine();
        }
        return count;
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Non-`bool` condition | **E0201** `condition must be of type 'bool', found '<type>'` |
| Omitting braces around body | **E0202** `expected '{' to begin 'while' body` |
| Infinite loop with no exit | **W0310** `loop may never terminate; no reachable 'break' or 'throw'` |
| Variable used after loop not definitely initialised | **E0120** `variable 'x' may not be initialised on all paths` |

## 6. Related Keywords & Guides

- [`do`](do.md) — post-condition variant; body always runs at least once
- [`for`](for.md) — counted loop with explicit init and step
- [`break`](break.md) — exits the loop early
- [`continue`](continue.md) — skips to the next iteration
