# `if`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `if` |
| Category | Control Flow |
| Context | Statement position inside method or constructor bodies |
| Related | [`else`](else.md), [`switch`](switch.md), [`while`](while.md) |

`if` is the fundamental conditional branching keyword in Solix. It evaluates a boolean expression and executes a block of code only when that expression is `true`. An optional `else` clause provides an alternative path executed when the condition is `false`. `if` statements may be chained with `else if` to express multi-way conditions without nesting.

## 2. Permitted Contexts (Syntax & Grammar)

```
IfStatement
    : 'if' '(' Expression ')' Block ( 'else' ( IfStatement | Block ) )?
    ;
```

- The condition expression must be enclosed in parentheses and must resolve to `bool`.
- The body must be a block delimited by `{` and `}`. Single-statement bodies without braces are **not** permitted.
- `else if` is formed by placing an `if` statement directly after `else`; it is not a separate keyword.

## 3. Semantics & Compiler Rules

- The condition expression is evaluated exactly once at runtime.
- The expression type must be `bool`. The compiler raises **E0201** (`non-boolean condition`) if the type is any other, including integer types — implicit conversion from integer to `bool` is not performed.
- Both the `then` block and the optional `else` block are type-checked independently.
- The compiler performs definite-assignment analysis across both branches to ensure variables used after the `if` statement are initialised in all reachable paths.
- Dead branches (conditions provably constant at compile time) produce a **W0301** (`unreachable branch`) warning.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;

public class TemperatureCheck {
    public void check(float64 temp) {
        if (temp > 100.0) {
            Console.println("Boiling!");
        }
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;

public class Classifier {
    public static string classify(int32 score) {
        if (score >= 90) {
            return "A";
        } else if (score >= 80) {
            return "B";
        } else if (score >= 70) {
            return "C";
        } else if (score >= 60) {
            return "D";
        } else {
            return "F";
        }
    }

    public static void main(string[] args) {
        Console.println(classify(85));  // prints "B"
        Console.println(classify(42));  // prints "F"
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Using a non-`bool` condition, e.g. `if (count)` | **E0201** `condition must be of type 'bool', found 'int32'` |
| Omitting braces around the body | **E0202** `expected '{' to begin 'if' body` |
| Variable used after `if` not initialised in all branches | **E0120** `variable 'x' may not be initialised on all paths` |
| Assigning inside condition, e.g. `if (x = getValue())` | **E0203** `assignment expression not permitted as condition; use '==' for comparison` |

## 6. Related Keywords & Guides

- [`else`](else.md) — alternative branch for `if`
- [`switch`](switch.md) — multi-way dispatch on a single expression
- [`while`](while.md) — conditional loop
- [`for`](for.md) — counted loop
