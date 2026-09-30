# `implements`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `implements` |
| Category | Declaration Clause / Expression Keyword |
| Context | Class declarations |
| Related | [`interface`](../declarations/interface.md), [`extends`](extends.md), [`class`](../declarations/class.md) |

`implements` declares that a class conforms to one or more interface specifications in Solix. By specifying `implements`, a class guarantees that it provides concrete method implementations for all abstract method signatures declared in the referenced interfaces, enabling polymorphic interface dispatch and nominal subtyping checks via `instanceof`.

## 2. Permitted Contexts (Syntax & Grammar)

```
ClassDeclaration : [ClassModifiers] 'class' Identifier [ 'extends' SuperClass ] [ 'implements' InterfaceList ] Block
InterfaceList : TypeIdentifier { ',' TypeIdentifier }
```

- Appears following the class name (and following the `extends` clause if present).
- Can reference one or multiple interface types separated by commas.
- Permitted on both concrete and `abstract` classes.
- Cannot be used on interface declarations (interfaces use `extends`).

## 3. Semantics & Compiler Rules

- **Contract Fulfillment**: If the class is concrete, every method declared in all implemented interfaces must be implemented with matching parameter types and return type.
- **Interface VTable Slots**: The compiler maps interface method offsets into the class's dispatch table.
- **Subtype Compatibility**: An instance of a class implementing interface `I` is assignment-compatible with variable type `I` and satisfies `obj instanceof I`.
- If an abstract class implements an interface, it is permitted to defer some or all method implementations to derived subclasses.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

public interface Printable {
    public void print();
}

public class Document implements Printable {
    private string text;

    public Document(string text) {
        this.text = text;
    }

    public void print() {
        Console.println(this.text);
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

public interface Comparable<T> {
    public int32 compare_to(T other);
}

public interface Serializable {
    public string serialize();
}

// Class implementing multiple interfaces
public class Account implements Comparable<Account>, Serializable {
    private int32 id;
    private float64 balance;

    public Account(int32 id, float64 balance) {
        this.id = id;
        this.balance = balance;
    }

    public int32 compare_to(Account other) {
        if (this.balance < other.balance) { return -1; }
        if (this.balance > other.balance) { return 1; }
        return 0;
    }

    public string serialize() {
        return "Account#" + this.id;
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Failing to implement an interface method | `Class 'Document' does not implement interface method 'print()' from interface 'Printable'` |
| Specifying a class in `implements` list | `Type 'SuperClass' is a class, not an interface` |
| Method signature mismatch (wrong return type) | `Method 'print()' has incompatible return type with interface 'Printable'` |

## 6. Related Keywords & Guides

- [`interface`](../declarations/interface.md) — Interface contract declarations
- [`extends`](extends.md) — Single class inheritance
- [`class`](../declarations/class.md) — Reference types implementing contracts
