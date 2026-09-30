# Classes and Object-Oriented Programming

Solix is a fully object-oriented language. Everything beyond primitive values is represented as a class instance. This guide covers class declaration, constructors, inheritance, interfaces, abstract classes, operator overloading, and the `super` keyword.

---

## Table of Contents

1. [Class Declaration](#class-declaration)
2. [Constructors](#constructors)
3. [Instance Fields](#instance-fields)
4. [Static Fields and Methods](#static-fields-and-methods)
5. [The `this` Keyword](#the-this-keyword)
6. [Single Inheritance (`extends`)](#single-inheritance-extends)
7. [Method Overriding (`virtual` / `override`)](#method-overriding-virtual--override)
8. [Abstract Classes and Methods](#abstract-classes-and-methods)
9. [Interfaces (`implements`)](#interfaces-implements)
10. [Operator Overloading](#operator-overloading)
11. [The `super` Keyword](#the-super-keyword)

---

## Class Declaration

A class is declared with the `class` keyword inside a package. Access is controlled via `public` (visible to all packages) or `private`/`protected`.

```solix
package com.example;

public class Point {
    public float64 x;
    public float64 y;
}
```

---

## Constructors

Constructors initialize a new instance. A class may have **multiple constructors** that differ by their parameter lists. Constructors are named after the class, have no return type, and are declared with `public`.

```solix
package com.example;

public class Rectangle {
    public float64 width;
    public float64 height;

    // Default constructor
    public Rectangle() {
        this.width  = 0.0;
        this.height = 0.0;
    }

    // Constructor with initial dimensions
    public Rectangle(float64 w, float64 h) {
        this.width  = w;
        this.height = h;
    }

    // Copy constructor
    public Rectangle(Rectangle source) {
        this.width  = source.width;
        this.height = source.height;
    }

    public float64 area() {
        return this.width * this.height;
    }
}
```

Creating instances:

```solix
Rectangle r1 = new Rectangle();           // 0×0
Rectangle r2 = new Rectangle(10.0, 5.0);  // 10×5
Rectangle r3 = new Rectangle(r2);         // copy of r2
```

---

## Instance Fields

Fields declared at class level (not inside a method) are **instance fields** — each object has its own copy. Fields may be `public`, `protected`, or `private`.

```solix
public class BankAccount {
    private float64 balance;
    private String owner_name;

    public BankAccount(String name, float64 initial) {
        this.owner_name = name;
        this.balance    = initial;
    }

    public void deposit(float64 amount) {
        this.balance = this.balance + amount;
    }

    public float64 get_balance() {
        return this.balance;
    }
}
```

---

## Static Fields and Methods

Fields and methods declared with `static` belong to the **class** rather than any instance. They are accessed via the class name, not `this`.

```solix
public class Counter {
    private static int32 instance_count = 0;

    public Counter() {
        Counter.instance_count++;
    }

    public static int32 get_count() {
        return Counter.instance_count;
    }
}

// Usage
Counter a = new Counter();
Counter b = new Counter();
Console.println(Counter.get_count());   // 2
```

Static methods cannot access instance fields (`this` is not available inside a static method).

---

## The `this` Keyword

`this` refers to the **current instance** within a constructor or instance method. It is used to disambiguate field names from parameter names and to call sibling methods.

```solix
public class Circle {
    private float64 radius;

    public Circle(float64 radius) {
        this.radius = radius;   // field = parameter
    }

    public float64 get_radius() {
        return this.radius;
    }

    public float64 circumference() {
        return 2.0 * 3.14159 * this.radius;
    }

    public float64 area() {
        return 3.14159 * this.radius * this.radius;
    }
}
```

---

## Single Inheritance (`extends`)

Solix supports **single inheritance**. A class may extend exactly one other class using the `extends` keyword. The subclass inherits all `public` and `protected` fields and methods.

```solix
package com.example.shapes;

public class Shape {
    protected String color;

    public Shape(String color) {
        this.color = color;
    }

    public virtual String describe() {
        return new String("A shape of color: ").concat(this.color);
    }
}

public class Circle extends Shape {
    private float64 radius;

    public Circle(String color, float64 radius) {
        super(color);          // Call parent constructor
        this.radius = radius;
    }

    public float64 area() {
        return 3.14159 * this.radius * this.radius;
    }
}
```

---

## Method Overriding (`virtual` / `override`)

A base-class method must be declared `virtual` to be overridable. A subclass provides a new implementation using `override`.

```solix
public class Animal {
    public virtual String speak() {
        return new String("...");
    }
}

public class Dog extends Animal {
    public override String speak() {
        return new String("Woof!");
    }
}

public class Cat extends Animal {
    public override String speak() {
        return new String("Meow!");
    }
}
```

**Polymorphic dispatch:**

```solix
Animal a = new Dog();
Console.println(a.speak());   // "Woof!" — dispatches to Dog.speak()
```

> [!IMPORTANT]
> If a base-class method is **not** declared `virtual`, it cannot be overridden. Attempting to use `override` on such a method is a compile-time error.

---

## Abstract Classes and Methods

An **abstract** class cannot be instantiated directly. It declares one or more **abstract methods** — methods with no body — that subclasses must implement.

```solix
public abstract class Shape {
    protected String color;

    public Shape(String color) {
        this.color = color;
    }

    // Subclasses must provide an implementation
    public abstract float64 area();

    // Concrete method available to all subclasses
    public String get_color() {
        return this.color;
    }
}

public class Square extends Shape {
    private float64 side;

    public Square(String color, float64 side) {
        super(color);
        this.side = side;
    }

    public override float64 area() {
        return this.side * this.side;
    }
}
```

Attempting to call an abstract method directly (e.g., on an abstract-type reference whose concrete subclass does not override it) causes a `THROW_ABSTRACT` at runtime.

---

## Interfaces (`implements`)

An **interface** defines a contract — a set of method signatures without implementations. Classes that fulfill the contract declare `implements <InterfaceName>`.

```solix
package com.example;

public interface Printable {
    void print_info();
}

public interface Serializable {
    String serialize();
}

public class Document implements Printable, Serializable {
    private String title;
    private String content;

    public Document(String title, String content) {
        this.title   = title;
        this.content = content;
    }

    public void print_info() {
        Console.println(this.title);
    }

    public String serialize() {
        return new String("[DOC] ").concat(this.title);
    }
}
```

A class can implement multiple interfaces. The compiler verifies that all interface methods are implemented.

---

## Operator Overloading

Solix allows classes to define custom behavior for standard operators using the `operator` keyword. The syntax is:

```
public <ReturnType> operator<symbol>(<parameter>) { ... }
```

Supported operators: `=`, `+`, `-`, `*`, `/`, `==`, `!=`, `<`, `>`, `<=`, `>=`.

```solix
public class Vector2 {
    public float64 x;
    public float64 y;

    public Vector2(float64 x, float64 y) {
        this.x = x;
        this.y = y;
    }

    // Addition
    public Vector2 operator+(Vector2 other) {
        return new Vector2(this.x + other.x, this.y + other.y);
    }

    // Scalar multiplication
    public Vector2 operator*(float64 scalar) {
        return new Vector2(this.x * scalar, this.y * scalar);
    }

    // Equality
    public bool operator==(Vector2 other) {
        return this.x == other.x && this.y == other.y;
    }

    // Assignment (deep copy)
    public Vector2 operator=(Vector2 source) {
        this.x = source.x;
        this.y = source.y;
        return this;
    }
}

// Usage
Vector2 a = new Vector2(1.0, 2.0);
Vector2 b = new Vector2(3.0, 4.0);
Vector2 c = a + b;           // (4.0, 6.0)
Vector2 d = c * 2.0;         // (8.0, 12.0)
```

---

## The `super` Keyword

`super` provides access to the **parent class** from within a subclass. It is used in two contexts:

### 1. Calling the Parent Constructor

The first statement in a subclass constructor can be `super(...)` to invoke the parent's constructor:

```solix
public class Vehicle {
    protected String brand;

    public Vehicle(String brand) {
        this.brand = brand;
    }
}

public class Car extends Vehicle {
    private int32 door_count;

    public Car(String brand, int32 doors) {
        super(brand);             // Must be the first statement
        this.door_count = doors;
    }
}
```

### 2. Accessing Overridden Parent Methods

`super.method(...)` calls the **parent's version** of a method even if the current class has overridden it:

```solix
public class Animal {
    public virtual String describe() {
        return new String("I am an animal.");
    }
}

public class Dog extends Animal {
    public override String describe() {
        String base = super.describe();
        return base.concat(new String(" I am also a dog."));
    }
}
```

> [!NOTE]
> `super` cannot be used in static methods or outside the direct subclass — it always refers to the immediate parent's scope.
