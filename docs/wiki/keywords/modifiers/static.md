# `static`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `static` |
| Category | Modifier |
| Context | Before field, method, or nested class declarations inside a class |
| Related | [`const`](const.md), [`inline`](inline.md), [`this`](../expressions/this.md) |

`static` associates a field or method with the class itself rather than with any particular instance. A `static` field exists once for the entire class, shared across all instances. A `static` method can be called without an object reference, using the class name as the qualifier. `static` is commonly used for utility/helper methods, factory methods, constants, and counters that track class-level state.

## 2. Permitted Contexts (Syntax & Grammar)

```
StaticMember
    : Modifier* 'static' ( FieldDeclaration | MethodDeclaration | ClassDeclaration )
    ;
```

- `static` may be combined with access modifiers and with `const`, `native`, `inline`, or `abstract` (when applicable).
- `static` may not be applied to constructors or to top-level class declarations.
- `static` methods may not use `this` or access instance fields/methods directly.

## 3. Semantics & Compiler Rules

- A `static` field is initialised once, at class loading time, before any constructor is called.
- `static` methods may only access other `static` members of the class unless they receive an instance as a parameter.
- Using `this` inside a `static` method is a compile error **E0540** (`'this' not available in static context`).
- Calling a `static` method via an instance reference (e.g., `obj.staticMethod()`) is allowed syntactically but generates **W0312** (`static method called on instance; call on class instead`).
- `static` and `virtual`/`override`/`abstract` are mutually exclusive.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;

public class MathHelper {
    public static float64 square(float64 x) {
        return x * x;
    }

    public static float64 cube(float64 x) {
        return x * x * x;
    }
}

public class Program {
    public static void main(string[] args) {
        Console.println(MathHelper.square(3.0));  // 9.0
        Console.println(MathHelper.cube(2.0));    // 8.0
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;

public class IdGenerator {
    private static int32 nextId = 1;

    public static int32 generate() {
        int32 id = nextId;
        nextId = nextId + 1;
        return id;
    }
}

public class Entity {
    private int32 id;
    private string label;

    public Entity(string label) {
        this.id    = IdGenerator.generate();
        this.label = label;
    }

    public int32 getId()     { return id;    }
    public string getLabel() { return label; }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Using `this` in a `static` method | **E0540** `'this' is not available in a static context` |
| Accessing an instance field from a `static` method | **E0541** `cannot access instance field 'label' from a static context` |
| Marking a constructor `static` | **E0542** `constructors cannot be 'static'` |
| Combining `static` with `virtual` | **E0543** `'static' and 'virtual' cannot be combined` |
| Calling a `static` method through an instance | **W0312** `static method 'generate()' should be called on 'IdGenerator', not on an instance` |

## 6. Related Keywords & Guides

- [`const`](const.md) — implicitly `static`; compile-time constant
- [`inline`](inline.md) — often combined with `static` for utility methods
- [`this`](../expressions/this.md) — not available in `static` context
