# `else`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `else` |
| Category | Control Flow |
| Context | Immediately after the closing `}` of an `if` block |
| Related | [`if`](if.md), [`switch`](switch.md) |

`else` introduces an alternative execution path that is taken when the condition of its associated `if` statement is `false`. It may directly precede another `if` keyword to form `else if` chains, allowing compact multi-way conditional logic without nesting additional `if` statements inside braces.

## 2. Permitted Contexts (Syntax & Grammar)

```
ElseClause
    : 'else' ( IfStatement | Block )
    ;
```

- `else` must immediately follow the closing `}` of an `if` block. No statements may appear between them.
- The body of `else` must be a braced block `{ }` or another `if` statement (for `else if`).
- An `else` without a preceding `if` is a syntax error.

## 3. Semantics & Compiler Rules

- Exactly one of the `if` body and the `else` body executes at runtime; never both.
- The compiler uses the presence (or absence) of an `else` clause in definite-assignment and definite-return analysis. A method with a non-`void` return type requires that both the `if` and `else` branches return or throw for the overall statement to count as a definite exit.
- Dangling `else` ambiguity is resolved by always associating `else` with the closest preceding `if`—the same rule as C, Java, and C#.
- **W0301** is issued when the `else` branch is provably unreachable due to a compile-time constant condition.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;

public class Toggle {
    public static string describe(bool flag) {
        if (flag) {
            return "enabled";
        } else {
            return "disabled";
        }
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;

public class AccessControl {
    private int32 role;

    public AccessControl(int32 role) {
        this.role = role;
    }

    public string getPermissionLabel() {
        if (role == 0) {
            return "guest";
        } else if (role == 1) {
            return "user";
        } else if (role == 2) {
            return "moderator";
        } else if (role == 3) {
            return "admin";
        } else {
            return "unknown";
        }
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| `else` without a preceding `if` | **E0210** `'else' without matching 'if'` |
| Placing a statement between `}` and `else` | **E0211** `unexpected token before 'else'` |
| Omitting braces on the `else` body | **E0202** `expected '{' to begin 'else' body` |

## 6. Related Keywords & Guides

- [`if`](if.md) — the conditional to which `else` belongs
- [`switch`](switch.md) — multi-way dispatch alternative to `else if` chains
