# `sizeof`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `sizeof` |
| Category | Expression (Memory Introspection Operator) |
| Context | Expressions yielding `int32` byte size |
| Related | [`new`](new.md), [`instanceof`](instanceof.md) |

`sizeof` is a compile-time and runtime memory introspection operator that determines the size in bytes occupied by a primitive type, class instance, array reference, or dynamic heap object. When invoked on primitive types or class type names, it evaluates to a compile-time constant integer. When applied to dynamic heap references, it queries the runtime allocator header to determine the allocated block size.

## 2. Permitted Contexts (Syntax & Grammar)

```
SizeOfExpression : 'sizeof' '(' ( TypeIdentifier | Expression ) ')'
```

- **Type Argument**: `sizeof(T)` accepts any primitive type (`int8`, `int16`, `int32`, `int64`, `uint8`, `uint16`, `uint32`, `uint64`, `float32`, `float64`, `bool`, `char`, `void`), array type (`T[]`), class name, or enum name.
- **Expression Argument**: `sizeof(expr)` accepts any valid expression. If the expression evaluates to an object reference, the size is evaluated dynamically at runtime.

## 3. Semantics & Compiler Rules

- **Compile-Time Constant Folding**:
  - Primitive types evaluate immediately to their scalar byte size (`int8` = 1, `int16` = 2, `int32` = 4, `int64` = 8, `float32` = 4, `float64` = 8, `bool` = 1, `char` = 1, `void` = 0).
  - Array references evaluate to 8 bytes (the pointer size).
  - Class types evaluate to `(instance_size * 8)` bytes, reflecting the vtable header word plus all instance fields (including inherited fields).
  - Enums evaluate to 4 bytes.
  - The compiler folds these expressions into `PUSH_CONST_I32 <bytes>`.
- **Dynamic Instance Sizing**:
  - When given an object reference expression, the compiler emits `SIZEOF` (opcode 90).
  - At runtime, the VM checks the block header at `heap[ref - 1]` to retrieve the allocated block size in words, multiplies by 8, and pushes the byte count.
  - Passing `null` (or 0) returns `0`.
- **Expression Type**: `sizeof(...)` always yields an `int32`.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;

public class Point {
    int32 x;
    int32 y;
}

public class Main {
    public static void main(char[][] args) {
        int32 intSize = sizeof(int32);      // 4 bytes (constant)
        int32 pointSize = sizeof(Point);    // 24 bytes: 8 (vtable) + 8 (x) + 8 (y)

        Point p = new Point();
        int32 dynamicSize = sizeof(p);      // 24 bytes (evaluated from heap header)

        Console.println("Point static size: " + pointSize);
        Console.println("Point heap size: " + dynamicSize);
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

import solix.systems.Console;

public class BufferPool {
    public static int32 calculateBufferSize(int32 elementCount) {
        return elementCount * sizeof(int64);
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

- **Undeclared Types**: Specifying an unknown type name inside `sizeof(UnknownType)` produces a compile error:
  `Unknown type 'UnknownType' in sizeof expression`
- **Reference vs Primitive Size**: In Solix, classes are reference types. Statically querying `sizeof(MyClass)` returns the instance footprint on the heap, while querying an array reference returns 8 bytes (pointer width).

## 6. Related Keywords & Guides

- [`new`](new.md) — Allocating dynamic heap instances
- [`instanceof`](instanceof.md) — Dynamic type testing
