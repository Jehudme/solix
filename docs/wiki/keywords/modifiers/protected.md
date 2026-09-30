# `protected`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `protected` |
| Category | Modifier |
| Context | Before field, method, constructor, or nested class declarations |
| Related | [`public`](public.md), [`private`](private.md), [`internal`](internal.md) |

`protected` grants access to a member from within the declaring class and all subclasses, regardless of package. It is the appropriate modifier for members that form part of an inheritance contract — fields and methods that subclasses need to read or override, but that should remain hidden from unrelated external code. `protected` access is wider than `private` but narrower than `internal` or `public`.

## 2. Permitted Contexts (Syntax & Grammar)

```
ProtectedDeclaration
    : 'protected' MemberDeclaration
    ;
```

- May be applied to fields, methods, constructors, and nested classes.
- May not be applied to top-level types.
- Only one access modifier per declaration is permitted.

## 3. Semantics & Compiler Rules

- A `protected` member is accessible within:
  - The declaring class itself.
  - Any class that directly or transitively extends the declaring class.
- Access from outside these contexts generates **E0520** (`protected member not accessible`).
- A subclass that overrides a `protected` method may widen access to `public` but may not narrow it.
- `protected` fields are a common source of unwanted coupling; prefer `protected` accessors or abstract methods.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

public abstract class Animal {
    protected string name;

    public Animal(string name) {
        this.name = name;
    }

    public abstract void speak();
}

public class Dog extends Animal {
    public Dog(string name) {
        super(name);
    }

    public override void speak() {
        // 'name' is protected in Animal, accessible here in Dog.
        solix.systems.Console.println(name + " says: Woof!");
    }
}
```

### Idiomatic Usage

```solix
package solix.example.ui;

import solix.systems.Console;

public abstract class Widget {
    protected int32 x;
    protected int32 y;
    protected int32 width;
    protected int32 height;

    public Widget(int32 x, int32 y, int32 width, int32 height) {
        this.x      = x;
        this.y      = y;
        this.width  = width;
        this.height = height;
    }

    // Template method: subclasses implement the rendering detail.
    public void render() {
        beforeRender();
        drawContent();
        afterRender();
    }

    protected void beforeRender() {
        // Default: no-op; subclasses may override.
    }

    protected abstract void drawContent();

    protected void afterRender() {
        // Default: no-op; subclasses may override.
    }
}

public class Button extends Widget {
    private string label;

    public Button(int32 x, int32 y, int32 w, int32 h, string label) {
        super(x, y, w, h);
        this.label = label;
    }

    protected override void drawContent() {
        Console.println("Button[" + label + "] at (" + x + "," + y + ")");
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Accessing a `protected` member from an unrelated class | **E0520** `'name' is protected in 'Animal' and not accessible from 'Zoo'` |
| Narrowing access in an override (e.g., `protected` → `private`) | **E0521** `cannot narrow access from 'protected' to 'private' when overriding` |
| Applying `protected` to a top-level type | **E0511** `access modifier 'protected' cannot be applied to a top-level type` |

## 6. Related Keywords & Guides

- [`public`](public.md) — widest access
- [`private`](private.md) — narrowest; not inherited
- [`internal`](internal.md) — package-scoped access
- [`override`](override.md) — overriding `protected` virtual methods
