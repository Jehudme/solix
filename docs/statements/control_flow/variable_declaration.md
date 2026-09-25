# VariableDeclarationStatement

## 1. Overview & Purpose

A `VariableDeclarationStatement` introduces one or more named variables into the current scope. It binds an identifier to a static type, allocates a register slot in the function's activation frame, and optionally assigns an initial value.

In Solix:
- **Primitives (Value Types)**: Numbers, booleans, and characters (`int32`, `float64`, `bool`, `char`) hold raw bits directly in the stack register and require no reference counting.
- **Reference Types**: Classes, interfaces, strings, and **all arrays** (including `int32[]`) hold heap pointers. Initializing a reference variable increments its reference counter (`INC_REF`), and leaving the variable's scope decrements it (`DEC_REF`).

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### How It Compiles
1. The binder resolves the type and checks if `array_depth > 0 || !is_primitive`. If so, it flags `is_reference_type = true`.
2. The binder assigns `memory_index = local_variable_index++`.
3. If an initializer expression is present:
   - Compiles the initializer expression onto the stack top.
   - If `is_reference_type == true`, emits `INC_REF`.
   - Emits `SET_LOCAL <memory_index>`.

### Bytecode Disassembly Example
```solix
// Solix Code
int32 count = 10;
String name = new String("Solix");
```

```bytecode
// Compiled VM Bytecode
PUSH_CONST_I32 10
SET_LOCAL 1                 // Slot 1: count = 10 (value type, no INC_REF)

PUSH_CONST_STRING 0         // "Solix"
CALL String.new(String)     // Returns heap pointer
INC_REF                     // Claim ownership (ref_count = 2)
SET_LOCAL 2                 // Slot 2: name = pointer
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Primitive and Reference Declarations
```solix
int32 a = 1;
float64 b = 2.5;
bool c = true;
String s = new String("hello");
int32[] nums = new int32[5];
```
*Expected Result*: Compiles cleanly; each variable is assigned a contiguous slot in the activation frame.

### Case 3.2: Polymorphic Upcasting
Assigning a derived class instance to a base class variable.
```solix
class Animal {}
class Cat extends Animal {}

void test() {
    Animal a = new Cat(); // Valid polymorphic binding
}
```
*Expected Result*: Passes type checking; `Cat` instance stored in `Animal` slot.

### Case 3.3: Interface Binding
```solix
interface Printable { void print(); }
class Doc implements Printable { void print() {} }

void test() {
    Printable p = new Doc();
}
```
*Expected Result*: Binds instance to interface variable with valid VTable slot.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Type Mismatch in Initializer
```solix
void test() {
    int32 x = "hello"; // Incompatible types
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Type mismatch in variable declaration: expected 'int32', got 'String'
```

### Case 4.2: Duplicate Declaration in Same Scope
```solix
void test() {
    int32 score = 10;
    int32 score = 20; // Error: duplicate
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Variable 'score' is already defined in the current scope
```

### Case 4.3: Unknown Type Name
```solix
void test() {
    NonExistentType obj = null;
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Unknown type 'NonExistentType'
```

### Case 4.4: Declaring Variable as Void
```solix
void test() {
    void placeholder;
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Variable cannot be of type 'void'
```
