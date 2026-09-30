# `virtual`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `virtual` |
| Category | Modifier |
| Context | Before method declarations in a class body |
| Related | [`override`](override.md), [`abstract`](abstract.md), [`extends`](../expressions/extends.md) |

`virtual` marks a method as dynamically dispatched — the implementation selected at runtime is determined by the actual runtime type of the object, not the static compile-time type. A `virtual` method provides a default implementation that subclasses may `override`. This is the cornerstone of polymorphism in Solix: code written against a base-class reference automatically invokes the most derived implementation.

## 2. Permitted Contexts (Syntax & Grammar)

```
VirtualMethod
    : Modifier* 'virtual' ReturnType Identifier '(' ParameterList? ')' Block
    ;
```

- `virtual` may only be applied to instance methods; constructors and `static` methods cannot be `virtual`.
- `virtual` may be combined with access modifiers (`public`, `protected`).
- A `virtual` method must have a body; use `abstract` for methods without a default implementation.

## 3. Semantics & Compiler Rules

- When a `virtual` method is called through a base-class reference, the runtime dispatches to the most-derived override.
- A subclass that wishes to replace the implementation must mark its method with `override`; without `override`, shadowing is a warning.
- `virtual` and `static` are mutually exclusive: **E0580**.
- `virtual` and `abstract` are mutually exclusive (abstract methods are implicitly virtual): **E0581**.
- Non-virtual (ordinary) methods are dispatched statically; calling them through a base reference always calls the base implementation.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;

public class Animal {
    public virtual void speak() {
        Console.println("...");
    }
}

public class Cat extends Animal {
    public override void speak() {
        Console.println("Meow!");
    }
}

public class Dog extends Animal {
    public override void speak() {
        Console.println("Woof!");
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;

public class Renderer {
    public virtual void beginFrame()  { /* default: no-op */ }
    public virtual void endFrame()    { /* default: no-op */ }

    public virtual void drawRect(int32 x, int32 y, int32 w, int32 h) {
        Console.println("Renderer.drawRect(" + x + "," + y + "," + w + "," + h + ")");
    }
}

public class GpuRenderer extends Renderer {
    public override void beginFrame() {
        Console.println("GpuRenderer: begin frame");
    }

    public override void endFrame() {
        Console.println("GpuRenderer: end frame");
    }

    public override void drawRect(int32 x, int32 y, int32 w, int32 h) {
        Console.println("GpuRenderer.drawRect(" + x + "," + y + "," + w + "," + h + ")");
    }
}

public class Scene {
    public static void render(Renderer r) {
        r.beginFrame();
        r.drawRect(0, 0, 800, 600);
        r.endFrame();
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Combining `virtual` with `static` | **E0580** `'virtual' and 'static' cannot be combined` |
| Combining `virtual` with `abstract` | **E0581** `'abstract' methods are implicitly virtual; do not use both` |
| Subclass shadows a `virtual` without `override` | **W0314** `method 'speak()' hides virtual method in 'Animal'; use 'override'` |
| Applying `virtual` to a constructor | **E0582** `constructors cannot be 'virtual'` |

## 6. Related Keywords & Guides

- [`override`](override.md) — required to replace a `virtual` method in a subclass
- [`abstract`](abstract.md) — declares a virtual method with no default implementation
- [`extends`](../expressions/extends.md) — establishes the inheritance relationship
