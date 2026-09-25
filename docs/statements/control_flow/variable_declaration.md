# VariableDeclarationStatement (`NodeType::VAR_DECL`)

## 1. Description, Purpose & Architectural Implementation

### Conceptual Overview
The `VariableDeclarationStatement` introduces one or more named variables into the active lexical scope, permanently associating each identifier with a concrete static type, a memory storage location, and an optional initial value.

In Solix's statically typed architecture, a variable declaration is not merely a label on a memory address. It establishes the foundational contract between the type checker, the Automatic Reference Counting (ARC) memory engine, and the runtime virtual machine. It specifies whether the variable contains an immediate scalar value (such as an integer or floating-point number) or an owning pointer to a heap-allocated reference object (such as a class instance, string, or array).

---

### Key Conceptual Roles

#### 1. Static Type Binding & Contract Enforcement
Solix enforces strict compile-time type safety. Every declared variable is bound to a `TypeInfo` structure that defines its base type, package qualification, generic type arguments, and array dimensions (`array_depth`).
- When a variable is initialized, the binder strictly verifies that the evaluated type of the initializer expression is assignable to the declared type (`is_assignable`).
- No implicit unsafe conversions occur. Attempting to assign incompatible types halts compilation with an explicit type mismatch diagnostic.

#### 2. Storage Allocation & Activation Frame Slotting
Local variables in Solix do not live in arbitrary memory locations. They occupy fixed, zero-indexed register slots within the current function's activation frame:
- The compiler assigns each local variable a unique `memory_index` in monotonically increasing order (`local_variable_index++`).
- This index maps directly to the VM's operand and local storage vectors, allowing high-performance indexed reads (`GET_LOCAL <index>`) and writes (`SET_LOCAL <index>`).
- Global variables, by contrast, are allocated slots in the global static segment and accessed via `GET_GLOBAL` and `SET_GLOBAL`.

#### 3. ARC Ownership Tracking (Reference vs. Primitive Distinction)
A central responsibility of the variable declaration system is distinguishing between value types and reference types:
- **Value Types**: Primitive scalar types (`int8`, `int16`, `int32`, `int64`, `uint8`, `uint16`, `uint32`, `uint64`, `float32`, `float64`, `bool`, `char`) with zero array depth (`array_depth == 0`). These types hold their payload directly in the local stack slot and require no reference counting.
- **Reference Types**: Class instances, interfaces, strings, and **all arrays** (including arrays of primitives, such as `int32[]`).
- For reference types, the variable declaration marks `is_reference_type = true`. When initialized, an `OpCode::INC_REF` instruction is emitted to claim ownership of the object. When the enclosing block terminates, the variable's reference is decremented (`DEC_REF`), ensuring prompt, leak-free destruction.

#### 4. Lexical Registration & Collision Prevention
When a variable is declared, it is registered in the immediate `SymbolTable` scope:
- If an identifier with the identical name already exists in the *current immediate* scope, the compiler reports a duplicate declaration error.
- If the identifier exists in an *outer parent* scope, declaration succeeds, creating a valid lexical shadow that persists until the enclosing block terminates.

---

### How VariableDeclaration Was Implemented in Solix

#### 1. Abstract Syntax Tree Representation (`statements.hpp`)
In the Solix AST, variable declarations are represented by `VariableDeclaration`:
```cpp
struct VariableDeclaration : public Node {
    TypeInfo type_info;
    std::string var_name;
    std::unique_ptr<Node> initializer;
    int32_t memory_index = -1;
    bool is_reference_type = false;
    bool is_const = false;

    VariableDeclaration(const Token& t, TypeInfo type, const std::string& name,
                        std::unique_ptr<Node> init = nullptr)
        : Node(NodeType::VAR_DECL, t), type_info(std::move(type)),
          var_name(name), initializer(std::move(init)) {
        if (initializer) initializer->parent = this;
    }
};
```

