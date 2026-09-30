# `override`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `override` |
| Category | Modifier |
| Context | Before method declarations in a subclass body |
| Related | [`virtual`](virtual.md), [`abstract`](abstract.md), [`extends`](../expressions/extends.md), [`super`](../expressions/super.md) |

`override` asserts that a method in a subclass is an intentional replacement of a `virtual` or `abstract` method inherited from a superclass. The compiler verifies that a matching `virtual` or `abstract` method exists in the inheritance chain with a compatible signature. `override` prevents accidental method hiding and makes the inheritance relationship explicit and auditable, especially when the superclass evolves over time.

## 2. Permitted Contexts (Syntax & Grammar)

```
OverrideMethod
    : Modifier* 'override' ReturnType Identifier '(' ParameterList? ')' Block
    ;
```

- `override` may only appear in a class that extends another class.
- The method being overridden must be `virtual` or `abstract`.
- The overriding method must have exactly the same name and parameter types as the overridden method.
- The return type must be the same as or a subtype of the overridden method's return type (covariant return).

## 3. Semantics & Compiler Rules

- The compiler validates that a compatible `virtual` or `abstract` method exists in a superclass; if not, **E0590** is raised.
- If a subclass method shadows a `virtual` method without `override`, **W0314** is issued.
- An `override` method may call the superclass implementation via `super.methodName(args)`.
- The access modifier of the override must not narrow the visibility of the overridden method.
- `override` implies `virtual`; the method is further overridable by deeper subclasses unless explicitly marked with a `final`-like pattern (not a keyword; seal by omitting `virtual` in the override).

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;

public class Vehicle {
    public virtual string describe() {
        return "Vehicle";
    }
}

public class Car extends Vehicle {
    public override string describe() {
        return "Car";
    }
}

public class ElectricCar extends Car {
    public override string describe() {
        return super.describe() + " (Electric)";
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;

public abstract class Report {
    protected string title;

    public Report(string title) {
        this.title = title;
    }

    public virtual void printHeader() {
        Console.println("=== " + title + " ===");
    }

    public abstract void printBody();

    public virtual void printFooter() {
        Console.println("=== End of Report ===");
    }

    public void print() {
        printHeader();
        printBody();
        printFooter();
    }
}

public class SalesReport extends Report {
    private float64 totalSales;

    public SalesReport(float64 totalSales) {
        super("Sales Report");
        this.totalSales = totalSales;
    }

    public override void printHeader() {
        super.printHeader();
        Console.println("Generated: 2026-09-30");
    }

    public override void printBody() {
        Console.println("Total Sales: $" + totalSales);
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| `override` with no matching virtual method in superclass | **E0590** `no virtual method 'speak()' in superclass to override` |
| Parameter types mismatch in override | **E0591** `override signature '(int32)' does not match virtual '(float64)'` |
| Narrowing access in override | **E0521** `cannot narrow access from 'public' to 'protected' when overriding` |
| Overriding a `static` method | **E0592** `static methods cannot be overridden; use 'override' only for virtual methods` |

## 6. Related Keywords & Guides

- [`virtual`](virtual.md) — marks methods that may be overridden
- [`abstract`](abstract.md) — abstract methods must be overridden in concrete subclasses
- [`super`](../expressions/super.md) — calls the parent implementation from an override
- [`extends`](../expressions/extends.md) — establishes the inheritance hierarchy
