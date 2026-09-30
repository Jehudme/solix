# `new`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `new` |
| Category | Expression |
| Context | Object instantiation, array allocation expressions |
| Related | [`class`](../declarations/class.md), [`this`](this.md) |

`new` is the memory allocation operator in Solix. It dynamically allocates storage on the heap for a new object instance or a typed array, initializes its memory, invokes the appropriate constructor for classes, sets up the object's ARC header (with an initial reference count of 1) and VTable identifier, and yields a reference pointer to the newly allocated block.

## 2. Permitted Contexts (Syntax & Grammar)

```
NewInstanceExpression : 'new' TypeName '(' [ ArgumentList ] ')'
ArrayCreationExpression : 'new' ElementType '[' Expression ']'
```

- Permitted within any expression where a reference value of the target type is expected.
- For classes, requires matching constructor arguments (or default zero-arg constructor if provided).
- For arrays, requires an integer expression inside brackets denoting the element count.
- Cannot be used with primitive scalar types directly (e.g. `new int32()` is illegal; use `new int32[10]` for arrays).
- Cannot be used to instantiate an `abstract class` or an `interface`.

## 3. Semantics & Compiler Rules

- **Heap Allocation**: Emits `ALLOC_DYNAMIC` with the calculated word count required for the object header, vtable pointer, and instance fields (or array length + elements).
- **VTable Binding**: Emits `SET_VTABLE` for class instances to link virtual method dispatch.
- **Constructor Execution**: Emits `CALL` to the resolved constructor overload passing the allocated address as `this` (slot 0).
- **ARC Registration**: Sets `ref_count = 1`. If the result is assigned to a variable or field, ARC tracks ownership cleanly.
- If heap memory is exhausted, the runtime throws `OutOfMemoryException`.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

public class Point {
    public int32 x;
    public int32 y;

    public Point(int32 x, int32 y) {
        this.x = x;
        this.y = y;
    }
}

public class Main {
    public static void main(char[][] args) {
        // Class instance instantiation
        Point p = new Point(10, 20);

        // Array creation
        int32[] numbers = new int32[5];
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.collections.List;

public class CustomerRegistry {
    private List<string> names;

    public CustomerRegistry() {
        this.names = new List<string>(32);
    }

    public void register(string name) {
        this.names.add(name);
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Instantiating an abstract class (`new AbstractClass()`) | `Cannot instantiate abstract class 'AbstractClass'` |
| Constructor argument mismatch | `No constructor matching arguments '(int32, string)' found for class 'Point'` |
| Array creation with non-integer size (`new int32["hello"]`) | `Array dimension expression must evaluate to an integer type` |

## 6. Related Keywords & Guides

- [`class`](../declarations/class.md) — Reference types instantiated with `new`
- [`this`](this.md) — Reference to the current instance inside constructors
- [New Instance Spec](../../../spec/statements/expressions/new_instance_expression.md) — Bytecode lowering of `new`
