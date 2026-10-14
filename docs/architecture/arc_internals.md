# Automatic Reference Counting (ARC) Internals

## 1. Overview

Solix uses deterministic Automatic Reference Counting (ARC) for heap memory management. Unlike tracing garbage collectors (such as Java or Go), Solix incurs no stop-the-world pauses. Objects are deallocated immediately at the exact instant their reference count reaches zero, and their memory is recycled or returned to the free block pool.

---

## 2. Heap Object Memory Layout

Every heap allocation in Solix is structured as a contiguous byte block in the VM `Memory::heap` (`std::vector<uint8_t>`) byte buffer:

```text
Address - 8:  [ BlockTag (4 bits) | Block Size in Bytes (28 bits) ]
Address - 4:  [ Reference Count (32 bits)                         ]
Address + 0:  [ VTable ID (64 bits)                               ]   <-- Field 0 / VTable Pointer
Address + 8:  [ Instance Field 0 (natural alignment)              ]
...
```

- **Header Bytes**:
  - `address - 8`: 32-bit word combining top 4 bits for `BlockTag` and 28 bits for allocated block byte size.
  - `address - 4`: 32-bit active reference counter (`ref_count`).
- **Initial State**: When `ALLOC_DYNAMIC` executes, `ref_count` is set to `1`.
- For in-depth details on field alignment, property width encodings, and primitive arrays, refer to [`memory_model.md`](memory_model.md).

---

## 3. Reference Count Operations

### 3.1. `INC_REF`
- Increments the reference count of the object at the given address:
  ```cpp
  uint32_t ref_count = read_u32(address - 4);
  ref_count++;
  write_u32(address - 4, ref_count);
  ```
- Emitted when an object reference is copied to another variable, passed by value, or stored into a strong field.

### 3.2. `DEC_REF`
- Decrements the reference count of the object at the given address.
- When `ref_count` reaches zero:
  1. Recursively calls `DEC_REF` on all child reference fields stored within the object.
  2. Clears all weak references pointing to this object by nullifying the registered slots.
  3. Returns the allocated block address to `Memory::free_blocks` for instant reuse.

---

## 4. Scope-Exit LIFO Destruction

The compiler's semantic `Binder` (Pass 2) tracks all reference variables in lexical scope. When an execution thread leaves a lexical block:
- The compiler emits `DEC_REF` instructions in reverse order of declaration (LIFO).
- In the presence of early exits (`break`, `continue`, `return`), the compiler inserts cleanup chains traversing all enclosing intermediate scopes up to the target boundary.

---

## 5. Weak Reference Mechanics

To eliminate memory leaks caused by circular references (e.g. parent-child tree nodes), Solix provides the `weak` modifier:
- Storing an address into a `weak` field emits `WEAK_SET_PROPERTY` rather than `SET_PROPERTY`.
- The runtime VM maintains a registry:
  ```cpp
  std::unordered_map<Address, std::unordered_set<Address>> weak_references;
  ```
  mapping the target object address to the set of heap memory slots holding a weak pointer to it.
- When the target object is freed by `decrease_reference()`, the runtime traverses its registered weak slots and writes `0` (`null`) to each slot, guaranteeing that weak references never become dangling pointers.
