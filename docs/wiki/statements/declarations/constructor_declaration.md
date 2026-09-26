# ConstructorDeclaration

## 1. Overview & Purpose

A `ConstructorDeclaration` initializes newly allocated class instances on the heap. Constructors share the exact name of their class and support member initializer lists (`: super(args), field(val)`).

Register slot 0 is assigned to `this`. If a class does not declare a constructor, the compiler automatically synthesizes a default parameterless constructor.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
class Point {
    int32 x;
    Point(int32 x) : x(x) {}
}
```

```bytecode
// Compiled VM Bytecode (Point constructor)
GET_LOCAL 1                 // Load parameter 'x'
GET_LOCAL 0                 // Load 'this' (slot 0)
SET_PROPERTY 0              // this.x = x
RETURN                      // Constructor returns 'this'
```
