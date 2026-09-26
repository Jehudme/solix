# ClassDeclaration

## 1. Overview & Purpose

A `ClassDeclaration` defines a reference type with encapsulated fields, constructors, methods, and operators. Solix supports single class inheritance (`extends`), multiple interface implementation (`implements`), and abstract classes.

Class instances are allocated on the heap as reference-counted objects managed by Automatic Reference Counting (ARC). Methods participate in dynamic virtual dispatch via Virtual Method Tables (VTables).

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Memory Layout
An instance has an 8-byte header followed by contiguous field offsets:
```text
+-------------------------------------------------------------+
| Header: uint32_t ref_count (ARC) | uint32_t vtable_id       |
+-------------------------------------------------------------+
| Field 0 (Superclass fields...)                              |
| Field 1 (Declared fields...)                                |
+-------------------------------------------------------------+
```

### Bytecode Disassembly Example
```solix
// Solix Code
class Dog extends Animal {
    void speak() { Console.println("Woof"); }
}
Animal a = new Dog();
a.speak();
```

```bytecode
// Compiled VM Bytecode
// 1. Instantiation
ALLOC_DYNAMIC 16            // Allocates header + fields
SET_VTABLE <Dog_vtable_id>  // Sets vtable pointer
CALL Dog.init()             // Runs constructor (ref_count = 1)
SET_LOCAL 1                 // Stored into 'a'

// 2. Virtual Method Dispatch
GET_LOCAL 1                 // Load 'a'
CALL_VIRTUAL <speak_slot_id>// Dynamic dispatch queries Dog VTable!
```
