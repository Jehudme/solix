# `break`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `break` |
| Category | Control Flow |
| Context | Inside `while`, `do-while`, `for`, or `switch` statements |
| Related | [`continue`](continue.md), [`switch`](switch.md), [`while`](while.md), [`for`](for.md), [`do`](do.md) |

`break` immediately exits the nearest enclosing `while`, `do-while`, `for`, or `switch` statement. Control transfers to the first statement that follows the terminated construct. `break` is mandatory at the end of every non-terminating `case` and `default` block in a `switch`, and is commonly used to exit loops early upon satisfying a search condition.

## 2. Permitted Contexts (Syntax & Grammar)

```
BreakStatement
    : 'break' ';'
    ;
```

- `break` must appear inside a loop (`while`, `do`, `for`) or a `switch` statement. Using it elsewhere is a compile error.
- `break` takes no label or target operand; it always exits the immediately enclosing construct.

## 3. Semantics & Compiler Rules

- Execution of `break` is unconditional. Statements after `break` within the same block are unreachable and trigger **W0300** (`unreachable code`).
- In nested constructs, `break` exits only the directly enclosing loop or `switch`, not any outer ones.
- A `break` inside a `switch` that is itself inside a loop exits the `switch`, not the loop.
- The compiler requires `break` (or `return`/`throw`) at the end of each `case`/`default` block; absence raises **E0240**.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;

public class Search {
    public static int32 findFirst(int32[] arr, int32 target) {
        int32 index = -1;
        for (int32 i = 0; i < arr.length; i = i + 1) {
            if (arr[i] == target) {
                index = i;
                break;
            }
        }
        return index;
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;

public class PrimeChecker {
    public static bool isPrime(int32 n) {
        if (n < 2) { return false; }
        bool prime = true;
        for (int32 i = 2; i * i <= n; i = i + 1) {
            if (n % i == 0) {
                prime = false;
                break;
            }
        }
        return prime;
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| `break` outside any loop or `switch` | **E0250** `'break' must be inside a loop or 'switch' statement` |
| Code placed after `break` in the same block | **W0300** `unreachable code after 'break'` |
| Expecting `break` to exit an outer nested loop | *(no error; subtle logic bug)* |

## 6. Related Keywords & Guides

- [`continue`](continue.md) — skips the rest of the iteration without exiting the loop
- [`switch`](switch.md) — requires `break` to terminate each case block
- [`while`](while.md), [`for`](for.md), [`do`](do.md) — loop constructs `break` can exit
