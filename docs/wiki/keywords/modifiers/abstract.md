# `abstract`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `abstract` |
| Category | Modifier |
| Context | Class declarations, method declarations |
| Related | [`class`](../declarations/class.md), [`virtual`](virtual.md), [`override`](override.md), [`interface`](../declarations/interface.md) |

`abstract` marks an entity as incomplete, serving as an architectural contract that derived types must fulfill. An `abstract class` cannot be instantiated directly using `new` and can contain both concrete and abstract methods. An `abstract method` declares a method signature without an implementation body; any non-abstract derived class inheriting from this class is required to provide a concrete implementation using the `override` keyword.

## 2. Permitted Contexts (Syntax & Grammar)

```
ClassModifier : 'abstract' | AccessModifier
MethodModifier : 'abstract' | 'virtual' | 'override' | 'static' | AccessModifier

AbstractClass : [AccessModifier] 'abstract' 'class' Identifier [ 'extends' Type ] [ 'implements' TypeList ] '{' ... '}'
AbstractMethod : [AccessModifier] 'abstract' ReturnType Identifier '(' ParameterList ')' ';'
```

- Permitted on class declarations.
- Permitted on method declarations inside `abstract` classes.
- Cannot be applied to fields, constructors, or static methods.
- An abstract method cannot have a body (`{ ... }`); it terminates with a semicolon `;`.
- Cannot be combined with `static`, `native`, or `private` on a method declaration.

## 3. Semantics & Compiler Rules

- **No Direct Instantiation**: Attempting to execute `new AbstractType()` causes a compile-time error.
- **Enclosing Class Requirement**: If a class contains at least one `abstract` method, the class itself must be marked `abstract`.
- **Mandatory Override**: Any concrete subclass inheriting an `abstract` method must provide a matching `override` method; otherwise, the compiler rejects the subclass declaration.
- Abstract methods are entered into the class's virtual table (VTable) with a stub that triggers an `op_THROW_ABSTRACT` instruction if invoked directly.

## 4. Code Examples

### Basic Usage

```solix
package solix.geometry;

public abstract class Shape {
    public abstract float64 area();
}

public class Circle extends Shape {
    private float64 radius;

    public Circle(float64 radius) {
        this.radius = radius;
    }

    public override float64 area() {
        return 3.1415926535 * this.radius * this.radius;
    }
}
```

### Idiomatic Usage

```solix
package solix.database;

import solix.systems.Console;

public abstract class DatabaseDriver {
    protected string connection_url;

    public DatabaseDriver(string url) {
        this.connection_url = url;
    }

    public abstract void connect();
    public abstract void disconnect();
    public abstract bool execute_query(string sql);

    // Template method pattern: concrete method relying on abstract steps
    public void run_transaction(string sql) {
        this.connect();
        Console.println("Running query: " + sql);
        this.execute_query(sql);
        this.disconnect();
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Instantiating an abstract class (`new Shape()`) | `Cannot instantiate abstract class 'Shape'` |
| Abstract method in a non-abstract class | `Abstract method 'area' declared in non-abstract class 'Circle'` |
| Failing to override an inherited abstract method | `Class 'Circle' must implement abstract method 'area' from base class 'Shape'` |
| Providing a body for an abstract method | `Abstract method 'area' cannot have a body` |

## 6. Related Keywords & Guides

- [`class`](../declarations/class.md) — Reference type declarations
- [`virtual`](virtual.md) — Declaring dynamically dispatched methods with a default implementation
- [`override`](override.md) — Implementing inherited virtual and abstract methods
- [`interface`](../declarations/interface.md) — Pure contract types with no state
