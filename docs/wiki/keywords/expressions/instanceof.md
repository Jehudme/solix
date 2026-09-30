# `instanceof`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `instanceof` |
| Category | Expression (Binary Relational Operator) |
| Context | Inside expressions evaluating to boolean |
| Related | [`class`](../declarations/class.md), [`interface`](../declarations/interface.md) |

`instanceof` is a type inspection operator that tests whether an object reference dynamically belongs to a specified class or implements a specific interface at runtime. It performs a hierarchical traversal across the virtual method table (VTable) inheritance chain and evaluates to a boolean scalar value (`true` or `false`).

## 2. Permitted Contexts (Syntax & Grammar)

```
RelationalExpression : Expression 'instanceof' TypeIdentifier
```

- Left operand must evaluate to a reference type (class instance, interface reference, or `null`).
- Right operand must be a resolvable type identifier (class or interface name).
- Cannot be used on primitive types (e.g., `42 instanceof int32` is a compile-time error).

## 3. Semantics & Compiler Rules

- **Null Handling**: If the left operand evaluates to `null`, `instanceof` unconditionally evaluates to `false` without raising an exception.
- **Bytecode Emission**: The compiler emits the `INSTANCEOF` opcode with the target type's VTable ID as an immediate argument.
- **Runtime Traversal**: The VM checks whether the object's `vtable_id` matches the target ID or has the target ID anywhere in its `vtable_bases` parent chain.
- If the types are statically known to be in completely disjoint inheritance trees, the compiler emits a compile-time error or warning.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;

public class Animal {}
public class Dog extends Animal {}
public class Cat extends Animal {}

public class Main {
    public static void main(char[][] args) {
        Animal a = new Dog();

        if (a instanceof Dog) {
            Console.println("a is a Dog");
        }

        if (!(a instanceof Cat)) {
            Console.println("a is NOT a Cat");
        }
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;

public class EventProcessor {
    public void process(solix.core.Object event) {
        if (event == null) {
            return;
        }

        if (event instanceof NetworkPacket) {
            NetworkPacket packet = (NetworkPacket) event;
            packet.handle_network();
        } else if (event instanceof TimerEvent) {
            TimerEvent timer = (TimerEvent) event;
            timer.trigger_alarm();
        } else {
            Console.println("Unknown event type received");
        }
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Using `instanceof` with primitive left operand (`x instanceof int32`) | `Incompatible types: primitive type 'int32' cannot be used with 'instanceof'` |
| Specifying a non-existent right operand | `Cannot resolve symbol 'MissingType' in instanceof expression` |
| Using an expression instead of a type on right side (`obj instanceof (1 + 2)`) | `Expected type identifier after 'instanceof'` |

## 6. Related Keywords & Guides

- [`class`](../declarations/class.md) — Reference types checked by `instanceof`
- [`interface`](../declarations/interface.md) — Interface conformance checked by `instanceof`
- [InstanceOf Spec](../../../spec/statements/expressions/instanceof_expression.md) — Compiler lowering details of `instanceof`
