# FieldDeclaration

## 1. Overview & Purpose

A `FieldDeclaration` defines state storage within a class instance (instance fields) or globally (static fields). Fields support access modifiers (`public`, `private`, `protected`) and the `weak` ownership modifier.

The `weak` keyword is crucial in Solix: marking a reference field as `weak` stores the pointer without incrementing the target object's reference counter, breaking circular ownership graphs (e.g. parent-child relationships) and preventing permanent memory leaks.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
class Node {
    Node strong_child;
    weak Node weak_parent;
}
node.strong_child = child;
node.weak_parent = parent;
```

```bytecode
// Compiled VM Bytecode
// 1. Strong Field Write
GET_LOCAL 2                 // child
INC_REF                     // Increment ref_count
GET_LOCAL 1                 // node
SET_PROPERTY 0              // Stores strong pointer

// 2. Weak Field Write
GET_LOCAL 3                 // parent
GET_LOCAL 1                 // node
WEAK_SET_PROPERTY 8         // Stores weak pointer (NO INC_REF!)
```
