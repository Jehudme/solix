# `enum`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `enum` |
| Category | Declaration |
| Context | Top-level in a source file, or nested inside a class |
| Related | [`class`](class.md), [`switch`](../control_flow/switch.md), [`case`](../control_flow/case.md) |

`enum` declares an enumeration type — a closed set of named constant values of a common type. Enum constants are implicitly `public static const` members of their enclosing enum type. Solix enumerations are first-class types and may be used as `switch` discriminants, field types, method parameters, and return values. The compiler can verify exhaustiveness when all enum members are handled in a `switch`.

## 2. Permitted Contexts (Syntax & Grammar)

```
EnumDeclaration
    : Modifier* 'enum' Identifier '{' EnumBody '}'
    ;

EnumBody
    : EnumConstant ( ',' EnumConstant )* ','?
    ;

EnumConstant
    : Identifier
    ;
```

- Enum constants are separated by commas; a trailing comma is permitted.
- Enum types may not extend other classes or enums.
- Enum types may implement interfaces.
- The members of an enum may be referenced as `EnumName.CONSTANT`.

## 3. Semantics & Compiler Rules

- Each enum constant is a unique, singleton instance of the enum type.
- Enum types implicitly provide `name()` (returns the constant name as a `string`) and `ordinal()` (returns a zero-based integer position).
- Two enum values may be compared with `==`; they are reference-equal if and only if they are the same constant.
- When an enum type is used as the discriminant in a `switch`, the compiler checks exhaustiveness and issues **W0302** if not all constants are handled and no `default` is present.
- Enum types cannot be instantiated with `new`.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

public enum Color { RED, GREEN, BLUE }
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;

public enum Planet { MERCURY, VENUS, EARTH, MARS, JUPITER, SATURN, URANUS, NEPTUNE }

public class SolarSystem {
    public static bool isInnerPlanet(Planet p) {
        switch (p) {
            case Planet.MERCURY:
            case Planet.VENUS:
            case Planet.EARTH:
            case Planet.MARS:
                return true;
            case Planet.JUPITER:
            case Planet.SATURN:
            case Planet.URANUS:
            case Planet.NEPTUNE:
                return false;
        }
        return false;
    }

    public static void main(string[] args) {
        Planet home = Planet.EARTH;
        Console.println(home.name());     // "EARTH"
        Console.println(home.ordinal());  // 2
        Console.println(isInnerPlanet(home));  // true
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Instantiating an enum with `new` | **E0420** `enum type 'Color' cannot be instantiated with 'new'` |
| Enum `switch` missing some constants without `default` | **W0302** `switch on enum 'Planet' does not handle: NEPTUNE` |
| Duplicate enum constant name | **E0421** `duplicate enum constant 'RED'` |
| Enum extending a class | **E0422** `enum types cannot use 'extends'` |

## 6. Related Keywords & Guides

- [`switch`](../control_flow/switch.md) — dispatching on enum values
- [`case`](../control_flow/case.md) — labelling enum branches
- [`class`](class.md) — reference types; enums are closed class-like types
