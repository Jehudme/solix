# `public`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `public` |
| Category | Modifier |
| Context | Before class, interface, enum, field, method, or constructor declarations |
| Related | [`private`](private.md), [`protected`](protected.md), [`internal`](internal.md) |

`public` is the broadest access modifier in Solix. A `public` member is accessible from any location in any package, subject only to the member's enclosing type being reachable. Declaring a top-level type `public` makes it part of a package's exported API surface. Library authors should carefully choose which members to declare `public`, as doing so creates a compatibility contract with consumers.

## 2. Permitted Contexts (Syntax & Grammar)

```
Modifier : 'public' | 'private' | 'protected' | 'internal' | ...
```

- May be applied to: top-level classes, interfaces, enums, nested types, fields, methods, constructors.
- Only one access modifier may appear per declaration.
- A `public` top-level class must be the only `public` class in its file, and the file must bear the class's name.

## 3. Semantics & Compiler Rules

- A `public` class or interface is visible from all packages.
- A `public` method or field on a non-`public` class is accessible only where the class itself is accessible — the member's effective access is constrained by its enclosing type.
- The compiler issues **E0500** if a `public` member's signature references a less-accessible type (e.g., a `public` method returning a `private` class).
- If no access modifier is specified, members default to `internal` visibility.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

public class Greeter {
    public string name;

    public Greeter(string name) {
        this.name = name;
    }

    public string greet() {
        return "Hello, " + name;
    }
}
```

### Idiomatic Usage

```solix
package solix.mylib;

import solix.systems.Console;

// Public API: only this class and its public members form the library contract.
public class Stack {
    private int32[] data;
    private int32   top;
    private static const int32 DEFAULT_CAPACITY = 16;

    public Stack() {
        this.data = new int32[DEFAULT_CAPACITY];
        this.top  = 0;
    }

    public void push(int32 value) {
        if (top >= data.length) {
            grow();
        }
        data[top] = value;
        top = top + 1;
    }

    public int32 pop() {
        if (top == 0) {
            throw new solix.core.UnderflowException("Stack is empty");
        }
        top = top - 1;
        return data[top];
    }

    public bool isEmpty() {
        return top == 0;
    }

    // Private implementation detail — not part of the public API.
    private void grow() {
        int32[] bigger = new int32[data.length * 2];
        for (int32 i = 0; i < data.length; i = i + 1) {
            bigger[i] = data[i];
        }
        data = bigger;
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Multiple access modifiers on one declaration | **E0501** `only one access modifier is permitted per declaration` |
| `public` method returning a `private` type | **E0500** `'public' member 'getConfig()' references less-accessible type 'Config'` |
| More than one `public` class in a source file | **E0402** `only one public class per source file is allowed` |

## 6. Related Keywords & Guides

- [`private`](private.md) — most restrictive; visible only within the declaring class
- [`protected`](protected.md) — visible within the class and its subclasses
- [`internal`](internal.md) — visible within the same package (default)
