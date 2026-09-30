# `class`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `class` |
| Category | Declaration |
| Context | Top-level in a source file, or as a nested type inside another class |
| Related | [`interface`](interface.md), [`enum`](enum.md), [`extends`](../expressions/extends.md), [`implements`](../expressions/implements.md), [`abstract`](../modifiers/abstract.md) |

`class` declares a reference type that encapsulates state (fields) and behaviour (methods and constructors). Solix classes support single inheritance via `extends` and may conform to multiple interfaces via `implements`. Every class that does not explicitly extend another implicitly extends the root class `solix.core.Object`. Instances of a class are allocated on the heap via `new` and are managed by Automatic Reference Counting (ARC).

## 2. Permitted Contexts (Syntax & Grammar)

```
ClassDeclaration
    : Modifier* 'class' Identifier
      ( 'extends' TypeName )?
      ( 'implements' TypeNameList )?
      '{' ClassBody '}'
    ;

ClassBody
    : ( FieldDeclaration | MethodDeclaration | ConstructorDeclaration | ClassDeclaration )*
    ;
```

- A source file may contain at most one `public` top-level class, whose name must match the file name.
- Nested (inner) classes are permitted and inherit the enclosing class's access modifiers scope.

## 3. Semantics & Compiler Rules

- If no constructor is declared, the compiler synthesises a default no-argument constructor.
- A class cannot extend more than one class (`extends` accepts a single type).
- A class may implement any number of interfaces.
- `abstract` classes cannot be instantiated; they may contain abstract methods.
- A non-abstract class must implement all abstract methods inherited from its superclass and all methods declared by its interfaces, or the compiler raises **E0400** (`unimplemented abstract member`).

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

public class Point {
    private float64 x;
    private float64 y;

    public Point(float64 x, float64 y) {
        this.x = x;
        this.y = y;
    }

    public float64 getX() { return x; }
    public float64 getY() { return y; }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;

public interface Drawable {
    public void draw();
}

public abstract class Shape implements Drawable {
    protected string color;

    public Shape(string color) {
        this.color = color;
    }

    public abstract float64 area();

    public string getColor() {
        return color;
    }
}

public class Circle extends Shape {
    private float64 radius;

    public Circle(float64 radius, string color) {
        super(color);
        this.radius = radius;
    }

    public override float64 area() {
        return 3.14159 * radius * radius;
    }

    public override void draw() {
        Console.println("Drawing circle r=" + radius + " color=" + color);
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| File name does not match `public class` name | **E0401** `public class 'Foo' must be declared in a file named 'Foo.slx'` |
| More than one `public` class in a file | **E0402** `only one public class per source file is allowed` |
| Non-abstract class with unimplemented abstract methods | **E0400** `class 'Circle' must implement 'area()' from 'Shape'` |
| Circular inheritance | **E0403** `class 'A' cannot extend 'B': circular inheritance detected` |
| Instantiating an abstract class | **E0404** `cannot instantiate abstract class 'Shape'` |

## 6. Related Keywords & Guides

- [`interface`](interface.md) — declares a purely abstract contract
- [`enum`](enum.md) — declares a finite set of named constants
- [`abstract`](../modifiers/abstract.md) — marks a class or method as abstract
- [`extends`](../expressions/extends.md) — specifies the superclass
- [`implements`](../expressions/implements.md) — lists implemented interfaces
- [`new`](../expressions/new.md) — creates class instances
