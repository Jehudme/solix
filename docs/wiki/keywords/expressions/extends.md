# `extends`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `extends` |
| Category | Declaration Clause / Expression Keyword |
| Context | Class declarations (inheritance) and interface declarations |
| Related | [`class`](../declarations/class.md), [`implements`](implements.md), [`super`](super.md) |

`extends` establishes nominal inheritance between types in Solix. When applied to a class declaration, it specifies single class inheritance, allowing the derived subclass to inherit all non-private fields, constructors, and methods from the base superclass. When applied to an interface declaration, it allows one interface to inherit and combine method contracts from one or more parent interfaces.

## 2. Permitted Contexts (Syntax & Grammar)

```
ClassExtendsClause : 'class' Identifier 'extends' SuperClassName
InterfaceExtendsClause : 'interface' Identifier 'extends' InterfaceList
```

- In class declarations: exactly one base class is permitted (single inheritance).
- In interface declarations: comma-separated list of multiple interfaces is permitted.
- The target type must be an already accessible class (or interface).
- Cannot extend final/primitive types or create cyclic inheritance hierarchies.

## 3. Semantics & Compiler Rules

- **Field Layout Inheritance**: The subclass memory layout begins with the contiguous fields of its superclass, followed by its own newly declared fields.
- **VTable Chaining**: The compiler constructs the subclass VTable by copying the superclass VTable entries and overriding matching slots, linking `vtable_bases` for `instanceof` checks.
- **Constructor Delegation**: The derived class constructor must invoke `super(...)` as its first action.
- Cyclic inheritance (e.g. `class A extends B` and `class B extends A`) causes compiler error `E0108: Cyclic inheritance detected`.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

public class Employee {
    protected string name;
    protected float64 salary;

    public Employee(string name, float64 salary) {
        this.name = name;
        this.salary = salary;
    }
}

public class Manager extends Employee {
    private string department;

    public Manager(string name, float64 salary, string department) {
        super(name, salary);
        this.department = department;
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

public interface Reader {
    public char read_char();
}

public interface Writer {
    public void write_char(char c);
}

// An interface extending multiple parent interfaces
public interface ReadWriter extends Reader, Writer {
    public void flush();
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Attempting multiple class inheritance (`class C extends A, B`) | `Syntax error: A class may only extend a single superclass` |
| Extending a non-class type (`class C extends int32`) | `Type 'int32' cannot be extended` |
| Cyclic class inheritance chain | `Cyclic inheritance detected: 'A' -> 'B' -> 'A'` |

## 6. Related Keywords & Guides

- [`class`](../declarations/class.md) — Reference types using `extends`
- [`implements`](implements.md) — Conforming to interfaces
- [`super`](super.md) — Invoking base class constructors and methods
