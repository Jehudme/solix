# `continue`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `continue` |
| Category | Control Flow |
| Context | Inside `while`, `do-while`, or `for` loops |
| Related | [`break`](break.md), [`while`](while.md), [`do`](do.md), [`for`](for.md) |

`continue` skips the remainder of the current loop iteration's body and transfers control back to the loop's continuation point — the condition check for `while` and `do-while`, or the step expression for `for`. It is useful for skipping elements that do not meet a processing criterion without introducing deeply nested `if` blocks, making loop bodies flatter and easier to read.

## 2. Permitted Contexts (Syntax & Grammar)

```
ContinueStatement
    : 'continue' ';'
    ;
```

- `continue` is only valid inside `while`, `do-while`, and `for` loops. It is not valid inside a `switch` body unless that `switch` is itself inside a loop.
- `continue` takes no label or target; it always applies to the immediately enclosing loop.

## 3. Semantics & Compiler Rules

- In a `while` or `do-while` loop, `continue` transfers control to the condition expression.
- In a `for` loop, `continue` transfers control to the `ForStep` expression, which executes before the condition is re-evaluated.
- Statements following `continue` in the same block are unreachable and generate **W0300**.
- `continue` inside a `switch` that is nested within a loop targets the enclosing loop, not the `switch`.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;

public class EvenPrinter {
    public static void printEvens(int32[] values) {
        for (int32 i = 0; i < values.length; i = i + 1) {
            if (values[i] % 2 != 0) {
                continue;
            }
            Console.println(values[i]);
        }
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;

public class DataFilter {
    public static float64 averagePositive(float64[] data) {
        float64 sum = 0.0;
        int32 count = 0;
        for (int32 i = 0; i < data.length; i = i + 1) {
            if (data[i] <= 0.0) {
                continue;
            }
            sum = sum + data[i];
            count = count + 1;
        }
        if (count == 0) {
            return 0.0;
        }
        return sum / count;
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| `continue` outside a loop | **E0251** `'continue' must be inside a loop` |
| Code after `continue` in the same block | **W0300** `unreachable code after 'continue'` |
| Using `continue` inside `switch` expecting to skip the switch | *(no error; targets the enclosing loop, a potential logic bug)* |

## 6. Related Keywords & Guides

- [`break`](break.md) — exits the loop entirely
- [`while`](while.md), [`do`](do.md), [`for`](for.md) — the loop constructs that `continue` applies to
