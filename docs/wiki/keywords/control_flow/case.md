# `case`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `case` |
| Category | Control Flow |
| Context | Inside a `switch` block only |
| Related | [`switch`](switch.md), [`default`](default.md), [`break`](break.md) |

`case` labels a specific value-branch within a `switch` statement. When the `switch` expression matches the constant value following `case`, execution begins at the first statement after the colon and continues until a `break`, `return`, or `throw` terminates the block. Each `case` label must carry a compile-time constant expression, and no two labels within the same `switch` may share the same value.

## 2. Permitted Contexts (Syntax & Grammar)

```
CaseClause
    : 'case' ConstantExpression ':' Statement*
    ;
```

- `case` is only valid inside the body of a `switch` statement; using it elsewhere is a syntax error.
- The expression after `case` must be a compile-time constant: an integer literal, character literal, string literal, boolean literal, or a reference to a `const` or `enum` member.
- Multiple `case` labels may be stacked before a single statement list to express "match any of these values".

## 3. Semantics & Compiler Rules

- When the `switch` discriminant equals the case constant, control jumps to the statements following the colon.
- The block must be terminated by `break`, `return`, or `throw`; the compiler issues **E0240** otherwise.
- Stacked labels (`case 1: case 2: return "low";`) are allowed and share a single body.
- The type of the constant must be compatible with the `switch` expression type; implicit widening conversions are applied where unambiguous, but narrowing is an error.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;

public class Vowel {
    public static bool isVowel(char c) {
        switch (c) {
            case 'a':
            case 'e':
            case 'i':
            case 'o':
            case 'u':
                return true;
            default:
                return false;
        }
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;

public enum Season { SPRING, SUMMER, AUTUMN, WINTER }

public class SeasonInfo {
    public static string getClothing(Season s) {
        switch (s) {
            case Season.SPRING:
                return "light jacket";
            case Season.SUMMER:
                return "t-shirt";
            case Season.AUTUMN:
                return "sweater";
            case Season.WINTER:
                return "heavy coat";
        }
        return "";
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| `case` outside a `switch` | **E0244** `'case' label not inside a 'switch' statement` |
| Non-constant case expression | **E0242** `'case' label must be a compile-time constant` |
| Duplicate case value | **E0241** `duplicate case value '3'` |
| Missing `break`/`return`/`throw` at end of case block | **E0240** `'case' block must end with 'break', 'return', or 'throw'` |

## 6. Related Keywords & Guides

- [`switch`](switch.md) — the enclosing statement
- [`default`](default.md) — the fallback label when no case matches
- [`break`](break.md) — exits the current case block
