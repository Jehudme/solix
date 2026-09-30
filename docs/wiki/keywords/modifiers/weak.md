# `weak`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `weak` |
| Category | Modifier |
| Context | Field declarations (reference types only) |
| Related | [`class`](../declarations/class.md), [`static`](static.md) |

`weak` is a memory modifier used to define non-owning reference fields in Solix. Solix uses deterministic Automatic Reference Counting (ARC) to manage heap memory. If two or more objects hold strong references to each other, a retain cycle occurs, preventing their reference counters from reaching zero and causing a memory leak. Marking a reference field `weak` instructs the runtime not to increment the target object's reference counter, breaking cycles while maintaining traceable references that are safely zeroed out when the target object is deallocated.

## 2. Permitted Contexts (Syntax & Grammar)

```
FieldModifier : 'weak' | 'static' | 'const' | AccessModifier
FieldDeclaration : [AccessModifier] ['weak'] Type Identifier [ '=' Expression ] ';'
```

- Permitted exclusively on class instance field declarations of reference types (classes, interfaces).
- Cannot be applied to primitive types (`int32`, `bool`, `char`, etc.) or value types.
- Cannot be applied to local variables or method parameters.
- Cannot be combined with `const` on the same field.

## 3. Semantics & Compiler Rules

- **ARC Non-Owning**: Storing an object reference into a `weak` field does not trigger an `INC_REF` bytecode instruction.
- **Cycle Prevention**: Solix's memory manager registers the address of the weak field with the target object's weak reference list in the runtime.
- **Automatic Nullification**: When the referenced object's reference count drops to 0 and it is freed, the runtime clears all registered weak fields pointing to that object, setting them to `null`.
- Accessing a `weak` reference reads the current pointer directly without retaining unless assigned to a local strong variable.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

public class Node {
    public int32 value;
    public Node next;      // Strong reference: owns child
    public weak Node prev; // Weak reference: does NOT own parent (prevents cycle)

    public Node(int32 value) {
        this.value = value;
    }
}
```

### Idiomatic Usage

```solix
package solix.tree;

import solix.collections.List;

public class TreeNode {
    private string name;
    private weak TreeNode parent; // Weak reference back to parent
    private List<TreeNode> children;

    public TreeNode(string name) {
        this.name = name;
        this.children = new List<TreeNode>();
    }

    public void add_child(TreeNode child) {
        child.parent = this;
        this.children.add(child);
    }

    public TreeNode get_parent() {
        return this.parent;
    }

    public List<TreeNode> get_children() {
        return this.children;
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Applying `weak` to a primitive field | `Type error: 'weak' modifier cannot be applied to primitive type 'int32'` |
| Applying `weak` to a local variable | `Syntax error: 'weak' modifier is only valid on class field declarations` |
| Combining `weak` and `const` | `Semantic error: Field cannot be both 'weak' and 'const'` |

## 6. Related Keywords & Guides

- [`class`](../declarations/class.md) — Reference types where `weak` fields reside
- [Memory Management & ARC Guide](../../../guide/05_memory_management_and_arc.md) — Comprehensive guide to Solix reference counting and cycle breaking
