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

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Cycle Breaking with Weak References
```solix
class Parent { Child c; }
class Child { weak Parent p; }

void test() {
    Parent p = new Parent();
    Child c = new Child();
    p.c = c;
    c.p = p; // Weak back-pointer: cycle is broken!
}
```
*Expected Result*: Both objects are deallocated when `p` and `c` exit scope.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Duplicate Field Identifier
```solix
class Item {
    int32 count;
    float64 count; // Error: duplicate
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Field 'count' is already declared in class 'Item'
```

### Case 4.2: Field Access on Null Reference (Runtime Fault)
```solix
void test() {
    Item item = null;
    item.count = 5; // Null dereference
}
```
*Expected Runtime Exception*:
```text
[FATAL VM PANIC] NullReferenceException: Attempted to write field on null object reference
```
