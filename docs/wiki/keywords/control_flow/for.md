# `for`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `for` |
| Category | Control Flow |
| Context | Statement position inside method or constructor bodies |
| Related | [`while`](while.md), [`do`](do.md), [`break`](break.md), [`continue`](continue.md) |

`for` is the classical three-part counted loop. It combines an initialiser, a condition, and a step expression into a single compact header, making it the preferred form for index-driven iteration over arrays or numeric ranges. All three parts are optional; omitting the condition produces an infinite loop.

## 2. Permitted Contexts (Syntax & Grammar)

```
ForStatement
    : 'for' '(' ForInit? ';' Expression? ';' ForStep? ')' Block
    ;

ForInit
    : LocalVariableDeclaration
    | ExpressionList
    ;

ForStep
    : ExpressionList
    ;
```

- Any or all of `ForInit`, `Expression` (condition), and `ForStep` may be omitted; the two semicolons are always required.
- A variable declared in `ForInit` is scoped to the loop header and body only.
- Multiple expressions in `ForStep` are separated by commas.

## 3. Semantics & Compiler Rules

1. `ForInit` executes once before the loop begins.
2. The condition (if present) is evaluated before each iteration; if `false`, the loop exits.
3. The body executes.
4. `ForStep` executes after the body, then control returns to step 2.
- An omitted condition is treated as `true`.
- `break` exits the loop; `continue` skips the body and proceeds to `ForStep`.
- Variables declared in `ForInit` shadow any outer variable of the same name within the loop scope.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;

public class ArrayPrinter {
    public static void printAll(int32[] items) {
        for (int32 i = 0; i < items.length; i = i + 1) {
            Console.println(items[i]);
        }
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;

public class MatrixOps {
    public static int32[][] transpose(int32[][] matrix, int32 rows, int32 cols) {
        int32[][] result = new int32[cols][rows];
        for (int32 r = 0; r < rows; r = r + 1) {
            for (int32 c = 0; c < cols; c = c + 1) {
                result[c][r] = matrix[r][c];
            }
        }
        return result;
    }

    public static int32 sumDiagonal(int32[][] matrix, int32 size) {
        int32 sum = 0;
        for (int32 i = 0; i < size; i = i + 1) {
            sum = sum + matrix[i][i];
        }
        return sum;
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Missing first or second semicolon | **E0230** `expected ';' in 'for' header` |
| Non-`bool` condition expression | **E0201** `condition must be of type 'bool', found '<type>'` |
| Referencing the loop variable after the loop | **E0121** `'i' is not in scope here; it was declared inside the 'for' header` |
| Omitting braces around body | **E0202** `expected '{' to begin 'for' body` |

## 6. Related Keywords & Guides

- [`while`](while.md) — pre-condition loop, preferred when step logic is complex
- [`do`](do.md) — post-condition loop
- [`break`](break.md) — exits the loop
- [`continue`](continue.md) — skips to `ForStep`
