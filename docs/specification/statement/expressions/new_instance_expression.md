# NewInstanceExpression

## 1. Overview & Purpose

A `NewInstanceExpression` (`new ClassName(args)`) dynamically allocates memory on the heap, initializes the object header (`ref_count = 1`, `vtable_id`), and invokes the matching constructor.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
Widget w = new Widget(5);
```

```bytecode
// Compiled VM Bytecode
ALLOC_DYNAMIC 16            // Allocates memory on heap
SET_VTABLE <Widget_vtable_id>
PUSH_CONST_I32 5            // Argument
CALL Widget.new(int32)      // Runs constructor
SET_LOCAL 1                 // Stored into w
```
