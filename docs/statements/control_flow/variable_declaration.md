# §12 VariableDeclarationStatement

## 1. Overview & Scope

A `VariableDeclarationStatement` introduces one or more named identifiers into the active lexical declaration space, binding each identifier to a static type, a memory slot within the execution frame, and an optional initializing expression.

In Solix's strongly typed, Automatic Reference Counting (ARC) architecture, a variable declaration is not a passive symbol table entry. It defines the formal contract governing storage representation (scalar immediate vs. heap reference pointer), memory lifecycle (participating in ARC or stack-allocated scalar storage), and type compatibility across all subsequent reads and mutations.

### Syntactic Placement
A `VariableDeclarationStatement` is legally permitted in the following scopes:
1. **Local Block Scope**: Inside any `BlockStatement` within methods, functions, constructors, or control-flow blocks.
2. **Loop Initialization Clauses**: Inside the initialization clause of a `for` loop statement.
3. **Global Translation-Unit Scope**: At the package root level, defining global state (compiled to static data segment slots).
4. **Class & Interface Bodies**: As member fields (governed by `FieldDeclaration`).

---

## 2. Syntax & Production Rules

### Production Rules
```solix
VariableDeclaration   ::= TypeSpecifier Identifier ('=' Expression)? ';'
TypeSpecifier         ::= PrimitiveType | QualifiedType | ArrayType | GenericType
PrimitiveType         ::= 'int8' | 'int16' | 'int32' | 'int64'
                        | 'uint8' | 'uint16' | 'uint32' | 'uint64'
                        | 'float32' | 'float64' | 'bool' | 'char'
QualifiedType         ::= (Identifier '.')* Identifier
ArrayType             ::= TypeSpecifier '[' ']'
GenericType           ::= QualifiedType '<' TypeSpecifier (',' TypeSpecifier)* '>'
```

### Canonical Code Patterns
```solix
// 1. Primitive Scalar Declarations (Value Types)
int32 counter = 0;
float64 factor = 1.5;
bool is_valid = true;
char delimiter = ';';
int32 uninit_value; // Uninitialized, defaults to 0

// 2. Reference Type Declarations (ARC Owning Pointers)
String name = new String("Solix");
MyClass instance = new MyClass();
MyInterface handler = new ConcreteHandler(); // Polymorphic interface binding

// 3. Array Declarations (Reference Types)
int32[] numbers = new int32[10];
String[] tokens = ["alpha", "beta", "gamma"];
float64[][] matrix = new float64[4][4];

// 4. Parameterized Generic Declarations
List<String> items = new List<String>();
Map<String, int32> lookup = new Map<String, int32>();
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Visibility Rules
- **Lexical Downward Reach**: A variable becomes visible immediately *after* its initializing expression is evaluated, extending downward throughout the remaining statement sequence of its enclosing block.
- **Self-Reference Prohibition**: An identifier cannot be referenced within its own initializing expression (e.g. `int32 x = x + 1;` is ill-formed).
- **Scope Boundary Confinement**: When the enclosing block terminates, the variable's identifier is removed from the active `SymbolTable`. Any subsequent access from outer or sibling scopes results in an unresolved symbol error.

### 3.2 Shadowing Rules
- A local variable declared in an inner block may share the identifier of a variable declared in an outer enclosing block.
- The inner declaration shadows the outer declaration: all subsequent reads and writes within the inner block bind to the inner variable's frame register.
- **Duplicate Declaration Rejection**: Declaring two variables with identical identifiers in the same immediate declaration space is rejected by the compiler.

### 3.3 Lifetime (Static Extent)
- Each local variable declaration receives a monotonically increasing frame index (`memory_index = local_variable_index++`).
- Value types occupy 8-byte slots in the activation frame holding raw numeric or boolean bits.
- Reference types occupy 8-byte slots in the activation frame holding heap object pointers.
- The static extent terminates at the closing brace of the declaring block, at which point reference types undergo ARC destruction.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Normal Completion
Execution of a `VariableDeclarationStatement` proceeds through the following operational steps:
1. **Uninitialized Declarations**:
   - If no initializer expression is present, the VM assigns the default zero-bit pattern to the variable's frame register:
     - Integral and floating-point types initialize to `0` or `0.0`.
     - Boolean types initialize to `false` (`0`).
     - Reference types initialize to `null` (`0x0`).
   - Normal completion occurs immediately; no bytecode opcodes are emitted.
2. **Initialized Declarations**:
   - The initializer expression is evaluated to completion, leaving the resulting value on top of the operand stack.
   - **ARC Ownership Claim**: If `is_reference_type == true`:
     - The VM executes `OpCode::INC_REF`, incrementing the heap object's internal reference counter.
   - **Frame Storage**:
     - The VM executes `OpCode::SET_LOCAL <memory_index>`, popping the stack top and storing the value into the assigned frame register.
   - Execution proceeds to the subsequent statement.

### 4.2 Abrupt Completion
A `VariableDeclarationStatement` completes abruptly if the evaluation of its initializer expression completes abruptly:
- If the initializer expression throws an exception (e.g. `int32 x = compute_risk();` where `compute_risk()` throws), the assignment does not occur.
- The frame register for this variable remains uninitialized or retains null.
- Control transfers immediately to the exception unwinding trampoline of the enclosing block.

### 4.3 Exception Unwinding & Trampolines
- When an exception unwinds through a block containing an initialized reference variable, that variable is included in the block's **Exception Cleanup Segment**.
- Unwinding executes `OpCode::GET_LOCAL <memory_index>` followed by `OpCode::DEC_REF`, guaranteeing that partially executed blocks do not leak newly allocated variables.

---

## 5. Memory Model & ARC Invariants

### 5.1 Value Types vs. Reference Types Invariant
In Solix, the boundary between value types and reference types is strictly formalized:
```cpp
Node *type_decl = global_scope.resolve(n.type_info.name);
n.is_reference_type = !(type_decl && type_decl->is_primitive && n.type_info.array_depth == 0);
```
- **Scalar Primitives**: `array_depth == 0` and `is_primitive == true`. Stored inline in stack frames. Incur 0 ARC overhead.
- **Arrays of Primitives**: (e.g. `int32[]`). Even though elements are primitives, the array buffer itself is a heap-allocated reference object (`array_depth > 0`). Hence, `is_reference_type == true`, and it is tracked via ARC.
- **Class & Interface Instances**: Always heap-allocated reference types. `is_reference_type == true`.

### 5.2 ARC Reference Counter Lifecycle
```text
Declaration with Initializer:
  1. Evaluate Initializer Expression ──► Pushes Object Pointer to Stack
  2. OpCode::INC_REF                 ──► Object ref_count increments (1 -> 2)
  3. OpCode::SET_LOCAL <slot>        ──► Frame Register stores Object Pointer

