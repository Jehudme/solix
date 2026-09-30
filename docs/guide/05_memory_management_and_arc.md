# Memory Management and ARC

Solix uses **Automatic Reference Counting (ARC)** for heap memory management. There is no garbage collector pause, no manual `free`, and no smart-pointer boilerplate. The compiler automatically inserts reference-counting instructions so that objects are destroyed as soon as their last reference disappears.

---

## Table of Contents

1. [How ARC Works](#how-arc-works)
2. [Object Header Layout](#object-header-layout)
3. [INC\_REF and DEC\_REF](#inc_ref-and-dec_ref)
4. [LIFO Scope-Exit Destruction](#lifo-scope-exit-destruction)
5. [Strong References (default)](#strong-references-default)
6. [Weak References (`weak`)](#weak-references-weak)
7. [Breaking Circular References](#breaking-circular-references)
8. [What Happens at Object Destruction](#what-happens-at-object-destruction)
9. [Free Block Reuse](#free-block-reuse)

---

## How ARC Works

Every class instance allocated on the heap carries a **reference count** in its header word. The runtime maintains this count:

- **Incremented (`INC_REF`)** when a new reference to the object is stored.
- **Decremented (`DEC_REF`)** when a reference goes out of scope or is reassigned.
- When the count **drops to zero**, the runtime calls the object's destructor and reclaims memory immediately — no deferred collection.

The Solix **Binder (Pass 2)** is responsible for injecting `INC_REF` and `DEC_REF` instructions at the appropriate points in the compiled output. User code never writes reference-counting calls manually.

---

## Object Header Layout

Each heap-allocated object begins at a **header word** immediately before its first field. The header is a 64-bit integer (`uint64_t`) whose two halves encode:

```
 63                    32 31                     0
┌─────────────────────────┬─────────────────────────┐
│        vtable_id        │       ref_count         │
│       (high 32 bits)    │       (low 32 bits)     │
└─────────────────────────┴─────────────────────────┘
```

| Bits | Field | Description |
|------|-------|-------------|
| 0–31 | `ref_count` | Number of live references to this object |
| 32–63 | `vtable_id` | Identifier used by `CALL_VIRTUAL` to dispatch virtual methods |

Immediately after the header word, the object's fields are laid out in declaration order, one 64-bit word per field.

**Example — `Rectangle` (two `float64` fields):**

```
Address N-1 : [vtable_id | ref_count]   ← header
Address N   : width  (float64)
Address N+1 : height (float64)
```

The GC address returned by `new` points to `N` (the first field), not to the header. The runtime accesses the header at `address - 1`.

---

## INC\_REF and DEC\_REF

These are VM opcodes emitted by the Assembler based on binding information.

### `INC_REF`

Pops an address off the stack, increments the reference count at `heap[address - 1] & 0xFFFFFFFF`, then pushes the address back.

Emitted whenever:
- A reference type variable is assigned.
- A reference type value is passed as a method argument.
- A reference type is stored into an object field or array slot.

### `DEC_REF`

Pops an address, decrements the reference count. If count reaches zero, immediately deallocates the object (see [What Happens at Object Destruction](#what-happens-at-object-destruction)).

Emitted whenever:
- A local variable holding a reference goes out of scope.
- A reference type variable is reassigned (the old value is decremented before the new value is stored).
- A return value that is a reference type is decremented if the caller does not consume it.

---

## LIFO Scope-Exit Destruction

When execution exits a block (`{ }`), variables declared in that block are destroyed in **Last-In, First-Out** order — the last-declared variable is decremented first.

```solix
{
    String a = new String("alpha");   // refcount(a) = 1
    String b = new String("beta");    // refcount(b) = 1
    String c = new String("gamma");   // refcount(c) = 1
    // ... use a, b, c ...
}  // Exit scope: DEC_REF(c), DEC_REF(b), DEC_REF(a)
   // All objects destroyed here (if no other references exist)
```

This predictable, deterministic destruction order is one of ARC's key advantages over tracing GC.

---

## Strong References (default)

By default, every reference-type field, local variable, and array slot is a **strong reference**. Holding a strong reference keeps the referenced object alive.

```solix
public class Node {
    public String value;
    public Node next;   // strong reference to the next node

    public Node(String val) {
        this.value = val;
        this.next  = null;
    }
}
```

When `node` goes out of scope, the chain is destroyed: decrementing `node` decrements its `next`, which decrements that node's `next`, and so on.

---

## Weak References (`weak`)

A **weak reference** does not contribute to the reference count. If the only remaining references to an object are weak, the object is still destroyed. When the object is destroyed, all weak references that pointed to it are **automatically set to null**.

Declare a weak field using the `weak` keyword:

```solix
public class Widget {
    weak Widget parent;   // Does NOT keep parent alive
    String name;

    public Widget(String name) {
        this.name = name;
        this.parent = null;
    }
}
```

Internally, `weak` fields use the `WEAK_SET_PROPERTY` opcode instead of `SET_PROPERTY`. The runtime maintains a **weak reference registry**: a map from `target_address` → `set of slot addresses`. When an object is deallocated, every slot in its registry set is zeroed.

---

## Breaking Circular References

A **circular reference** is a cycle where object A holds a strong reference to B and B holds a strong reference back to A. Under pure ARC, neither object's count ever reaches zero and both leak.

**Problem:**

```solix
public class Parent {
    public Child child;   // strong — keeps child alive

    public Parent() {
        this.child = null;
    }
}

public class Child {
    public Parent parent;  // strong — creates a cycle!

    public Child() {
        this.parent = null;
    }
}
```

If a `Parent` and `Child` reference each other, removing all external references still leaves them with `ref_count = 1` (from each other), so they are never freed.

**Solution — use `weak` on the back-reference:**

```solix
public class Parent {
    public Child child;         // strong — parent owns the child

    public Parent() {
        this.child = null;
    }
}

public class Child {
    weak Parent parent;         // weak — does NOT prevent parent's destruction

    public Child() {
        this.parent = null;
    }
}

// Usage
Parent p = new Parent();       // refcount(p) = 1
Child  c = new Child();        // refcount(c) = 1
p.child  = c;                  // refcount(c) = 2 (p.child strong ref)
c.parent = p;                  // refcount(p) = 1 (weak — does NOT increment)

// When p goes out of scope: refcount(p) -> 0 -> p destroyed
//   -> p.child DEC_REF -> refcount(c) -> 1 (only c variable remains)
// When c goes out of scope: refcount(c) -> 0 -> c destroyed
//   -> c.parent was weak: already null (set by runtime when p was destroyed)
```

The rule of thumb: **the "owner" holds a strong reference; the "back-pointer" or "observer" holds a weak reference.**

---

## What Happens at Object Destruction

When `ref_count` reaches zero:

1. The runtime walks all **reference-type fields** in the object's vtable layout and calls `DEC_REF` on each.
2. Any **weak reference registry** entries pointing to this address are nullified (set to 0).
3. The header word at `address - 1` is overwritten with a freed-block descriptor.
4. The memory block is added to the **free block list** for reuse.

Field cleanup is performed automatically — there are no user-defined destructors in Solix.

---

## Free Block Reuse

When memory is freed, the block is added to a `free_blocks` list maintained in the `Memory` subsystem. Future allocations perform a **best-fit search** of this list:

1. Scan `free_blocks` for the smallest block that fits the requested size.
2. If found: remove from the list, optionally split the remainder, and return.
3. If no fit: allocate from the top of the dynamic heap.

This strategy minimizes fragmentation and keeps average allocation cost low for programs that create and destroy many objects of similar sizes.

> [!TIP]
> Design hot paths to reuse objects rather than repeatedly allocating and discarding them. While ARC eliminates GC pauses, frequent small allocations still have overhead from heap traversal and reference-count updates.
