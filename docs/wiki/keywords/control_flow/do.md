# `do`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `do` |
| Category | Control Flow |
| Context | Statement position inside method or constructor bodies |
| Related | [`while`](while.md), [`break`](break.md), [`continue`](continue.md) |

`do` begins a post-condition loop. Unlike `while`, the body of a `do` loop executes unconditionally on the first iteration; the condition is evaluated only after each execution of the body. This guarantees that the body runs at least once regardless of the condition, making `do` the natural choice for read-parse loops and user-input validation.

## 2. Permitted Contexts (Syntax & Grammar)

```
DoStatement
    : 'do' Block 'while' '(' Expression ')' ';'
    ;
```

- The body block appears before the `while` clause.
- The trailing semicolon after the closing `)` is mandatory.
- The condition must be parenthesised and of type `bool`.

## 3. Semantics & Compiler Rules

- The body is always executed at least once.
- After the body, the condition is evaluated; if `true`, the body runs again.
- `break` exits the loop; `continue` skips the remainder of the body and proceeds to condition evaluation.
- A constant `true` condition triggers **W0310** (`infinite loop detected`) unless the body always exits via `break` or `throw`.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;

public class RepeatDemo {
    public static int32 sumUntilZero(int32[] values) {
        int32 i = 0;
        int32 sum = 0;
        do {
            sum = sum + values[i];
            i = i + 1;
        } while (i < values.length && values[i - 1] != 0);
        return sum;
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;
import solix.io.Terminal;

public class InputValidator {
    public static int32 readPositive(Terminal term) {
        int32 value;
        do {
            Console.println("Enter a positive integer: ");
            value = term.readInt();
            if (value <= 0) {
                Console.println("Invalid input, please try again.");
            }
        } while (value <= 0);
        return value;
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Missing semicolon after `while (cond)` | **E0220** `expected ';' after 'do-while' condition` |
| Non-`bool` condition | **E0201** `condition must be of type 'bool', found '<type>'` |
| Omitting braces around body | **E0202** `expected '{' to begin 'do' body` |
| Writing `do while` without a condition | **E0221** `expected '(' after 'while' in 'do-while'` |

## 6. Related Keywords & Guides

- [`while`](while.md) — pre-condition loop; body may not execute at all
- [`for`](for.md) — counted loop
- [`break`](break.md) — exits the loop
- [`continue`](continue.md) — skips to condition evaluation
