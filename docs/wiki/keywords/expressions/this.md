# `this`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `this` |
| Category | Expression |
| Context | Inside instance constructors and methods |
| Related | [`super`](super.md), [`class`](../declarations/class.md) |

`this` is a keyword in Solix representing a reference to the current object instance upon which an instance method, constructor, or operator is being executed. It is passed implicitly as the first argument (`slot 0`) to all non-static methods and constructors, allowing unambiguous access to instance fields, member methods, and passing the current object to other methods.

## 2. Permitted Contexts (Syntax & Grammar)

```
ThisExpression : 'this'
ThisMemberAccess : 'this' '.' Identifier
ThisMethodCall : 'this' '.' Identifier '(' [ ArgumentList ] ')'
```

- Permitted within non-static member methods, instance field initializers, constructors, and overloaded operators.
- Cannot be referenced in `static` methods, static initializers, or top-level functions.
- Cannot be assigned to as an lvalue (e.g. `this = other;` is illegal).

## 3. Semantics & Compiler Rules

- **Local Slot 0**: In the Solix virtual machine call frame layout, `this` is assigned to local variable slot 0 (`GET_LOCAL 0`).
- **Disambiguation**: Used to distinguish instance fields from local variables or method parameters of the same name (e.g., `this.count = count;`).
- **Receiver Object**: When calling an instance method `speak()`, writing `this.speak()` or `speak()` evaluates `this` as the receiver address pushed before arguments.
- Any attempt to use `this` in a static context or outside a class produces the compiler error: `'this' cannot be used in a static context`.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

public class Rectangle {
    private int32 width;
    private int32 height;

    public Rectangle(int32 width, int32 height) {
        // Disambiguate fields from constructor parameters
        this.width = width;
        this.height = height;
    }

    public int32 area() {
        return this.width * this.height;
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

public class Builder {
    private string title;
    private int32 count;

    public Builder set_title(string title) {
        this.title = title;
        return this; // Fluent method chaining returning self
    }

    public Builder set_count(int32 count) {
        this.count = count;
        return this;
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Using `this` in a static method | `'this' cannot be used in a static context` |
| Attempting to assign to `this` (`this = x;`) | `Invalid lvalue: cannot assign to 'this'` |
| Using `this` outside of any class | `'this' is only valid within instance methods or constructors` |

## 6. Related Keywords & Guides

- [`super`](super.md) — Base class instance reference
- [`class`](../declarations/class.md) — Defining reference types containing `this`
- [`static`](../modifiers/static.md) — Declaring members that lack a `this` pointer
