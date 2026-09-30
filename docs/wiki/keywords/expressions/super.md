# `super`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `super` |
| Category | Expression |
| Context | Inside instance constructors and methods of derived classes |
| Related | [`this`](this.md), [`extends`](extends.md), [`class`](../declarations/class.md) |

`super` is a contextual reference keyword in Solix representing the immediate superclass of the current object. It serves two distinct purposes: invoking base class constructors in derived class constructor initializers, and calling non-virtual (or base class overridden) method implementations from within derived class methods.

## 2. Permitted Contexts (Syntax & Grammar)

```
SuperConstructorInvocation : 'super' '(' [ ArgumentList ] ')' ';'
SuperMethodCall : 'super' '.' Identifier '(' [ ArgumentList ] ')'
SuperFieldAccess : 'super' '.' Identifier
```

- Permitted only within instance constructors and instance methods of classes that explicitly specify an `extends` clause.
- `super(...)` constructor calls must appear as the very first statement within the derived class constructor.
- Cannot be used in `static` methods or static initialization blocks.
- Cannot be used in classes that do not inherit from a base class.

## 3. Semantics & Compiler Rules

- **Slot 0 Binding**: `super` evaluates to the object reference at local slot 0 (`this`), but type-bound to the superclass definition.
- **Direct Dispatch**: When invoking `super.method()`, the compiler bypasses virtual table dynamic dispatch and emits a direct `CALL` opcode to the superclass method address.
- **Constructor Chaining**: If a derived constructor does not explicitly invoke `super(...)`, the compiler automatically inserts an implicit zero-argument `super()` call before executing any statements.
- Using `super` outside an instance method or constructor triggers compiler error `E0312: 'super' used outside of class or in static context`.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

public class Animal {
    public string name;

    public Animal(string name) {
        this.name = name;
    }

    public virtual void speak() {
        Console.println(this.name + " makes a sound");
    }
}

public class Dog extends Animal {
    public Dog(string name) {
        super(name); // Invoking base class constructor
    }

    public override void speak() {
        super.speak(); // Calling base class implementation
        Console.println(this.name + " barks!");
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

public class BaseTask {
    protected int32 state;

    public BaseTask() {
        this.state = 0;
    }

    public virtual void run() {
        this.state = 1;
    }
}

public class LoggingTask extends BaseTask {
    public LoggingTask() {
        super();
    }

    public override void run() {
        Console.println("Task starting...");
        super.run();
        Console.println("Task finished with state: " + this.state);
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Using `super` in a static method | `'super' cannot be used in a static context` |
| Calling `super(...)` not as first statement | `Call to 'super' must be the first statement in constructor` |
| Using `super` in a class without a base class | `Class does not have a superclass` |

## 6. Related Keywords & Guides

- [`this`](this.md) — Reference to the current instance
- [`extends`](extends.md) — Declaring superclass inheritance
- [`override`](../modifiers/override.md) — Overriding virtual methods in derived classes
