# Tuple Primitive & Variadic Generics Specification

## Overview

The `tuple` type in Solix is a first-class language primitive (not an object class) representing an ordered, fixed-length heterogeneous collection of values. It serves as the foundation for multi-value returns, ad-hoc groupings, and variadic template parameter lowering (`<args...>`).

Because `tuple` is a built-in primitive, the capitalized identifier `Tuple` remains reserved for future high-level object-oriented wrappers (e.g., `solix.core.Tuple` in `solixlib`).

---

## 1. Syntax & Declaration

### 1.1 Tuple Types
Tuple types can be declared either unparameterized (dynamic/heterogeneous) or parameterized with static element types:
```solix
tuple untyped_data;
tuple<int32, string, bool> typed_data;
```

### 1.2 Tuple Instantiation
Tuples are allocated via the `new tuple(...)` expression:
```solix
tuple t = new tuple(42, "hello", true);
```
At runtime, this allocates a contiguous memory block tagged as `BlockTag::TUPLE` holding element count, per-element type tags, and 64-bit value slots.

### 1.3 Element Access & Length
- **Length**: Accessible via the `.length` property (returns `int32` element count).
- **Index Access**: Accessible via array subscript syntax `t[i]`, where `i` is an `int32` expression.
```solix
int32 len = t.length;
int32 first = (int32)t[0];
```
Subscript access outside `[0, length - 1]` throws an `IndexOutOfBoundsException` at runtime.

### 1.4 Runtime Type Inspection & Casting
Elements extracted from a tuple can be inspected and cast dynamically:
- **`instanceof`**: Evaluates whether an element matches a primitive type or class/interface hierarchy.
  ```solix
  if (t[0] instanceof int32) { ... }
  if (t[1] instanceof Worker) { ... }
  ```
- **Explicit Cast**: Coerces the element into the specified target type with runtime validation. Incompatible casts throw a `TypeCastException`.
  ```solix
  string s = (string)t[1];
  ```

---

## 2. Variadic Template Parameter Lowering (`<args...>`)

Function templates can declare a trailing variadic template parameter pack using ellipsis `...`:
```solix
int32 format<args...>(string fmt, args...) {
    for (int32 i = 0; i < args.length; i++) {
        // dynamic element inspection
    }
}
```

### 2.1 Lowering Rules
1. **Parameter Pack**: `<args...>` designates a pack of 0 or more generic types.
2. **Signature Lowering**: A parameter `args...` in the method signature is lowered by the compiler into a single parameter `tuple<args...> args` (or `tuple args`).
3. **Call-Site Bundling**: When callers invoke a variadic function, all trailing arguments corresponding to the variadic pack are automatically bundled by the compiler into a `NEW_TUPLE` expression:
   ```solix
   format("Result: %d %s", 100, "ok");
   // Lowered into:
   // format("Result: %d %s", new tuple(100, "ok"));
   ```
4. **Validation Constraints**:
   - Only one variadic parameter pack is permitted per template declaration.
   - The variadic parameter must be the last parameter in the method signature.

---

## 3. Memory Layout & Virtual Machine Semantics

### 3.1 Heap Layout (`BlockTag::TUPLE = 11`)
Tuples are tracked by the Automatic Reference Counting (ARC) allocator with tag `BlockTag::TUPLE`:

```
+-------------------------------------------------------------+
| Header: Length (4B) | Tag + RefCount (4B)                   |
+-------------------------------------------------------------+
| Element Type Tags: uint32_t types[count]                    |
+-------------------------------------------------------------+
| Values Payload (8-byte aligned): uint64_t values[count]     |
+-------------------------------------------------------------+
```

Each 32-bit type tag encodes:
- `Bit 31 (0x80000000)`: Reference flag (1 = reference type, 0 = primitive type).
- `Bit 30 (0x40000000)`: Class type flag.
- `Bits 0-29`: Primitive code (`TYPE_TAG_INT32`, etc.) or class `vtable_id`.

### 3.2 Bytecode Instructions
| Opcode | Mnemonic | Immediate Operands | Stack Inputs | Stack Outputs | Description |
|---|---|---|---|---|---|
| `0x65` | `NEW_TUPLE` | `u32 count`, `u32[count] type_tags` | `elem0, ..., elemN-1` | `tuple_addr` | Allocates a tuple with specified elements and type tags. |
| `0x66` | `TUPLE_GET` | None | `tuple_addr, index` | `value` | Retrieves element at `index`. Increments ref count if element is reference. |
| `0x67` | `TUPLE_LEN` | None | `tuple_addr` | `count` | Pushes the number of elements in the tuple. |
| `0x68` | `TUPLE_TYPE` | None | `tuple_addr, index` | `type_tag` | Pushes the 32-bit type tag of the element. |
| `0x69` | `TUPLE_CAST` | `u32 target_type_tag` | `tuple_addr, index` | `value` | Verifies and casts element to target type tag; panics on mismatch. |
| `0x6A` | `TUPLE_INSTANCEOF` | `u32 target_type_tag` | `tuple_addr, index` | `bool` | Tests if element satisfies target type tag. |

---

## 4. Deterministic Destructor & RAII Semantics

When a `tuple` instance's reference count drops to 0:
1. **Scope Exit**: Variables going out of scope emit `DEC_REF`.
2. **Reassignment**: Overwriting a tuple variable decrements previous instance.
3. **Exception Unwinding**: Call frames popped during exception propagation invoke `DEC_REF` on all local references, including tuples.

### 4.1 Recursive Child Reclamation
In `Runtime::object_destructor`:
- The runtime iterates through `types[0 ... count - 1]`.
- For every element where `types[i] & 0x80000000` is true (reference type):
  - The runtime calls `decrease_reference(child_addr)`.
  - If `child_addr`'s reference count reaches 0, its destructor (`~ClassName()`) is immediately executed, followed by freeing memory.
- If child elements are nested tuples, destruction cascades recursively with zero memory leaks.
