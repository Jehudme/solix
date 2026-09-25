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

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Instantiation with Overloaded Constructor
```solix
class Item {
    int32 val;
    Item() { this.val = 0; }
    Item(int32 v) { this.val = v; }
}

void test() {
    Item i1 = new Item();
    Item i2 = new Item(42);
}
```
*Expected Result*: `i1.val` is 0; `i2.val` is 42.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Instantiating Abstract Class
```solix
abstract class Base {}
Base b = new Base(); // Error
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot instantiate abstract class 'Base'
```