#### 2. Semantic Analysis & Type Resolution (`binder.cpp`)
During semantic analysis (`BinderPass::BIND_EXECUTION`):
1. **Type Resolution**: The binder resolves the declared type using `resolve_type(n.type_info, &n)`, transforming relative type names into fully qualified symbol references (e.g. `String` -> `std.string.String`).
2. **Reference Type Classification**:
   ```cpp
   Node *type_decl = global_scope.resolve(n.type_info.name);
   n.is_reference_type = !(type_decl && type_decl->is_primitive && n.type_info.array_depth == 0);
   ```
   Notice that an array of primitives (`int32[]`) has `array_depth == 1`, so `is_reference_type` evaluates to `true`.
3. **Initializer Verification**:
   If an initializer expression exists:
   - `evaluate_expression(n.initializer.get())` derives the initializer's concrete `TypeInfo`.
   - `is_assignable(n.type_info, initializer_type)` checks compatibility, supporting class inheritance upcasting, interface conformance, and numeric widening.
   - If incompatible, an error is recorded: `"Type mismatch in variable declaration: expected 'X', got 'Y'"`.
4. **Frame Allocation & Registration**:
   - Assigns `n.memory_index = local_variable_index++`.
   - Registers the variable in the active scope via `declare_local(n.var_name, &n)`.

#### 3. Bytecode Emission (`assembler.cpp`)
When the assembler visits a `VariableDeclaration`:
```cpp
void Assembler::visit(VariableDeclaration &node) {
    if (node.initializer) {
        compile_expression(node.initializer.get());
        if (node.is_reference_type) {
            emit_byte(static_cast<uint8_t>(OpCode::INC_REF));
        }
        emit_byte(static_cast<uint8_t>(OpCode::SET_LOCAL));
        emit_int32(node.memory_index);
    }
}
```
If the variable is uninitialized, no bytecode is emitted at declaration time. The slot in the VM's activation frame defaults to zero (or null for reference types).

---

## 2. Syntax & Grammar

### Syntax Forms
Solix supports both initialized and uninitialized variable declarations across primitive, reference, and array types:

```solix
// 1. Primitive Scalar Declarations
int32 counter = 0;
float64 ratio; // Uninitialized, defaults to 0.0
bool isActive = true;
char symbol = 'A';

// 2. Reference Type Declarations
String message = new String("Solix");
MyClass instance = new MyClass();
MyInterface handler = new ConcreteHandler(); // Polymorphic interface binding

// 3. Array Declarations
int32[] numbers = new int32[10];
String[] names = ["Alice", "Bob", "Charlie"];
float64[][] matrix = new float64[4][4];

// 4. Parameterized Generic Declarations
List<int32> list = new List<int32>();
Map<String, User> userCache = new Map<String, User>();
```

---

## 3. Underlying Systems & VM Mechanics

### Memory Layout & Activation Frame Slotting
When a function is called, the VM allocates an activation frame containing a fixed vector of registers sized according to `local_variable_index`:

```text
Function Frame Layout:
+------------+-------------------------------------------+
| Slot Index | Variable Name & Type                      |
+------------+-------------------------------------------+
| 0          | this (Implicit pointer for instance methods)|
| 1          | param1 (First function parameter)         |
| 2          | param2 (Second function parameter)        |
| 3          | local_var1 (First local: int32 counter)   |
| 4          | local_var2 (Second local: String text)    |
| ...        | ...                                       |
+------------+-------------------------------------------+
```

### ARC Interaction During Lifecycle
```text
Declaration with Initializer:
  1. Evaluate Initializer Expression -> Pushes Object Ref to Stack
  2. OpCode::INC_REF                 -> Increments Object ref_count (e.g. 1 -> 2)
  3. OpCode::SET_LOCAL <slot>        -> Pops Object Ref from Stack into Frame Register

Enclosing Block Exit:
  1. OpCode::GET_LOCAL <slot>        -> Pushes Object Ref to Stack
  2. OpCode::DEC_REF                 -> Decrements Object ref_count (e.g. 2 -> 1 or 1 -> 0)
  3. If ref_count == 0               -> VM executes object destructor & frees heap buffer
```

