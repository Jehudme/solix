# Automatic Reference Counting (ARC) Internals

## 1. Overview

Solix uses deterministic Automatic Reference Counting (ARC) for heap memory management. Unlike tracing garbage collectors (such as Java or Go), Solix incurs no stop-the-world pauses. Objects are deallocated immediately at the exact instant their reference count reaches zero, and their memory is recycled or returned to the free block pool.

---

## 2. Heap Object Memory Layout

Every heap allocation in Solix is structured as a contiguous block of 64-bit words in the VM `Memory::heap` array:

```text
Address - 1:  [  Block Size (32 bits)  |  Reference Count (32 bits)  ]   <-- Object Header Word
Address + 0:  [  VTable ID (32 bits)   |  Reserved / Padding (32 bits)]   <-- Field 0 / VTable Pointer
Address + 1:  [  Instance Field 0 (64 bits)                           ]
Address + 2:  [  Instance Field 1 (64 bits)                           ]
...
```

- **Header Word (`heap[address - 1]`)**:
  - Low 32 bits: Active reference counter (`ref_count`).
  - High 32 bits: Allocated block size in 64-bit words (`blk_size`).
- **Initial State**: When `ALLOC_DYNAMIC` executes, `ref_count` is set to `1`.

---

## 3. Reference Count Operations

### 3.1. `INC_REF`
- Increments the reference count of the object at the given address:
  ```cpp
  uint64_t header = heap[address - 1];
  uint32_t ref_count = (uint32_t)(header & 0xFFFFFFFF);
  ref_count++;
  heap[address - 1] = (header & 0xFFFFFFFF00000000ULL) | ref_count;
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
