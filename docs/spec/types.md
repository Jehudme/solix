# Solix Type System & Static Semantics

This document specifies the complete type system of Solix: primitive types, reference types, conversion rules, subtype relations, generics, and name resolution. It is normative — the compiler must enforce these rules at compile time unless otherwise noted.

---

## Table of Contents

1. [Primitive Types](#1-primitive-types)
2. [Numeric Widening Conversions](#2-numeric-widening-conversions)
3. [Narrowing Conversions](#3-narrowing-conversions)
4. [Reference Types](#4-reference-types)
5. [Null Safety](#5-null-safety)
6. [Array Types](#6-array-types)
7. [Type Compatibility Rules](#7-type-compatibility-rules)
8. [Nominal Subtyping](#8-nominal-subtyping)
9. [Method Override Rules](#9-method-override-rules)
10. [Template & Generic System](#10-template--generic-system)
11. [Type Resolution Order](#11-type-resolution-order)

---

## 1. Primitive Types

Primitive types are value types. They are stored directly in stack slots or object fields as 64-bit words; the higher bits are zero-extended or ignored depending on the type. Primitive types are **never** `null` and are never heap-allocated on their own.

| Type      | Width    | Signed | Range / Values                                      | Default Value |
|-----------|----------|--------|-----------------------------------------------------|---------------|
| `bool`    | 1-bit logical | — | `false` (0) or `true` (1)                         | `false`       |
| `char`    | 8 bits   | No     | U+0000 – U+007F (ASCII)                             | `'\0'`        |
| `int8`    | 8 bits   | Yes    | −128 to 127                                         | `0`           |
| `int16`   | 16 bits  | Yes    | −32 768 to 32 767                                   | `0`           |
| `int32`   | 32 bits  | Yes    | −2 147 483 648 to 2 147 483 647                     | `0`           |
| `int64`   | 64 bits  | Yes    | −9 223 372 036 854 775 808 to 9 223 372 036 854 775 807 | `0`       |
| `uint8`   | 8 bits   | No     | 0 to 255                                            | `0`           |
| `uint16`  | 16 bits  | No     | 0 to 65 535                                         | `0`           |
| `uint32`  | 32 bits  | No     | 0 to 4 294 967 295                                  | `0`           |
| `uint64`  | 64 bits  | No     | 0 to 18 446 744 073 709 551 615                     | `0`           |
| `float32` | 32 bits  | —      | IEEE 754 single-precision                           | `0.0f`        |
| `float64` | 64 bits  | —      | IEEE 754 double-precision                           | `0.0`         |
| `void`    | —        | —      | No value; valid only as a method return type         | —             |

> [!NOTE]
> All primitive values occupy one full 64-bit stack word at runtime. The VM sign-extends or zero-extends narrower types when they are pushed onto the operand stack, and truncates on conversion back.

---

## 2. Numeric Widening Conversions

**Widening** conversions are implicit: the compiler inserts the appropriate `CONV_*` instruction automatically when an expression of a narrower type is used where a wider type is expected, with no loss of information.

### Signed Integer Widening Chain

```
int8 → int16 → int32 → int64
```

Each step sign-extends the value. An `int8` with value `−1` (bit pattern `0xFF`) widens to `int64` `−1` (bit pattern `0xFFFFFFFFFFFFFFFF`).

### Unsigned Integer Widening Chain

```
uint8 → uint16 → uint32 → uint64
```

Each step zero-extends the value.

### Float Widening

```
float32 → float64
```

The value is converted from IEEE 754 single-precision to double-precision. This conversion is always exact for all finite `float32` values.

### Integer-to-Float Widening

Signed integers up to `int32` widen implicitly to `float64` when required by context. Larger integers (`int64`) require an explicit cast due to potential precision loss (see §3).

```
int8, int16, int32 → float64   (implicit)
int64 → float64                (explicit cast required)
```

### Widening at Assignments and Call Sites

Widening occurs automatically at:
- Variable assignments: `int64 x = someInt32Value;`
- Method argument passing: a parameter of type `int64` accepts an `int32` argument.
- Return statements: a method returning `float64` may return a `float32` expression.

```solix
int32 a = 100;
int64 b = a;      // implicit widening: CONV_I64 emitted

float32 f = 1.5f;
float64 d = f;    // implicit widening: CONV_F64 emitted
```

---

## 3. Narrowing Conversions

**Narrowing** conversions may lose information and require an **explicit cast expression** (see `cast_expression.md`). The compiler will not insert them implicitly; doing so is a compile-time type error.

```solix
int64 big = 300L;
int8  small = (int8) big;   // explicit cast required; value wraps to 44
```

Narrowing conversions include any assignment or argument-passing in the direction opposite to the widening chains in §2, and any conversion between signed and unsigned types of the same width (e.g., `int32` → `uint32`).

| From        | To           | Risk                              |
|-------------|--------------|-----------------------------------|
| `int64`     | `int32`/`int16`/`int8` | Value truncation, sign change |
| `float64`   | `float32`    | Precision loss, overflow to ±Inf  |
| `float64`   | `int64`      | Fractional part discarded, range clamped |
| `int32`     | `uint32`     | Negative values become large positives |
| any wider   | any narrower | Truncation of high bits           |

> [!CAUTION]
> Narrowing casts do not throw at runtime. Out-of-range values silently wrap or truncate. Validate ranges manually before narrowing if correctness is required.

---

## 4. Reference Types

A **reference type** is any type whose values are heap addresses (pointers). Reference types include:

- **Classes** — declared with `class`, heap-allocated with `new`.
- **Interfaces** — declared with `interface`; a value of interface type is a reference to a class instance that implements it.
- **Arrays** — `T[]` for any type `T` (primitive or reference); heap-allocated with `new T[n]` or array literals.

Reference-type variables hold either a valid heap address or `null`. Dereferencing `null` raises a `NullPointerException` at runtime.

Reference types participate in ARC (Automatic Reference Counting). Every live reference to an object keeps its reference count above zero; when the count reaches zero the object is immediately deallocated (see the VM ISA specification, §6).

---

## 5. Null Safety

`null` is a valid value for any reference type (class, interface, or array). It is **not** valid for any primitive type. Assigning `null` to a primitive variable is a compile-time type error.

```solix
MyClass obj = null;   // valid — reference type
int32   n   = null;   // ERROR: cannot assign null to primitive type int32
```

The compiler does **not** perform flow-sensitive null analysis. All null checks must be performed explicitly at runtime by the programmer:

```solix
if (obj != null) {
    obj.doSomething();
}
```

---

## 6. Array Types

### Syntax

`T[]` denotes an array of elements of type `T`. Both primitive and reference element types are supported:

```solix
int32[]    numbers;
MyClass[]  objects;
float64[]  data;
```

### Creation

Arrays are heap-allocated with `new T[n]`, where `n` is an `int32` or `int64` expression specifying the number of elements. All elements are zero-initialized on allocation (primitives to `0`/`0.0`/`false`, references to `null`).

```solix
int32[]  primes = new int32[10];
MyClass[] items = new MyClass[5];
```

### Length

The number of elements in an array is accessed via the `.length` property. It returns an `int64`.

```solix
int64 len = primes.length;   // 10
```

### Element Access

Array elements are accessed via zero-indexed subscript notation. Access is **bounds-checked** at runtime: an index less than zero or greater than or equal to `length` throws an `IndexOutOfBoundsException`.

```solix
primes[0] = 2;
primes[1] = 3;
int32 first = primes[0];   // 2
```

### Memory Layout

At the VM level, an array object is a heap allocation whose first field word stores the element count, followed by one word per element (see VM ISA §5).

---

## 7. Type Compatibility Rules

### Assignment Compatibility

An expression of type `S` is **assignment-compatible** with a target type `T` if any of the following holds:

1. `S` is identical to `T`.
2. `S` is a primitive type that widens implicitly to `T` (see §2).
3. `S` is a class type and `T` is a class or interface type, and `S` is a subtype of `T` (see §8).
4. `S` is `null` and `T` is a reference type.

If none of these conditions hold, an explicit cast is required; if the cast is also type-unsafe, it is rejected at compile time or checked at runtime (for reference downcasts).

### Method Argument Compatibility

Each actual argument must be assignment-compatible with the corresponding formal parameter type. Solix does not support variadic arguments or argument default values; every call site must supply exactly the declared number of arguments.

### Operator Operand Compatibility

Binary arithmetic and comparison operators require both operands to be of a compatible numeric type. If the operands differ, the narrower operand is widened to match the wider operand's type before the operation.

---

## 8. Nominal Subtyping

Solix uses **nominal subtyping**: type compatibility is determined by explicit declarations, not structural similarity.

### Single Inheritance

A class may extend at most one other class using the `extends` keyword. The inheritance chain forms a linear hierarchy. A class that does not explicitly extend another implicitly extends the root class `std.Object`.

```solix
class Animal {
    public virtual void speak() { ... }
}

class Dog extends Animal {
    public override void speak() { ... }
}
```

`Dog` is a subtype of `Animal`. A value of type `Dog` is assignment-compatible with a variable of type `Animal`.

### Interface Conformance

A class may implement any number of interfaces using the `implements` keyword, separated by commas:

```solix
class Cat extends Animal implements Serializable, Comparable {
    ...
}
```

`Cat` is a subtype of both `Serializable` and `Comparable`. A reference to a `Cat` instance may be stored in a variable of either interface type.

### Subtype Relation

The subtype relation `S <: T` is the reflexive, transitive closure of:
- `S extends T` (direct superclass), and
- `S implements T` (direct interface conformance).

All classes implicitly satisfy `AnyClass <: std.Object`.

---

## 9. Method Override Rules

A method in a subclass **overrides** a method in a superclass when:

1. Both methods have the **same name**.
2. Both methods have the **same number of parameters** with **identical parameter types** (in declaration order).
3. The overriding method's return type is **identical to** or a **subtype of** (covariant) the overridden method's return type.
4. The superclass method is declared `virtual` or `abstract`.
5. The subclass method is declared `override`.

Failure to satisfy condition 4 or 5 when the signatures match is a compile-time error. A method declared `override` that does not actually override any superclass method is also a compile-time error.

```solix
class Base {
    public virtual Animal create() { return new Animal(); }
}

class Derived extends Base {
    public override Dog create() { return new Dog(); }  // covariant return: Dog <: Animal
}
```

### Abstract Methods

An `abstract` method declares a signature with no body. A concrete (non-abstract) subclass must provide an `override` for every inherited abstract method. Invoking an unimplemented abstract method at runtime raises a `THROW_ABSTRACT` fault.

---

## 10. Template & Generic System

Solix supports a lightweight **template system** for parameterizing classes and methods over types. Templates are instantiated at compile time; there is no runtime type erasure.

### Class Templates

A class is parameterized by appending `<T>` (or multiple parameters `<T, U>`) after the class name. The type parameter `T` may be used as a field type, method parameter type, or return type within the class body.

```solix
class Box<T> {
    private T value;

    public Box(T value) {
        this.value = value;
    }

    public T get() {
        return this.value;
    }
}
```

Instantiation supplies a concrete type argument:

```solix
Box<int32> intBox = new Box<int32>(42);
Box<MyClass> objBox = new Box<MyClass>(new MyClass());
```

### Method Templates

Individual methods may also be parameterized independently of their enclosing class:

```solix
class Utils {
    public static <T> T identity(T value) {
        return value;
    }
}
```

### Type Deduction

When all type arguments can be inferred from the supplied arguments, the type parameter may be omitted at the call site:

```solix
int32 x = Utils.identity(42);         // T deduced as int32
MyClass obj = Utils.identity(new MyClass());  // T deduced as MyClass
```

Type deduction fails if the argument types are ambiguous or do not uniquely determine the type parameters. In that case the type argument must be supplied explicitly.

### Array Type as a Built-In Template

`T[]` is the canonical built-in generic type representing an array parameterized over element type `T`. It follows the same subtype rules: `Dog[] <: Animal[]` if `Dog <: Animal`.

---

## 11. Type Resolution Order

When the compiler encounters an unqualified type name `N`, it resolves it by searching the following scopes **in order**, stopping at the first match:

| Priority | Scope                        | Description                                                                 |
|----------|------------------------------|-----------------------------------------------------------------------------|
| 1        | **Local variables**          | Parameters and local variables declared in the current method scope         |
| 2        | **Explicit imports**         | Names brought into scope by `import pkg.ClassName` statements               |
| 3        | **Wildcard imports**         | Names brought into scope by `import pkg.*` statements                       |
| 4        | **Current package**          | All top-level classes declared in the same package as the current file      |
| 5        | **Global / std scope**       | The built-in standard library (`std.*`) implicitly available everywhere     |
| 6        | **Suffix matching**          | Unqualified name matched against the suffix of fully-qualified names in the classpath |

If a name matches at multiple levels of the same priority (e.g., two wildcard imports both export `Foo`), the compiler emits an ambiguity error and requires explicit qualification.

Fully-qualified names (e.g., `com.example.Foo`) bypass the resolution order and are resolved directly.

```solix
package com.example;

import std.io.InputStream;    // explicit import — priority 2
import std.util.*;            // wildcard  import — priority 3

class Demo {
    public void run() {
        InputStream s = ...;  // resolved via explicit import (priority 2)
        List<int32> xs = ...; // resolved via wildcard import (priority 3)
    }
}
```

---

## 12. Function Pointer Types

Solix supports primitive function pointers with explicit type syntax cleanly separated from variable identifiers:

$$\text{Type: } \texttt{<return\_type>(*)(<arguments\_types>)} \quad\quad \text{Variable: } \texttt{<name>}$$

### Syntax and Characteristics

- **Signature**: `<return_type>(*)(<param_type_1>, <param_type_2>, ...)`
- **Void Arguments**: Empty argument list `()` represents zero arguments: `void(*)()`.
- **Value Semantics**: Function pointer values occupy 64 bits (8 bytes) on the evaluation stack and local frames. The lower 32 bits represent the bytecode instruction pointer (`target_ip`), while the upper 32 bits hold the environment address (`0` for stateless function pointers).
- **Size**: `sizeof(<ret>(*)(<args>))` evaluates to `8` bytes at compile time.
- **Reference Management**: Unlike heap object references (`std.string`, classes, arrays), function pointers are primitive 64-bit scalar values and do not produce ARC `INC_REF` or `DEC_REF` bytecode operations on their own.

### Usage Example

```solix
alias BinaryOp = int32(*)(int32, int32);

public class Math {
    public static int32 add(int32 a, int32 b) {
        return a + b;
    }
}

static int32 main() {
    BinaryOp op = Math.add;
    int32 result = op(10, 20); // 30
    return result == 30 ? 0 : 1;
}
```

---

## 13. First-Class Lambdas and Closures

Solix unifies function pointers and closures into the same 64-bit primitive callable type `<return_type>(*)(<param_types>)`.

### Syntax

Lambdas support explicit captures in brackets, parameter declarations, optional return type specifications, and expression or block bodies:

```solix
// Stateless lambda (zero heap allocation, env_address = 0)
int32(*)(int32, int32) add = [](int32 a, int32 b) => a + b;

// Capturing local variables by value
int32 factor = 5;
int32(*)(int32) mult = [factor](int32 x) => x * factor;

// Multi-line block body with explicit return type
int32(*)(int32, int32) compute = [factor](int32 a, int32 b) : int32 {
    int32 sum = a + b;
    return sum * factor;
};

// Capturing `this` in instance methods
public int32(*)(int32) getHandler() {
    return [this](int32 x) => x + this.offset;
}
```

### Word Representation & Memory Model

- **Word Packing**:
  $$\texttt{callable} = (\texttt{env\_address} \ll 32) \mid \texttt{target\_ip}$$
  - Stateless lambdas (`[]`) have `env_address = 0`. They behave identically to primitive function pointers with zero heap allocations.
  - Closures with captures allocate an environment array on the dynamic heap (`ALLOC_DYNAMIC`).
- **Environment Layout**:
  - `heap[env + 0]`: Reference bitmask (`ref_mask`), where bit `i` indicates whether capture `i` is an ARC reference.
  - `heap[env + 1]`: Callable bitmask (`callable_mask`), where bit `i` indicates whether capture `i` is a nested closure callable.
  - `heap[env + 2 + i]`: Stored capture value `i`.
- **Automatic Reference Counting (ARC)**:
  - Capturing object references or nested callables increments their reference count upon environment creation (`INC_REF` / `INC_REF_CALLABLE`).
  - When closure variables are reassigned or exit their lexical scope, the compiler emits `DEC_REF_CALLABLE`.
  - When the closure environment's ref count drops to zero, `decrease_reference_callable` recursively decrements all captured reference objects and nested closures via the bitmasks and releases the environment block.
- **Invocation & Capture Unpacking**:
  - Invoking a closure via `op(...)` saves the previous `active_closure_env` on the call stack frame and sets `active_closure_env` to the upper 32 bits of the callable word.
  - The synthesized lambda method prologue executes `UNPACK_CAPTURES`, copying capture values from `heap[active_closure_env + 2 + i]` directly into the lambda's local frame slots.
  - On method return or stack unwinding, `active_closure_env` is restored from the call frame.


