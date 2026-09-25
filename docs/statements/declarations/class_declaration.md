# §4 ClassDeclaration

## 1. Overview & Scope

A `ClassDeclaration` defines a user-defined reference type in Solix. Classes serve as the foundational construct for object-oriented programming, encapsulating state (fields), initialization routines (constructors), and behaviors (methods and operators). Solix supports single class inheritance (`extends`), multiple interface implementation (`implements`), abstract classes, and nested class declarations.

Instances of classes are allocated on the heap as reference-counted objects governed by Solix's Automatic Reference Counting (ARC) runtime. Classes participate in dynamic polymorphism via Virtual Method Tables (VTables) indexed by unique slot IDs.

### Syntactic Placement
A `ClassDeclaration` is permitted at global package scope or nested directly inside another class declaration. It is prohibited inside method bodies or local blocks.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
ClassDeclaration    ::= ClassModifier* 'class' Identifier TypeParams? SuperClause? InterfaceClause? ClassBody
ClassModifier       ::= 'abstract' | 'final' | 'public' | 'private'
SuperClause         ::= 'extends' QualifiedType
InterfaceClause     ::= 'implements' QualifiedType (',' QualifiedType)*
ClassBody           ::= '{' ClassMember* '}'
ClassMember         ::= FieldDeclaration
                      | ConstructorDeclaration
                      | MethodDeclaration
                      | OperatorDeclaration
                      | ClassDeclaration
                      | EnumDeclaration
```

### Canonical Code Patterns
```solix
// 1. Concrete Class with Single Inheritance and Interface Implementation
class Dog extends Animal implements Printable, Serializable {
    String breed;

    Dog(String name, String breed) : super(name) {
        this.breed = breed;
    }

    void print() {
        Console.println("Dog: " + this.breed);
    }
}

// 2. Abstract Class Contract
abstract class Shape {
    abstract float64 area();
}
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Class Symbol Registration
- Registered in the global symbol table during Pass 1a (`REGISTER_GLOBALS`).
- Stamped with the containing `package_context`.

### 3.2 Member Visibility & Access Control
- `public`: Accessible from any package.
- `protected`: Accessible within the declaring class, its subclasses, and within the same package.
- `private`: Accessible only within the lexical body of the declaring class.

### 3.3 Inheritance Hierarchy Invariants
- **Single Class Inheritance**: A class may extend at most one superclass.
- **Cycle-Free Inheritance**: Circular inheritance graphs (e.g. `class A extends B; class B extends A;`) are strictly detected and rejected.
- **Multiple Interface Conformance**: A class can implement any number of interfaces.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Compile-Time Layout Calculation
1. **Memory Sizing**: The compiler computes the instance byte size: superclass field size + sum of declared instance fields.
2. **VTable ID Assignment**: The compiler assigns a unique 32-bit `vtable_id`. Virtual method slots are assigned sequentially, inheriting superclass slots and overriding matching signatures.
3. **Default Constructor Synthesis**: If no constructor is declared, the compiler synthesizes a zero-argument default constructor that invokes `super()`.

### 4.2 Dynamic Instantiation
When `new MyClass(...)` executes:
1. Heap memory is allocated (`OpCode::ALLOC <size>`).
2. The object header is initialized (`ref_count = 1`, `vtable_id`).
3. The constructor is invoked with the instance pointer at stack slot 0 (`this`).

---

## 5. Memory Model & ARC Invariants

### 5.1 Heap Object Layout
```text
+-------------------------------------------------------------+
| Header: uint32_t ref_count (ARC) | uint32_t vtable_id       |
+-------------------------------------------------------------+
| Field 0 Offset (Superclass fields...)                       |
+-------------------------------------------------------------+
| Field 1 Offset (Declared fields: primitive or reference)    |
+-------------------------------------------------------------+
```

### 5.2 Destructor Teardown Invariant
When `ref_count` transitions to 0:
- The VM invokes the class destructor.
- All reference-type fields undergo `DEC_REF` in reverse declaration order.
- The heap block is released via `RELEASE`.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Circular Inheritance Rejection
```solix
class Alpha extends Beta {}
class Beta extends Alpha {} // Error: circular inheritance
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Circular inheritance detected for class 'Alpha'
```

### Rule 6.2: Concrete Class Missing Abstract Method Implementation
```solix
abstract class Shape { abstract float64 area(); }
class Square extends Shape {} // Error: missing area()
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Class 'Square' must implement abstract method 'area()' from 'Shape'
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Instantiation of Abstract Class
Attempting to instantiate an abstract class directly is prevented at compile time; if bypassed via raw bytecode, the VM raises an `AbstractClassInstantiationException`.

---

## 8. Conformance & Verification Examples

### Example 8.1: Dynamic Polymorphism via VTable
```solix
class Base {
    String get_name() { return new String("Base"); }
}
class Derived extends Base {
    String get_name() { return new String("Derived"); }
}

void verify_polymorphism() {
    Base obj = new Derived();
    Console.println(obj.get_name()); // Dispatches to Derived.get_name
}
```
*Verification Invariant*: Output is `"Derived"`. Method dispatch queries VTable slot dynamically.