Block Termination (Normal or Unwinding):
  1. OpCode::GET_LOCAL <slot>        ──► Pushes Object Pointer to Stack
  2. OpCode::DEC_REF                 ──► Object ref_count decrements (2 -> 1 or 1 -> 0)
  3. If ref_count == 0               ──► Runtime frees memory buffer via RELEASE
```

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Type Compatibility Violation
An initializer expression must be assignable to the declared variable type (`is_assignable`).
```solix
void test_type_error() {
    int32 count = "twenty"; // Incompatible types
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Type mismatch in variable declaration: expected 'int32', got 'String'
```

### Rule 6.2: Duplicate Variable Identifier
Declaring a variable with an identifier already declared in the same immediate block scope is illegal.
```solix
void test_duplicate() {
    int32 score = 100;
    int32 score = 200; // Duplicate declaration
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Variable 'score' is already defined in the current scope
```

### Rule 6.3: Unresolved Type Identifier
Declaring a variable using a type name that cannot be resolved in any imported package or symbol table is rejected.
```solix
void test_unknown() {
    NonExistentClass item = null;
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Unknown type 'NonExistentClass'
```

### Rule 6.4: Illegal Void Variable
Variables cannot be declared with type `void`.
```solix
void test_void() {
    void placeholder;
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Variable cannot be of type 'void'
```

### Rule 6.5: Implicit Invalid Downcasting
Assigning a superclass reference to a subclass variable without an explicit cast is prohibited.
```solix
class Base {}
class Derived extends Base {}

void test_downcast() {
    Base b = new Base();
    Derived d = b; // Prohibited: requires explicit cast
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Type mismatch in variable declaration: expected 'Derived', got 'Base'
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Heap Out-of-Memory During Object Instantiation
If the heap memory allocator fails to allocate memory for the object in the initializer expression, the VM raises an `OutOfMemoryException`.
```solix
void test_oom() {
    int32[] huge = new int32[2147483647];
}
```
*Runtime Fault*:
```text
[FATAL VM PANIC] OutOfMemoryException: Failed to allocate 8589934588 bytes on heap
```

---

## 8. Conformance & Verification Examples

### Example 8.1: Primitive Array Classification as ARC Reference
```solix
// Conformance Test: Primitive arrays must participate in ARC deallocation
void verify_primitive_array_arc() {
    {
        int32[] data = new int32[64];
        data[0] = 42;
    }
    // Block exit must emit DEC_REF for data
}
```
*Verification Invariant*: Disassembly verifies that `INC_REF` is emitted when storing into `data`, and `DEC_REF` is emitted when exiting the block.

### Example 8.2: Polymorphic Interface Variable Assignment
```solix
interface Reader {
    int32 read_byte();
}
class FileReader implements Reader {
    int32 read_byte() { return 1; }
}

void verify_interface_assignment() {
    Reader r = new FileReader(); // Valid interface binding
    int32 val = r.read_byte();
}
```
*Verification Invariant*: Static analysis succeeds; dynamic dispatch invokes `FileReader.read_byte` via interface VTable slot.

### Example 8.3: Lexical Masking Across Nested Scopes
```solix
int32 outer = 50;
{
    String outer = new String("masked");
    Console.println(outer); // Prints: "masked"
}
Console.println(outer);     // Prints: 50
```
*Verification Invariant*: Outer scalar `outer` retains value `50`. Inner `String` is deallocated at inner block closing brace.