---

## 4. Positive Test Scenarios (Valid Variations)

### Scenario 4.1: Uninitialized Primitive and Reference Variables
Variables declared without an explicit initializer must default to zero or null and compile cleanly.
```solix
void test_uninitialized() {
    int32 uninit_int;
    String uninit_str;
    // Both variables allocate valid frame slots
}
```
*Verification*: Compiles with zero errors. Stack frame reserves 2 local slots.

### Scenario 4.2: Upcasting to Superclass Reference
Declaring a variable of a base class type and initializing it with a derived class instance.
```solix
class Animal {}
class Dog extends Animal {}

void test_upcast() {
    Animal pet = new Dog(); // Valid polymorphic assignment
}
```
*Verification*: `is_assignable` returns `true`. Code emits `INC_REF` and stores in `pet` slot.

### Scenario 4.3: Interface Binding
Declaring a variable of an interface type and initializing it with an implementing class instance.
```solix
interface Printable {
    void print();
}
class Document implements Printable {
    void print() { Console.println("Document"); }
}

void test_interface_binding() {
    Printable p = new Document();
    p.print();
}
```
*Verification*: Compiles successfully. Method invocation dispatches dynamically through interface VTable.

### Scenario 4.4: Array of Primitives as Reference Type
Declaring an array of primitive integers and verifying that it is correctly classified as a reference type for ARC.
```solix
void test_primitive_array() {
    {
        int32[] buffer = new int32[1024];
        buffer[0] = 42;
    }
    // 'buffer' must be cleaned up via DEC_REF upon exiting the block
}
```
*Verification*: Bytecode disassembly confirms `INC_REF` is emitted upon initialization and `DEC_REF` is emitted upon block termination.

### Scenario 4.5: Variable Shadowing Across Nested Scopes
Declaring an inner variable that shadows an outer variable with an entirely different type.
```solix
void test_shadowing() {
    int32 val = 100;
    {
        String val = new String("shadow");
        Console.println(val); // Resolves to String
    }
    Console.println(val); // Resolves to int32 (100)
}
```
*Verification*: Outputs `"shadow"` followed by `100`. Both variables occupy separate frame indices.

---

## 5. Negative Test Scenarios (Invalid Variations)

### Scenario 5.1: Incompatible Type Assignment
Attempting to initialize a variable with an expression of an incompatible type.
```solix
void test_type_mismatch() {
    int32 count = "one hundred"; // Error: String cannot be assigned to int32
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Type mismatch in variable declaration: expected 'int32', got 'String'
```

### Scenario 5.2: Duplicate Variable Declaration in Same Scope
Declaring two variables with the exact same identifier in the same immediate block.
```solix
void test_duplicate_var() {
    int32 score = 10;
    int32 score = 20; // Error: duplicate
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Variable 'score' is already defined in the current scope
```

### Scenario 5.3: Unknown / Unresolved Type Identifier
Declaring a variable with a type name that does not exist in any imported package or local scope.
```solix
void test_unknown_type() {
    NonExistentType obj = null;
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Unknown type 'NonExistentType'
```

### Scenario 5.4: Assigning Superclass Instance to Subclass Variable (Invalid Downcast)
Attempting to assign a base class instance to a derived class variable without an explicit cast.
```solix
class Base {}
class Derived extends Base {}

void test_invalid_implicit_downcast() {
    Derived d = new Base(); // Error: Base is not assignable to Derived
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Type mismatch in variable declaration: expected 'Derived', got 'Base'
```

### Scenario 5.5: Void Variable Declaration
Attempting to declare a variable of type `void`.
```solix
void test_void_variable() {
    void unusable;
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Variable cannot be of type 'void'
```
