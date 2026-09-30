# `switch`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `switch` |
| Category | Control Flow |
| Context | Statement position inside method or constructor bodies |
| Related | [`case`](case.md), [`default`](default.md), [`break`](break.md), [`if`](if.md) |

`switch` is a multi-way branching statement that dispatches execution to one of several labelled `case` sections based on the value of a single expression. It is idiomatic for exhaustive dispatch over integral types and enumerations, and is often more readable than a long `else if` chain when the discriminant can take many discrete values.

## 2. Permitted Contexts (Syntax & Grammar)

```
SwitchStatement
    : 'switch' '(' Expression ')' '{' SwitchBlock '}'
    ;

SwitchBlock
    : ( CaseClause | DefaultClause )*
    ;

CaseClause
    : 'case' ConstantExpression ':' Statement*
    ;

DefaultClause
    : 'default' ':' Statement*
    ;
```

- The switch expression must be of type `int8`, `int16`, `int32`, `int64`, `char`, `bool`, `string`, or any `enum` type.
- Each `case` label must be a compile-time constant expression of a type compatible with the switch expression.
- Duplicate `case` values are a compile error.
- At most one `default` clause may appear; it may be placed anywhere within the switch block.

## 3. Semantics & Compiler Rules

- Solix `switch` does **not** fall through between cases by default. Each case block executes independently unless it explicitly calls `break` (which is required to exit the block).
- The compiler issues **E0240** if a `case` block lacks a terminal `break`, `return`, or `throw`.
- The `default` clause is executed when no `case` matches.
- When switching on an `enum`, the compiler issues **W0302** (`non-exhaustive switch`) if not all enum constants are covered and no `default` is present.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;

public class DayName {
    public static string name(int32 day) {
        switch (day) {
            case 0: return "Sunday";
            case 1: return "Monday";
            case 2: return "Tuesday";
            case 3: return "Wednesday";
            case 4: return "Thursday";
            case 5: return "Friday";
            case 6: return "Saturday";
            default: return "Unknown";
        }
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;

public enum Direction { NORTH, SOUTH, EAST, WEST }

public class Navigator {
    public static string describe(Direction d) {
        switch (d) {
            case Direction.NORTH: return "heading north";
            case Direction.SOUTH: return "heading south";
            case Direction.EAST:  return "heading east";
            case Direction.WEST:  return "heading west";
        }
        // Unreachable; compiler verifies exhaustiveness for enums.
        return "";
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Missing `break` at end of a non-terminating case | **E0240** `'case' block must end with 'break', 'return', or 'throw'` |
| Duplicate `case` labels | **E0241** `duplicate case value '<value>'` |
| Non-constant `case` expression | **E0242** `'case' label must be a compile-time constant` |
| Multiple `default` clauses | **E0243** `switch statement may have at most one 'default' label` |
| Non-exhaustive enum switch without `default` | **W0302** `switch on enum 'Direction' does not handle: WEST` |

## 6. Related Keywords & Guides

- [`case`](case.md) — labels a branch within `switch`
- [`default`](default.md) — fallback branch
- [`break`](break.md) — exits the switch block
- [`if`](if.md) — alternative for complex boolean conditions
