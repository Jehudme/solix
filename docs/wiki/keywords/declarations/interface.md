# `interface`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `interface` |
| Category | Declaration |
| Context | Top-level in a source file, or nested inside a class |
| Related | [`class`](class.md), [`implements`](../expressions/implements.md), [`extends`](../expressions/extends.md), [`abstract`](../modifiers/abstract.md) |

`interface` declares a purely abstract contract — a named set of method signatures (and optionally constant fields) that conforming classes must implement. Interfaces define capabilities independent of class hierarchy, enabling polymorphism across otherwise unrelated classes. An interface may extend one or more other interfaces. Solix interfaces do not support default method bodies.

## 2. Permitted Contexts (Syntax & Grammar)

```
InterfaceDeclaration
    : Modifier* 'interface' Identifier
      ( 'extends' TypeNameList )?
      '{' InterfaceBody '}'
    ;

InterfaceBody
    : ( MethodSignature | ConstantField )*
    ;

MethodSignature
    : Modifier* ReturnType Identifier '(' ParameterList? ')' ';'
    ;
```

- All methods declared in an interface are implicitly `public` and `abstract`; providing these modifiers explicitly is permitted but redundant.
- Interface fields are implicitly `public static const`.
- An interface may `extends` multiple other interfaces (comma-separated).

## 3. Semantics & Compiler Rules

- A class uses `implements` to conform to an interface; it must provide a concrete implementation for every method signature in the interface (and any interfaces it extends).
- Interfaces cannot be instantiated directly.
- A reference of interface type may hold any object whose class implements that interface; the compiler enforces type compatibility.
- Implementing a method from an interface requires the `override` modifier in the implementing class.
- **E0410** is raised when a class claims to implement an interface but does not provide all required methods.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

public interface Serializable {
    public string serialize();
    public void deserialize(string data);
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;

public interface Comparable {
    public int32 compareTo(Object other);
}

public interface Printable {
    public void print();
}

public class Temperature implements Comparable, Printable {
    private float64 celsius;

    public Temperature(float64 celsius) {
        this.celsius = celsius;
    }

    public override int32 compareTo(Object other) {
        Temperature t = (Temperature) other;
        if (celsius < t.celsius) { return -1; }
        if (celsius > t.celsius) { return  1; }
        return 0;
    }

    public override void print() {
        Console.println(celsius + "°C");
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Providing a method body inside an interface | **E0411** `interface method 'print' must not have a body` |
| Instantiating an interface | **E0412** `cannot instantiate interface 'Printable'` |
| Class does not implement all interface methods | **E0410** `class 'Temperature' must implement 'compareTo(Object)' from 'Comparable'` |
| Non-`public` method in interface | **W0306** `interface method 'foo' is implicitly 'public'; explicit access modifier ignored` |

## 6. Related Keywords & Guides

- [`class`](class.md) — declares concrete types that implement interfaces
- [`implements`](../expressions/implements.md) — declares interface conformance on a class
- [`extends`](../expressions/extends.md) — used by interfaces to inherit from other interfaces
- [`abstract`](../modifiers/abstract.md) — can mark a class with partial implementations
