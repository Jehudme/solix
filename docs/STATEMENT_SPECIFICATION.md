# Solix Master Statement & Construct Specification

This document provides the definitive, exhaustive specification of the Solix programming language, structured strictly by **Statement and Construct Type**. Every construct maps directly to the Solix Abstract Syntax Tree (`NodeType`), compilation pipeline processes (Lexer, Parser, Binder, Assembler), and Virtual Machine runtime execution semantics.

For each statement and construct, this specification details:
1. **Formal Grammar & Syntax**
2. **Underlying Compiler & Runtime Mechanism** (Memory layout, ARC lifecycle, VTable dispatch, stack frames, bytecode generation)
3. **Comprehensive Valid Variations (All conditions that work)**
4. **Comprehensive Invalid Variations (All conditions that fail, with the exact compiler diagnostic error message or runtime VM exception)**

---

# Table of Contents
- [Part I: Compilation Unit & Module Statements](#part-i-compilation-unit--module-statements)
  - [1. PackageStatement (`PACKAGE_STMT`)](#1-packagestatement-nodetypepackage_stmt)
  - [2. ImportStatement (`IMPORT_STMT`)](#2-importstatement-nodetypeimport_stmt)
  - [3. AliasStatement (`ALIAS_STMT`)](#3-aliasstatement-nodetypealias_stmt)
- [Part II: Type & Structural Declarations](#part-ii-type--structural-declarations)
  - [4. ClassDeclaration (`CLASS_DECL`)](#4-classdeclaration-nodetypeclass_decl)
  - [5. InterfaceDeclaration (`INTERFACE`)](#5-interfacedeclaration-nodetypeinterface)
  - [6. EnumDeclaration (`ENUM_DECL`)](#6-enumdeclaration-nodetypeenum_decl)
- [Part III: Class Member Declarations](#part-iii-class-member-declarations)
  - [7. FieldDeclaration (`FIELD_DECL`)](#7-fielddeclaration-nodetypefield_decl)
  - [8. ConstructorDeclaration (`CONSTRUCTOR_DECL`)](#8-constructordeclaration-nodetypeconstructor_decl)
  - [9. MethodDeclaration (`METHOD_DECL`)](#9-methoddeclaration-nodetypemethod_decl)
  - [10. OperatorDeclaration (`OPERATOR`)](#10-operatordeclaration-nodetypeoperator)
- [Part IV: Local Scope & Execution Statements](#part-iv-local-scope--execution-statements)
  - [11. BlockStatement (`BLOCK`)](#11-blockstatement-nodetypeblock)
  - [12. VariableDeclarationStatement (`VAR_DECL`)](#12-variabledeclarationstatement-nodetypevar_decl)
  - [13. ExpressionStatement (`EXPR_STMT`)](#13-expressionstatement-nodetypeexpr_stmt)
  - [14. IfStatement (`IF_STMT`)](#14-ifstatement-nodetypeif_stmt)
  - [15. WhileStatement (`WHILE_STMT`)](#15-whilestatement-nodetypewhile_stmt)
  - [16. DoWhileStatement (`DO_WHILE_STMT`)](#16-dowhilestatement-nodetypedo_while_stmt)
  - [17. ForStatement (`FOR_STMT`)](#17-forstatement-nodetypefor_stmt)
  - [18. SwitchStatement (`SWITCH_STMT`) & CaseStatement (`CASE_STMT`)](#18-switchstatement-nodetypeswitch_stmt--casestatement-nodetypecase_stmt)
  - [19. BreakStatement (`BREAK_STMT`)](#19-breakstatement-nodetypebreak_stmt)
  - [20. ContinueStatement (`CONTINUE_STMT`)](#20-continuestatement-nodetypecontinue_stmt)
  - [21. ReturnStatement (`RETURN_STMT`)](#21-returnstatement-nodetypereturn_stmt)
  - [22. ThrowStatement (`THROW_STMT`)](#22-throwstatement-nodetypethrow_stmt)
  - [23. TryCatchFinallyStatement (`TRY_STMT`, `CATCH_CLAUSE`)](#23-trycatchfinallystatement-nodetypetry_stmt-nodetypecatch_clause)
- [Part V: Expressions & Operators](#part-v-expressions--operators)
  - [24. AssignmentExpression (`ASSIGNMENT_EXPR`)](#24-assignmentexpression-nodetypeassignment_expr)
  - [25. TernaryExpression (`TERNARY_EXPR`)](#25-ternaryexpression-nodetypeternary_expr)
  - [26. BinaryExpression (`BINARY_EXPR`)](#26-binaryexpression-nodetypebinary_expr)
  - [27. UnaryExpression (`UNARY_EXPR`)](#27-unaryexpression-nodetypeunary_expr)
  - [28. CastExpression (`CAST_EXPR`)](#28-castexpression-nodetypecast_expr)
  - [29. InstanceOfExpression (`INSTANCEOF_EXPR`)](#29-instanceofexpression-nodetypeinstanceof_expr)
  - [30. NewInstanceExpression (`NEW_INSTANCE`)](#30-newinstanceexpression-nodetypenew_instance)
  - [31. ArrayCreationExpression (`ARRAY_CREATION`)](#31-arraycreationexpression-nodetypearray_creation)
  - [32. ArrayAccessExpression (`ARRAY_ACCESS`)](#32-arrayaccessexpression-nodetypearray_access)
  - [33. ArrayLiteralExpression (`ARRAY_LITERAL`)](#33-arrayliteralexpression-nodetypearray_literal)
  - [34. MemberAccessExpression (`MEMBER_ACCESS`)](#34-memberaccessexpression-nodetypemember_access)
  - [35. MethodCallExpression (`METHOD_CALL`)](#35-methodcallexpression-nodetypemethod_call)
  - [36. IdentifierNode (`IDENTIFIER`)](#36-identifiernode-nodetypeidentifier)
  - [37. LiteralNode (`LITERAL`)](#37-literalnode-nodetypeliteral)

---

# Part I: Compilation Unit & Module Statements

## 1. `PackageStatement` (`NodeType::PACKAGE_STMT`)

### Syntax
```solix
package <identifier> ('.' <identifier>)* ';'
```

### Underlying Compiler & Runtime Mechanism
- Evaluated during Pass 1a (`REGISTER_GLOBALS`) and Pass 1b (`REGISTER_MEMBERS`).
- Stamped onto AST child nodes via `node->package_context` to guarantee namespace context preservation across flat multi-file passes.
- Prefixes all top-level symbols (classes, enums, global aliases) with `package_name + "."`.
- Registers the package into the compiler's `known_packages` table for cross-file resolution.
- Emits zero runtime bytecode; purely compile-time namespace partitioning.

### Valid Variations
1. **Single-Level Package**: `package solix;`
2. **Deep Hierarchical Package**: `package com.solix.advanced.render.vulkan;`
3. **Implicit Root Package**: Omitting `package` places all declarations in the global root namespace.
4. **Multiple Files Sharing One Package**: Multiple distinct `.slx` source files declaring the identical package name merge symbols into the same package namespace.

### Invalid Variations & Expected Errors
1. **Multiple Package Declarations in One File**:
   - `package a; package b;`  
     *Error*: `Multiple package declarations in single compilation unit`
2. **Package Declaration After Top-Level Code**:
   - `class Item {} package my_pkg;`  
     *Error*: `Package statement must be the first statement in the file`
3. **Invalid Characters or Numerics in Package Path**:
   - `package 123.foo;`  
     *Error*: `Expected identifier in package declaration`
4. **Missing Semicolon**:
   - `package com.foo`  
     *Error*: `Expected ';' after package name`

---

## 2. `ImportStatement` (`NodeType::IMPORT_STMT`)

### Syntax
```solix
import <package-path> ('.' '*' | ('.' | '::') <symbol>) ';'
```

### Underlying Compiler & Runtime Mechanism
- Collected in Pass 1a into `pending_imports` and bound after all packages and symbols are cataloged (`process_imports`).
- Individual symbol imports register `imported_symbols[symbol_name] = full_mangled_name`.
- Wildcard imports register `known_packages.insert(target_pkg)`.
- Symbol lookups check `imported_symbols` before falling back to cross-package suffix searches, enabling explicit conflict resolution.

### Valid Variations
1. **Full Symbol Import**: `import solix.core.String;`
2. **Sub-Namespace Symbol Import**: `import core.String;`
3. **Wildcard Package Import**: `import solix.collections.*;` or `import collections.*;`
4. **C++ Scope Resolution Import**: `import solix::core::Objects;`
5. **Multiple Imports in Single File**: Importing dozens of modules sequentially.

### Invalid Variations & Expected Errors
1. **Importing Non-Existent Symbol**:
   - `import solix.core.NonExistentClass;`  
     *Error*: `Cannot resolve imported symbol: solix.core.NonExistentClass`
2. **Importing Non-Existent Wildcard Package**:
   - `import fake.unknown.pkg.*;`  
     *Error*: `Cannot resolve imported package: fake.unknown.pkg`
3. **Wildcard Not at Tail**:
   - `import solix.*.String;`  
     *Error*: `Expected identifier or '*' after '.'`
4. **Wildcard Ambiguity Conflict**:
   - `import pkg_a.*; import pkg_b.*;` where both define `Widget`, accessed as unqualified `Widget w = ...;`  
     *Error*: `Ambiguous symbol 'Widget': multiple candidates found (pkg_a.Widget, pkg_b.Widget). Specify full package or use import to disambiguate.`

---

## 3. `AliasStatement` (`NodeType::ALIAS_STMT`)

### Syntax
```solix
alias <identifier> ['<' <T...> '>'] '=' <type-info> ';'
```

### Underlying Compiler & Runtime Mechanism
- Registered in Pass 1a in `global_scope.symbols` as an `AliasStatement` node.
- Supports generic parameterization: if template parameters `<T...>` are present, stored as a parameterized alias blueprint.
- In Pass 2, `target_type` is resolved and `resolved_declaration` is linked directly to the underlying `ClassDeclaration`, `EnumDeclaration`, or template blueprint.
- Re-export alias deduplication: unifies multiple aliases referencing the same underlying type so that re-exports (such as `solix.NullPointerException` aliasing `solix.core.NullPointerException`) never cause false-positive collision errors.
- Monomorphizes lazily upon instantiation if the alias targets a generic template.

### Valid Variations
1. **Primitive Array Shorthand**: `alias Matrix = float64[][];`
2. **Full Symbol Namespace Alias**: `alias Str = solix.core.String;`
3. **Partial Symbol Namespace Alias**: `alias Str = core.String;`
4. **Template Blueprint Alias**: `alias List = solix.collections.List;`
5. **Templated Generic Alias**: `alias IntMap<V> = Map<int32, V>;`
6. **Re-Export Backward Compatibility Alias**:
   - In `package solix; alias String = solix.core.String;`

### Invalid Variations & Expected Errors
1. **Aliasing Undefined Type**:
   - `alias Bad = nonexistent.Type;`  
     *Error*: `Unknown type: nonexistent.Type`
2. **Duplicate Alias Name in Scope**:
   - `alias Item = int32; alias Item = float64;`  
     *Error*: `Duplicate global symbol: Item`
3. **Cyclic Alias Reference**:
   - `alias A = B; alias B = A;`  
     *Error*: `Cyclic type alias definition detected: A -> B -> A`
4. **Missing Semicolon**:
   - `alias ID = uint64`  
     *Error*: `Expected ';' after alias declaration`

---

# Part II: Type & Structural Declarations

## 4. `ClassDeclaration` (`NodeType::CLASS_DECL`)

### Syntax
```solix
[access-modifier] ['abstract'] class <identifier> ['<' <T...> '>'] ['extends' | ':' <base-class>] ['implements' <ifaces...>] '{' <members...> '}'
```

### Underlying Compiler & Runtime Mechanism
- **Memory Layout & Instance Size**:
  - In Pass 2, instance layout calculates cumulative byte/word offsets. Base class fields are placed first (offsets `1 .. N-1`), followed by derived fields.
  - Word 0 contains the internal ARC header and `vtable_id`.
  - `instance_size` is calculated and hardcoded into `ALLOC` opcodes.
- **Dynamic VTable Generation**:
  - Virtual methods are assigned sequential slot indices.
  - Subclasses clone the base vtable and overwrite overridden slots.
  - Classes assigned a unique integer `vtable_id`.
- **Default Constructor Synthesis**:
  - If no constructor is written, the compiler automatically synthesizes `public ClassName() {}`.
- **Template Monomorphization**:
  - Template classes (`class Box<T>`) are stored as blueprints in `template_registry`. Upon instantiation, the AST is duplicated, substituted, and bound with a mangled name (`Box$int32`).

### Valid Variations
1. **Minimal Concrete Class**: `class Simple {}`
2. **Single Inheritance**: `class Dog extends Animal {}` or `class Dog : Animal {}`
3. **Generic Class with Multiple Parameters**: `class Map<K, V> { K key; V val; }`
4. **Abstract Base Class**: `public abstract class Shape { public abstract float64 area(); }`
5. **Nested Field Declarations with Default Initializers**:
   ```solix
   class Player {
       int32 hp = 100;
       String name = new String("Player1");
   ```
6. **Nested Classes & Enums**: Declaring inner classes or enums inside the class body:
   ```solix
   public class Outer {
       public class Inner {
           int32 inner_id;
       }
       public enum InnerState { A, B }
   }
   ```

### Invalid Variations & Expected Errors
1. **Duplicate Class in Package**:
   - `class Player {} class Player {}`  
     *Error*: `Duplicate global symbol: Player`
2. **Extending Non-Existent Base Class**:
   - `class Dog extends GhostAnimal {}`  
     *Error*: `Base class not found: GhostAnimal`
3. **Extending Primitive or Array Type**:
   - `class IntWrapper extends int32 {}`  
     *Error*: `Base class cannot be a primitive or array type`
4. **Circular Inheritance**:
   - `class A extends B {} class B extends A {}`  
     *Error*: `Cyclic inheritance detected for class 'A'`
5. **Multiple Inheritance**:
   - `class Dog extends Mammal, Canine {}`  
     *Error*: Syntax error: unexpected token `,` after base class
6. **Direct Instantiation of Abstract Class**:
   - `Shape s = new Shape();`  
     *Error*: `Cannot instantiate abstract class 'Shape'`

---

## 5. `InterfaceDeclaration` (`NodeType::INTERFACE`)

### Syntax
```solix
[access-modifier] interface <identifier> '{' <method-signature...>';' '}'
```

### Underlying Compiler & Runtime Mechanism
- Declares a pure protocol of method signatures without bodies or state.
- Checked during Pass 2: any concrete class claiming `implements Iface` must provide concrete implementations matching every method signature.

### Valid Variations
1. **Single Method Interface**: `interface Runnable { void run(); }`
2. **Multi-Method Interface**: `interface Collection { int32 size(); void clear(); }`
3. **Multiple Conformance**: `class Texture implements Renderable, Disposable { ... }`

### Invalid Variations & Expected Errors
1. **Declaring Fields in Interface**:
   - `interface Bad { int32 x; }`  
     *Error*: `Interfaces cannot declare fields`
2. **Method with Body in Interface**:
   - `interface Bad { void run() {} }`  
     *Error*: `Interface methods cannot have bodies`
3. **Direct Instantiation**:
   - `Runnable r = new Runnable();`  
     *Error*: `Cannot instantiate interface 'Runnable'`
4. **Class Failing to Implement Interface Method**:
   - `class App implements Runnable {}` (without `run()`)  
     *Error*: `Class 'App' does not implement required interface method 'void Runnable.run()'`

---

## 6. `EnumDeclaration` (`NodeType::ENUM_DECL`)

### Syntax
```solix
[access-modifier] enum <identifier> '{' <identifier> (',' <identifier>)* [','] '}'
```

### Underlying Compiler & Runtime Mechanism
- Registered in `global_scope.symbols` as an `EnumDeclaration`.
- Members assigned integer ordinal indices (0, 1, 2, ...).
- Emitted in bytecode as integer constants.
- Supports switch dispatch and equality comparisons.

### Valid Variations
1. **Standard Enum**: `enum State { IDLE, RUNNING, PAUSED, STOPPED }`
2. **Trailing Comma Enum**: `enum Level { LOW, HIGH, }`
3. **Qualified Member Access**: `State s = State.RUNNING;`
4. **Enum In Switch**: `switch (s) { case State.IDLE: ... }`

### Invalid Variations & Expected Errors
1. **Duplicate Enum Constants**:
   - `enum Mode { ON, OFF, ON }`  
     *Error*: `Duplicate enum constant 'ON'`
2. **Accessing Non-Existent Member**:
   - `State s = State.UNKNOWN;`  
     *Error*: `Undefined enum member: UNKNOWN`
3. **Assigning Integer to Enum Without Cast**:
   - `State s = 0;`  
     *Error*: `Type mismatch in variable declaration: expected 'State', got 'int32'`

---

# Part III: Class Member Declarations

## 7. `FieldDeclaration` (`NodeType::FIELD_DECL`)

### Syntax
```solix
[access-modifier] ['static'] ['const'] ['weak'] <type-info> <name> ['=' <initializer-expr>] ';'
```

### Underlying Compiler & Runtime Mechanism
- **Instance Fields**: Assigned memory index offsets in instance layout (`memory_index = 1 .. N`). Field initializers are compiled into bytecode and injected into constructor preambles before user code runs. Emits `SET_PROPERTY` (or `WEAK_SET_PROPERTY` for weak fields).
- **Static Fields**: Assigned global memory index in `static_variable_index`. Evaluated in global data section; accessed via `GET_GLOBAL` and `SET_GLOBAL`.
- **Top-Level Global Variables**: Can be declared directly outside classes in compilation units; stored in global memory index.
- **ARC Lifecycle**: Reference-typed fields trigger `INC_REF` upon assignment. When the parent object is destroyed (`RELEASE`), the VM iterates reference field offsets and triggers `DEC_REF`.
- **Weak Fields (`weak`)**: Non-owning pointer flag set (`is_weak = true`). Emits `WEAK_SET_PROPERTY`, which skips `INC_REF` to prevent cyclic memory leaks.

### Valid Variations
1. **Instance Field Uninitialized**: `public int32 health;`
2. **Instance Field with Initializer**: `private float64 scale = 1.0;`
3. **Static Class Field**: `public static int32 count = 0;`
4. **Top-Level Global Variable**: `int32 global_counter = 42;`
5. **Const Immutable Field**: `public const int32 BUFFER_SIZE = 1024;`
6. **Weak Reference Pointer**: `protected weak TreeNode parent_node;`

### Invalid Variations & Expected Errors
1. **Initializer Type Mismatch**:
   - `int32 score = "top";`  
     *Error*: `Type mismatch in field initialization: expected 'int32', got 'char[]'`
2. **Weak Modifier on Primitive**:
   - `weak int32 count;`  
     *Error*: `'weak' modifier only valid on reference types`
3. **Duplicate Field Name in Class**:
   - `int32 x; float64 x;`  
     *Error*: `Duplicate member 'x' in class`
4. **Accessing Uninstantiated `this` in Field Initializer**:
   - `int32 y = this.compute();`  
     *Error*: `Cannot access 'this' in field initializer`

---

## 8. `ConstructorDeclaration` (`NodeType::CONSTRUCTOR_DECL`)

### Syntax
```solix
[access-modifier] <class-name> '(' <parameters...> ')' [':' <init-item> (',' <init-item>)*] <block>
// where <init-item> is either 'super' '(' <args...> ')' or <field-name> '(' <expr> ')'
```

### Underlying Compiler & Runtime Mechanism
- Sets up activation frame with `this` at register 0.
- If `: super(...)` is present, compiles arguments and emits `INVOKE_DIRECT` to the base constructor.
- Evaluates member initializer items (`: field_name(expr)`), compiling them as direct assignments (`this.field_name = expr;`) in the constructor preamble.
- Injects field default initializers into the bytecode stream directly following the super call and member initializer list.
- Executes user constructor body.
- Returns `this` reference.

### Valid Variations
1. **Parameterless Constructor**: `public Player() { this.hp = 100; }`
2. **Parameterized Constructor**: `public Player(int32 hp) { this.hp = hp; }`
3. **Explicit Base Constructor Chaining**:
   ```solix
   public Dog(char[] name, int32 age) : super(name) {
       this.age = age;
   }
   ```
4. **C++ Style Member Initializer List**:
   ```solix
   public Vector2(float64 x, float64 y) : x(x), y(y) {}
   ```
5. **Combined Base Constructor and Member Initializers**:
   ```solix
   public Dog(char[] name, int32 age) : super(name), age(age) {}
   ```
6. **Multiple Overloaded Constructors**: Overloading constructors by parameter arity and types.

### Invalid Variations & Expected Errors
1. **Constructor Name Mismatch**:
   - In class `Player`: `public User() {}`  
     *Error*: Treated as method with missing return type
2. **Returning a Value**:
   - `public Player() { return 10; }`  
     *Error*: `Constructors cannot return a value`
3. **Super Constructor Argument Mismatch**:
   - `public Dog() : super(10, 20, 30) {}` where `Animal` has only `Animal(char[])`  
     *Error*: `No matching constructor: Animal.ctor(int32,int32,int32)`
4. **Placing `super()` Inside Body Statements**:
   - `public Dog() { int32 x = 0; super(); }`  
     *Error*: Parse error: unexpected token `super`

---

## 9. `MethodDeclaration` (`NodeType::METHOD_DECL`)

### Syntax
```solix
[access-modifier] ['static' | 'virtual' | 'override' | 'abstract' | 'inline' | 'native'] <return-type> <name> ['<' <T...> '>'] '(' <params...> ')' (<block> | ';')
```

### Underlying Compiler & Runtime Mechanism
- **Instance Calling Convention**: Parameter 0 is implicit `this`.
- **Static Methods**: No `this` pointer; resolved at compile time via `INVOKE_STATIC`.
- **Virtual Dispatch**: Emits `INVOKE_VIRTUAL` referencing slot index in `vtable_id`.
- **Native Host Interop (`native`)**: Bound to C++ runtime function pointer in `RuntimeOptions::native_functions`.
- **Generic Monomorphization**: Cloned and instantiated per concrete type combination, cached in monomorphization table.

### Valid Variations
1. **Standard Instance Method**: `public void set_hp(int32 hp) { this.hp = hp; }`
2. **Static Utility Method**: `public static int32 max(int32 a, int32 b) { return a > b ? a : b; }`
3. **Top-Level Free Functions (Outside Class)**:
   ```solix
   public static int32 main(char[][] args) {
       return 0;
   }
   ```
4. **Virtual and Override Polymorphism**:
   - Base: `public virtual void render();`
   - Derived: `public override void render() { ... }`
5. **Abstract Method Declaration**: `public abstract void serialize();` (in abstract class; vtable slot populated with `THROW_ABSTRACT` guard)
6. **Generic Template Method**: `public static void swap<T>(T[] arr, int32 i, int32 j) { ... }`
7. **Native Method Declaration**: `public native static void print(char[] str);`

### Invalid Variations & Expected Errors
1. **Abstract Method with Body**:
   - `public abstract void f() { return; }`  
     *Error*: `Abstract methods cannot have a body`
2. **Non-Abstract Non-Native Method without Body**:
   - `public void f();`  
     *Error*: `Non-abstract method must have a body`
3. **Static Virtual Method**:
   - `public static virtual void f() {}`  
     *Error*: `Static methods cannot be virtual or abstract`
4. **Override Without Base Virtual Match**:
   - `public override void fake_method() {}`  
     *Error*: `Method 'fake_method' marked override but does not override any base method`
5. **Override Signature Mismatch**:
   - Base: `virtual void run(int32 speed);` Subclass: `override void run(float64 speed);`  
     *Error*: `Overriding method signature does not match base virtual method`
6. **Native Method with Body**:
   - `public native void print(char[] s) {}`  
     *Error*: `Native methods cannot have a body`
7. **Calling Abstract Method Directly at Runtime**:
   - Invoking abstract method without subclass override  
     *Runtime VM Check*: `THROW_ABSTRACT`

---

## 10. `OperatorDeclaration` (`NodeType::OPERATOR`)

### Syntax
```solix
[access-modifier] <return-type> 'operator' <operator-symbol> '(' <parameter> ')' <block>
// where <operator-symbol> is one of: '+', '-', '*', '/', '='
```

### Underlying Compiler & Runtime Mechanism
- Compiles as a specialized member method with mangled name `operator<op>(param_type)`.
- Specifically supported operators: `+`, `-`, `*`, `/`, and `=`.
- When the parser encounters binary operators (`a + b`, `a = b`) where `a` is a class type, the binder transforms the binary expression into a method call on `a` passing `b`.
- ARC retains return values from operators properly.

### Valid Variations
1. **Addition Overload**: `public Vector2 operator+(Vector2 other) { return new Vector2(this.x + other.x, this.y + other.y); }`
2. **Subtraction Overload**: `public Vector2 operator-(Vector2 other) { return new Vector2(this.x - other.x, this.y - other.y); }`
3. **Multiplication Overload**: `public Vector2 operator*(float64 scalar) { return new Vector2(this.x * scalar, this.y * scalar); }`
4. **Division Overload**: `public Vector2 operator/(float64 scalar) { return new Vector2(this.x / scalar, this.y / scalar); }`
5. **Assignment Overload**: `public Vector2 operator=(Vector2 other) { this.x = other.x; this.y = other.y; return this; }`

### Invalid Variations & Expected Errors
1. **Overloading Unsupported Operator Symbol**:
   - `public void operator%() {}` or `public void operator==() {}` or `public void operator.() {}`  
     *Error*: Parse error: `Invalid operator for overloading`
2. **Operator Overload Declared Outside Class**:
   - `Vector2 operator+(Vector2 a, Vector2 b) { ... }`  
     *Error*: `Operator overloads must be declared as member methods within a class`
3. **Binary Operator with Wrong Parameter Count**:
   - `public Vector2 operator+() {}` (takes 0 args)  
     *Error*: `Binary operator overload must take exactly one argument`
   - `public Vector2 operator+(Vector2 a, Vector2 b) {}` (takes 2 args)  
     *Error*: `Binary operator overload must take exactly one argument`

---

# Part IV: Local Scope & Execution Statements

## 11. `BlockStatement` (`NodeType::BLOCK`)

### Syntax
```solix
'{' <statement...>* '}'
```

### Underlying Compiler & Runtime Mechanism
- Pushes a new lexical `SymbolTable` scope into the scope stack on entry.
- Allocates local register indices (`local_variable_index`).
- **ARC Automatic Scope Exit Cleanup**:
  - The compiler tracks all reference-typed local variables introduced in this block.
  - Upon natural exit of the block, the compiler emits `DEC_REF` / `RELEASE` instructions for every reference variable declared in that block in reverse declaration order.
- Restores parent lexical scope on exit.

### Valid Variations
1. **Empty Block**: `{}`
2. **Function Body Block**: `{ return 0; }`
3. **Arbitrary Nested Scoping**:
   ```solix
   int32 x = 10;
   {
       int32 y = 20;
       String scoped_str = new String("temp");
       // scoped_str freed by ARC exactly at closing brace
   }
   // y is out of scope here
   ```

### Invalid Variations & Expected Errors
1. **Accessing Local Variable Outside Its Enclosing Block**:
   - `{ int32 inside = 42; } int32 outside = inside;`  
     *Error*: `Undefined identifier: inside`
2. **Unmatched Closing Brace**:
   - `void f() { int32 x = 1; } }`  
     *Error*: Syntax error: unexpected token `}`
3. **Unterminated Block**:
   - `void f() { int32 x = 1;`  
     *Error*: Syntax error: expected `}` before end of file

---

## 12. `VariableDeclarationStatement` (`NodeType::VAR_DECL`)

### Syntax
```solix
['const'] <type-info> <name> ['=' <initializer-expr>] ';'
```

### Underlying Compiler & Runtime Mechanism
- Registers variable in active lexical `current_scope`.
- Assigns stack register index `local_variable_index++`.
- Evaluates initializer expression and emits store opcode (`SET_LOCAL`).
- Enforces reference counting rules if type is class or array (`is_reference_type`).
- Enforces variable shadowing checks: scans parent scopes and emits diagnostic warning if inner variable hides an outer name.

### Valid Variations
1. **Uninitialized Primitive**: `int32 x;` (zero-initialized)
2. **Initialized Primitive**: `float64 f = 3.14159;`
3. **Reference Allocation**: `String s = new String("hello");`
4. **Const Immutable Binding**: `const int32 MAX_USERS = 500;`
5. **Reference Parameter Binding**: `int32& ref_target;`

### Invalid Variations & Expected Errors
1. **Type Mismatch in Initialization**:
   - `int32 x = "text";`  
     *Error*: `Type mismatch in variable declaration: expected 'int32', got 'char[]'`
2. **Uninitialized Const Variable**:
   - `const int32 LIMIT;`  
     *Error*: `Const variable 'LIMIT' must have an initializer`
3. **Duplicate Variable in Same Scope**:
   - `int32 count = 1; int32 count = 2;`  
     *Error*: `Duplicate local variable 'count'`
4. **Variable Shadowing Warning**:
   - Outer: `int32 temp = 1;` Inner: `int32 temp = 2;`  
     *Warning*: `[warning] Variable 'temp' shadows a variable in an outer scope`

---

## 13. `ExpressionStatement` (`NodeType::EXPR_STMT`)

### Syntax
```solix
<expression> ';'
```

### Underlying Compiler & Runtime Mechanism
- Evaluates expression for side effects (assignment, method call, increment).
- If the evaluated expression leaves an unused return value on the VM operand stack, the assembler emits a `POP` opcode to maintain balanced stack height.

### Valid Variations
1. **Method Call Side Effect**: `Console.println("Log");`
2. **Increment / Decrement Expression**: `counter++; --index;`
3. **Assignment Expression**: `total = a + b;`

### Invalid Variations & Expected Errors
1. **Missing Semicolon**:
   - `Console.println("Log")`  
     *Error*: `Expected ';' after expression`
2. **Naked Meaningless Expression**:
   - `42 + 10;` (evaluated and discarded with warning)

---

## 14. `IfStatement` (`NodeType::IF_STMT`)

### Syntax
```solix
'if' '(' <condition-expr> ')' <then-statement> ['else' <else-statement>]
```

### Underlying Compiler & Runtime Mechanism
- Evaluates condition: must evaluate strictly to `bool`.
- Emits conditional jump `JUMP_IF_FALSE` to the `else` label or exit label.
- Any reference variables declared inside the `then` branch have their ARC cleanups injected before jumping to exit.
- `else` branch followed by exit label.

### Valid Variations
1. **Single Branch**: `if (is_valid) { execute(); }`
2. **Two-Way Branch**: `if (flag) { do_a(); } else { do_b(); }`
3. **Multi-Way Else-If Chain**:
   ```solix
   if (score >= 90) { grade = 'A'; }
   else if (score >= 80) { grade = 'B'; }
   else { grade = 'C'; }
   ```

### Invalid Variations & Expected Errors
1. **Condition Is Not Boolean**:
   - `if (1) { ... }`  
     *Error*: `Condition must be bool`
   - `if (new Object()) { ... }`  
     *Error*: `Condition must be bool`
2. **Missing Parentheses**:
   - `if x > 0 { ... }`  
     *Error*: `Expected '(' after 'if'`

---

## 15. `WhileStatement` (`NodeType::WHILE_STMT`)

### Syntax
```solix
'while' '(' <condition-expr> ')' <statement>
```

### Underlying Compiler & Runtime Mechanism
- Creates loop start label.
- Evaluates condition: must evaluate to `bool`.
- Emits `JUMP_IF_FALSE` to loop exit label.
- Loop body compiles with active loop context (for `break` and `continue` targets).
- Tail emits unconditional `JUMP` back to loop start.
- Loop exit label cleans up loop-level activation state.

### Valid Variations
1. **Standard Counter Loop**: `while (i < 10) { i++; }`
2. **Loop with Embedded Break & Continue**:
   ```solix
   while (has_next()) {
       Item item = get_next();
       if (item.skip) continue; // item cleaned by ARC
       if (item.done) break;    // item cleaned by ARC
       process(item);
   }
   ```

### Invalid Variations & Expected Errors
1. **Condition Is Not Boolean**:
   - `while (100) { ... }`  
     *Error*: `Condition must be bool`
2. **Missing Condition Expression**:
   - `while () { ... }`  
     *Error*: `Expected expression in while condition`

---

## 16. `DoWhileStatement` (`NodeType::DO_WHILE_STMT`)

### Syntax
```solix
'do' <statement> 'while' '(' <condition-expr> ')' ';'
```

### Underlying Compiler & Runtime Mechanism
- Emits loop body first without initial condition check.
- Emits condition check at bottom.
- Evaluates condition (`bool` required).
- Emits `JUMP_IF_TRUE` back to body start.
- Post-loop exit label.

### Valid Variations
1. **Guaranteed Initial Iteration**:
   ```solix
   do {
       step();
   } while (should_continue());
   ```

### Invalid Variations & Expected Errors
1. **Missing `while` Keyword**:
   - `do { step(); }`  
     *Error*: `Expected 'while' after do body`
2. **Missing Trailing Semicolon**:
   - `do { step(); } while (flag)`  
     *Error*: `Expected ';' after do-while condition`
3. **Condition Is Not Boolean**:
   - `do { ... } while ("yes");`  
     *Error*: `Condition must be bool`

---

## 17. `ForStatement` (`NodeType::FOR_STMT`)

### Syntax
```solix
'for' '(' [<init-stmt>] ';' [<condition-expr>] ';' [<step-expr>] ')' <statement>
```

### Underlying Compiler & Runtime Mechanism
- Pushes for-loop lexical scope.
- Compiles `init-stmt` (e.g. `int32 i = 0`).
- Emits loop start label.
- Compiles `condition-expr` (defaults to `true` if omitted). Emits `JUMP_IF_FALSE` to exit.
- Compiles body.
- Compiles `step-expr` (target for `continue`).
- Emits `JUMP` to condition check.
- Emits exit label; cleans up loop-scoped variables (like `i`).

### Valid Variations
1. **Standard 3-Clause Loop**: `for (int32 i = 0; i < len; i++) { ... }`
2. **Omitted Clauses**:
   - `for (;;) { if (done) break; }`
3. **Multiple Step Expressions**: `for (int32 i = 0; i < 10; i++, j--) { ... }`

### Invalid Variations & Expected Errors
1. **Condition Is Not Boolean**:
   - `for (int32 i = 0; i; i++)`  
     *Error*: `Condition must be bool`
2. **Loop Variable Leaking to Outer Scope**:
   - `for (int32 i = 0; i < 10; i++) {} i = 5;`  
     *Error*: `Undefined identifier: i`

---

## 18. `SwitchStatement` (`NodeType::SWITCH_STMT`) & `CaseStatement` (`NodeType::CASE_STMT`)

### Syntax
```solix
'switch' '(' <expr> ')' '{' ('case' <constant-literal> ':' <statement...>*)* ['default' ':' <statement...>*] '}'
```

### Underlying Compiler & Runtime Mechanism
- Evaluates switch expression (must be `int8`..`int64`, `uint8`..`uint64`, `char`, or `enum`).
- Compiles jump table comparing expression value with each constant case.
- Supports fallthrough: if a `case` does not end with `break;` or `return;`, execution flows into the next case instructions.
- `default` branch executes if no cases match.

### Valid Variations
1. **Integer Switching**: `switch (code) { case 200: ... break; default: ... break; }`
2. **Enum Switching**: `switch (state) { case State.IDLE: ... break; }`
3. **Fallthrough Cascading**:
   ```solix
   switch (val) {
       case 1:
       case 2: log_low(); break;
       case 3: log_high(); break;
   }
   ```

### Invalid Variations & Expected Errors
1. **Switching on Float, String, or Object Reference**:
   - `switch (3.14) { ... }`  
     *Error*: `Switch expression must evaluate to integer, char, or enum`
   - `switch (new Object()) { ... }`  
     *Error*: `Switch expression must evaluate to integer, char, or enum`
2. **Non-Constant Expression in Case**:
   - `case variable_x: ...`  
     *Error*: `Case value must be a constant literal expression`
3. **Duplicate Case Values**:
   - `case 1: ... case 1: ...`  
     *Error*: `Duplicate case value '1'`
4. **Multiple Default Blocks**:
   - `default: ... default: ...`  
     *Error*: `Switch statement can only have one default branch`

---

## 19. `BreakStatement` (`NodeType::BREAK_STMT`)

### Syntax
```solix
'break' ';'
```

### Underlying Compiler & Runtime Mechanism
- Scans up AST for the innermost enclosing `for`, `while`, `do-while`, or `switch`.
- Emits ARC `DEC_REF` cleanups for all local variables active in the scopes between the `break` statement and the enclosing loop boundary.
- Emits unconditional `JUMP` to the loop/switch exit label.

### Valid Variations
1. **Exiting Loop**: `for (int32 i = 0; i < 10; i++) { if (i == 5) break; }`
2. **Exiting Switch**: `case 1: do_work(); break;`

### Invalid Variations & Expected Errors
1. **Break Outside Loop or Switch**:
   - `void test() { break; }`  
     *Error*: `Break statement outside of loop or switch`

---

## 20. `ContinueStatement` (`NodeType::CONTINUE_STMT`)

### Syntax
```solix
'continue' ';'
```

### Underlying Compiler & Runtime Mechanism
- Scans up AST for the innermost enclosing `for`, `while`, or `do-while`.
- Emits ARC `DEC_REF` cleanups for local variables active in the scopes within the current loop iteration.
- Emits unconditional `JUMP` to the step/condition evaluation label of the loop.

### Valid Variations
1. **Skipping Iteration in For Loop**: `for (int32 i = 0; i < 10; i++) { if (skip[i]) continue; }`
2. **Skipping Iteration in While Loop**: `while (read()) { if (invalid) continue; }`

### Invalid Variations & Expected Errors
1. **Continue Outside Loop**:
   - `void test() { continue; }`  
     *Error*: `Continue statement outside of loop`
2. **Continue Inside Switch (Without Outer Loop)**:
   - `switch (x) { case 1: continue; }`  
     *Error*: `Continue statement outside of loop`

---

## 21. `ReturnStatement` (`NodeType::RETURN_STMT`)

### Syntax
```solix
'return' [<expression>] ';'
```

### Underlying Compiler & Runtime Mechanism
- **Return Value Preservation**:
  - If returning a reference object, the compiler compiles the expression into a designated return register and increments its reference count (`INC_REF`).
- **Activation Frame Unwinding**:
  - Emits `DEC_REF` / `RELEASE` for all local variables across all active lexical blocks up to function root.
- Emits `RET` opcode.

### Valid Variations
1. **Void Function Return**: `void test() { return; }`
2. **Value Return**: `int32 add(int32 a, int32 b) { return a + b; }`
3. **Reference Object Return (ARC Safe)**:
   ```solix
   String create() {
       String s = new String("safe");
       return s; // Safely returned without dangling pointer
   }
   ```

### Invalid Variations & Expected Errors
1. **Returning Value from Void Function**:
   - `void test() { return 42; }`  
     *Error*: `Cannot return a value from void function`
2. **Returning Void from Non-Void Function**:
   - `int32 test() { return; }`  
     *Error*: `Expected return value of type 'int32'`
3. **Type Mismatch in Return**:
   - `int32 test() { return "hello"; }`  
     *Error*: `Type mismatch in return statement: expected 'int32', got 'char[]'`

---

## 22. `ThrowStatement` (`NodeType::THROW_STMT`)

### Syntax
```solix
'throw' <expression> ';'
```

### Underlying Compiler & Runtime Mechanism
- Evaluates expression (must inherit from `Exception`).
- Emits `THROW` opcode.
- VM intercepts thrown object, records active exception, and begins unwinding activation frames.
- Local variables in intermediate activation frames have their ARC counts decremented during stack unwinding until a matching `catch` handler or top-level crash is reached.

### Valid Variations
1. **Throwing Instantiated Exception**: `throw new NullPointerException("Null ref");`
2. **Rethrowing Caught Exception**: `catch (Exception e) { throw e; }`

### Invalid Variations & Expected Errors
1. **Throwing Primitive Integer / Float**:
   - `throw 404;`  
     *Error*: `Can only throw instances of Exception or its subclasses`
2. **Throwing String Literal**:
   - `throw "Fatal error";`  
     *Error*: `Can only throw instances of Exception or its subclasses`
3. **Throwing Unrelated Class**:
   - `class Node {} throw new Node();`  
     *Error*: `Can only throw instances of Exception or its subclasses`

---

## 23. `TryCatchFinallyStatement` (`NodeType::TRY_STMT`, `CATCH_CLAUSE`)

### Syntax
```solix
'try' <block> ('catch' '(' <type-info> <identifier> ')' <block>)* ['finally' <block>]
```

### Underlying Compiler & Runtime Mechanism
- Compiles exception handler table entry with bytecode boundaries `[try_start, try_end]`.
- For each `catch` clause, registers handler address and target `vtable_id`.
- Handled at runtime via `THROW_EXCEPTION`, `GET_EXCEPTION`, and `CLEAR_EXCEPTION` opcodes.
- On exception, VM checks if the thrown exception conforms to target `vtable_id` (via vtable inheritance tree).
- `finally` block compiles into a trampoline: executed on normal fallthrough, on uncaught exceptions during unwinding, and before executing any `return` inside `try` or `catch` (via `REGISTER_RETURN_CLEANUP` and `JMP_TO_OUTER_CLEANUP`).

### Valid Variations
1. **Standard Try-Catch**: `try { risky(); } catch (IOException e) { log(e); }`
2. **Multi-Catch Hierarchy**:
   ```solix
   try {
       work();
   } catch (NullPointerException e) {
       handle_null();
   } catch (Exception e) {
       handle_general();
   }
   ```
3. **Try-Catch-Finally**:
   ```solix
   try {
       open();
   } catch (Exception e) {
       recover();
   } finally {
       close(); // Guaranteed execution
   }
   ```

### Invalid Variations & Expected Errors
1. **Try Without Catch or Finally**:
   - `try { work(); }`  
     *Error*: `Try statement must have at least one catch clause or finally block`
2. **Catching Non-Exception Type**:
   - `catch (int32 code) { ... }`  
     *Error*: `Catch parameter must inherit from Exception`
3. **Shadowed Unreachable Catch Block**:
   - `catch (Exception e) { ... } catch (NullPointerException npe) { ... }`  
     *Error*: `Catch block for 'NullPointerException' is unreachable (already caught by 'Exception')`

---

# Part V: Expressions & Operators

## 24. `AssignmentExpression` (`NodeType::ASSIGNMENT_EXPR`)

### Syntax
```solix
<lvalue> ('=' | '+=' | '-=' | '*=' | '/=' | '%=') <expression>
```

### Underlying Compiler & Runtime Mechanism
- Evaluates RHS expression.
- If LHS is a reference variable, emits `INC_REF` for new value and `DEC_REF` for previous occupant.
- If LHS is a static field on a qualified path (`Config.timeout = 5`), emits `SET_GLOBAL` directly without compiling the class receiver as a runtime instance value.
- If class has overloaded `operator=`, transforms assignment into method call.

### Valid Variations
1. **Local Variable Assignment**: `x = 10; s = new String("new");`
2. **Compound Numeric Assignment**: `x += 5; x *= 2;`
3. **Static Class Field Assignment**: `solix.core.Config.debug = true;`
4. **Array Element Assignment**: `arr[0] = 42; matrix[1][2] = 99;`
5. **Instance Member Assignment**: `player.hp = 100;`

### Invalid Variations & Expected Errors
1. **Assigning to Non-LValue (Literal, Binary, or Unary Expression)**:
   - `10 = x;` or `(a + b) = 5;` or `-x = 2;`  
     *Error*: Parse error: `Invalid assignment target`
2. **Assigning to Const Variable**:
   - `const int32 MAX = 10; MAX = 20;`  
     *Error*: `Cannot assign to const variable 'MAX'`
3. **Assignment Type Mismatch**:
   - `int32 x = 0; x = "str";`  
     *Error*: `Assignment type mismatch: 'int32' vs 'char[]'`

---

## 25. `TernaryExpression` (`NodeType::TERNARY_EXPR`)

### Syntax
```solix
<condition-expr> '?' <true-expr> ':' <false-expr>
```

### Underlying Compiler & Runtime Mechanism
- Evaluates condition: must evaluate strictly to `bool`.
- Emits conditional jump past true branch.
- Validates that `true-expr` and `false-expr` yield matching types.
- Evaluates only the chosen branch at runtime.

### Valid Variations
1. **Primitive Numeric Ternary**: `int32 max = (a > b) ? a : b;`
2. **Reference Ternary**: `String s = (p != null) ? p.name : "Default";`
3. **Nested Ternary**: `int32 sign = (x > 0) ? 1 : ((x < 0) ? -1 : 0);`

### Invalid Variations & Expected Errors
1. **Condition Is Not Bool**:
   - `int32 v = 1 ? 10 : 20;`  
     *Error*: `Ternary condition must be bool`
2. **Mismatched Branch Types**:
   - `auto v = (flag) ? 10 : "text";`  
     *Error*: `Ternary branches must have the same type`
3. **Missing Colon**:
   - `int32 v = (flag) ? 10;`  
     *Error*: `Expected ':' in ternary expression`

---

## 26. `BinaryExpression` (`NodeType::BINARY_EXPR`)

### Syntax
```solix
<left-expr> <operator> <right-expr>
```
*Operators*: `+`, `-`, `*`, `/`, `%`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||`, `&`

### Underlying Compiler & Runtime Mechanism
- Resolves operator overload on left operand if class type.
- Emits dedicated typed ALU opcodes:
  - `int32`: `ADD_I32`, `SUB_I32`, `MUL_I32`, `DIV_I32`
  - `int64`: `ADD_I64`, `SUB_I64`, `MUL_I64`, `DIV_I64`
  - `float64`: `ADD_F64`, `SUB_F64`, `MUL_F64`, `DIV_F64`
- Reference equality: checks raw heap addresses.
- Null equality: supports null on either side.

### Valid Variations
1. **Typed Primitive Arithmetic**: `int64 c = a + b; float64 d = x * y;`
2. **Subtype Null Equality**: `if (obj == null) ... if (null != obj) ...`
3. **Logical Short-Circuit**: `if (ptr != null && ptr.val > 0) { ... }`

### Invalid Variations & Expected Errors
1. **Operands Type Mismatch**:
   - `int32 res = 10 + 5.5;`  
     *Error*: `Binary operands type mismatch: 'int32' vs 'float64'`
2. **Integer Division by Zero (Runtime)**:
   - `int32 q = 10 / 0;`  
     *Runtime Exception*: `DivideByZero`
3. **Comparing Incompatible Types**:
   - `if (new Dog() == new Engine())`  
     *Error*: `Cannot compare unrelated types 'Dog' and 'Engine'`

---

## 27. `UnaryExpression` (`NodeType::UNARY_EXPR`)

### Syntax
```solix
('-' | '+' | '!' | '++' | '--') <expr>   // Prefix
<expr> ('++' | '--')                    // Postfix
```

### Underlying Compiler & Runtime Mechanism
- Negation (`-`): emits `NEG_I32`, `NEG_I64`, or `NEG_F64`.
- Inversion (`!`): requires `bool`, emits `LOGICAL_NOT`.
- Postfix `++`: evaluates value, duplicates, increments lvalue in place, leaves original value on stack.
- Prefix `++`: increments lvalue in place, leaves incremented value on stack.

### Valid Variations
1. **Prefix Increment / Decrement**: `++count; --count;`
2. **Postfix Increment / Decrement**: `int32 old = count++;`
3. **Numeric Negation**: `int64 neg = -big_val;`
4. **Boolean Inversion**: `bool opposite = !flag;`

### Invalid Variations & Expected Errors
1. **Unary Not on Non-Bool**:
   - `int32 x = !0;`  
     *Error*: `Unary operator '!' requires bool operand`
2. **Incrementing Literal or R-Value**:
   - `5++;` or `(a + b)++;`  
     *Error*: `Increment/decrement operand must be an assignable variable (lvalue)`

---

## 28. `CastExpression` (`NodeType::CAST_EXPR`)

### Syntax
```solix
'(' <target-type> ')' <expression>
```

### Underlying Compiler & Runtime Mechanism
- Evaluates operand expression.
- Numeric cast emits conversion opcode (`I32_TO_I64`, `F64_TO_I32`, etc.).
- Upcast: zero runtime cost; verified by compiler via inheritance tree.
- Downcast: compiler verifies type hierarchy; runtime checks `vtable_id`.

### Valid Variations
1. **Numeric Truncation & Extension**: `int32 i = (int32)3.99; float64 f = (float64)10;`
2. **Scalar Char Conversion**: `int32 code = (int32)'A'; char c = (char)65;`
3. **Class Upcast**: `Animal a = (Animal)new Dog();`
4. **Class Downcast**: `Dog d = (Dog)animal_ref;`

### Invalid Variations & Expected Errors
1. **Cast Between Primitive and Class**:
   - `Dog d = (Dog)42;`  
     *Error*: `Cannot cast between primitive and class types`
2. **Cast Between Unrelated Classes**:
   - `Dog d = new Dog(); Engine e = (Engine)d;`  
     *Error*: `Cannot cast 'Dog' to 'Engine' - no inheritance relationship`

---

## 29. `InstanceOfExpression` (`NodeType::INSTANCEOF_EXPR`)

### Syntax
```solix
<expression> 'instanceof' <class-type>
```

### Underlying Compiler & Runtime Mechanism
- Evaluates expression. If null, evaluates to `false`.
- Reads `vtable_id` from instance word 0.
- Traverses base vtable pointers in runtime metadata table to check if target type is an ancestor.
- Leaves `bool` on stack.

### Valid Variations
1. **Class Hierarchy Query**:
   ```solix
   if (entity instanceof Enemy) {
       Enemy e = (Enemy)entity;
   }
   ```

### Invalid Variations & Expected Errors
1. **Left Hand Side is Primitive**:
   - `if (10 instanceof int32)`  
     *Error*: `instanceof requires an object reference on left-hand side`
2. **Right Hand Side is Not a Class**:
   - `if (obj instanceof int32)`  
     *Error*: `Right-hand side of instanceof must be a class type`

---

## 30. `NewInstanceExpression` (`NodeType::NEW_INSTANCE`)

### Syntax
```solix
'new' <class-type> ['<' <type-args...> '>'] '(' <arguments...> ')'
```

### Underlying Compiler & Runtime Mechanism
- Emits `ALLOC` with class `instance_size`.
- Sets word 0 to `vtable_id` and initial ARC count = 1.
- Injects field default initializers.
- Emits `INVOKE_DIRECT` to chosen constructor matching argument types.
- Leaves allocated reference on stack.

### Valid Variations
1. **Default Constructor Call**: `Player p = new Player();`
2. **Parameterized Constructor Call**: `Player p = new Player(100);`
3. **Generic Class Instantiation**: `List<String> list = new List<String>();`

### Invalid Variations & Expected Errors
1. **Instantiating Abstract Class**:
   - `Shape s = new Shape();`  
     *Error*: `Cannot instantiate abstract class 'Shape'`
2. **No Matching Constructor Signature**:
   - `class A { public A(int32 x) {} } A a = new A();`  
     *Error*: `No matching constructor: A.ctor()`

---

## 31. `ArrayCreationExpression` (`NodeType::ARRAY_CREATION`)

### Syntax
```solix
'new' <element-type> '[' <size-expr> ']' ('[' <size-expr> ']')*
```

### Underlying Compiler & Runtime Mechanism
- Evaluates size expression (must be `int32`).
- Emits `ARRAY_ALLOC` with element count and type metadata.
- Sets array length header.
- Initializes all elements to zero or null.
- Multi-dimensional syntax compiles into nested allocation loops.

### Valid Variations
1. **1D Primitive Array**: `int32[] nums = new int32[10];`
2. **Multidimensional Array**: `int32[][] grid = new int32[5][5];`
3. **Object Reference Array**: `String[] words = new String[16];`

### Invalid Variations & Expected Errors
1. **Array Size Not int32**:
   - `int32[] arr = new int32[3.14];`  
     *Error*: `Array size must be int32`
2. **Negative Array Size (Runtime)**:
   - `int32[] arr = new int32[-5];`  
     *Runtime Exception*: `IndexOutOfBounds`

---

## 32. `ArrayAccessExpression` (`NodeType::ARRAY_ACCESS`)

### Syntax
```solix
<array-expr> '[' <index-expr> ']'
```

### Underlying Compiler & Runtime Mechanism
- Evaluates array reference. If null, VM raises `NullPointer`.
- Evaluates index (must be `int32`).
- VM executes hardware/software bounds check: verifies `0 <= index < length`.
- Emits `ARRAY_LOAD` or prepares destination for `ARRAY_STORE`.

### Valid Variations
1. **1D Array Access**: `int32 val = arr[0]; arr[0] = 5;`
2. **Multidimensional Access**: `int32 cell = grid[row][col];`

### Invalid Variations & Expected Errors
1. **Non-Integer Index**:
   - `int32 v = arr["first"];`  
     *Error*: `Array index must be int32`
2. **Indexing Non-Array**:
   - `int32 x = 42; int32 v = x[0];`  
     *Error*: `Cannot index a non-array value`
3. **Out-of-Bounds Subscript (Runtime)**:
   - `int32[] arr = new int32[3]; int32 v = arr[10];`  
     *Runtime Exception*: `IndexOutOfBounds`

---

## 33. `ArrayLiteralExpression` (`NodeType::ARRAY_LITERAL`)

### Syntax
```solix
'[' <expression> (',' <expression>)* [','] ']'
'{' <expression> (',' <expression>)* [','] '}'
```

### Underlying Compiler & Runtime Mechanism
- Compiler infers element type from expressions.
- Polymorphic deduction: finds most specific common base class.
- Emits `ARRAY_ALLOC` with literal length followed by sequential stores.

### Valid Variations
1. **Square Bracket Primitive Array Literal**: `int32[] arr = [1, 2, 3, 4, 5];`
2. **Curly Brace Primitive Array Literal**: `int32[] arr2 = {10, 20, 30};`
3. **Polymorphic Object Array Literal**: `Animal[] pets = [new Dog(), new Cat()];`

### Invalid Variations & Expected Errors
1. **Heterogeneous Incompatible Elements**:
   - `auto arr = [1, "two", new Dog()];`  
     *Error*: `Mixed types in array literal`

---

## 34. `MemberAccessExpression` (`NodeType::MEMBER_ACCESS`)

### Syntax
```solix
<expression> '.' <identifier>
<namespace-path> '::' <identifier>
```

### Underlying Compiler & Runtime Mechanism
- Checks if LHS is a symbol path (`extract_symbol_path`): if it resolves to a `ClassDeclaration` or `EnumDeclaration`, accesses static member without instance evaluation.
- If instance, calculates field memory offset or dispatches method.
- Special property `.length` on arrays reads length header.

### Valid Variations
1. **Instance Field Access**: `int32 hp = player.hp;`
2. **Array Length Access**: `int32 len = arr.length;`
3. **Static Member via Full Path**: `solix.core.Objects.is_null(obj);`
4. **Static Member via Partial Path**: `core.Objects.is_null(obj);`
5. **Static Member via C++ Scope Operator**: `core::Objects::is_null(obj);`

### Invalid Variations & Expected Errors
1. **Accessing Private Member Outside Class**:
   - `class A { private int32 x; } A a = new A(); a.x = 10;`  
     *Error*: `Cannot access private member of class 'A'`
2. **Accessing Non-Existent Property on Array**:
   - `int32 s = arr.size;`  
     *Error*: `Arrays only have the 'length' property`
3. **Dereferencing Null Object (Runtime)**:
   - `Player p = (Player)null; int32 hp = p.health;`  
     *Runtime Exception*: `NullPointer`

---

## 35. `MethodCallExpression` (`NodeType::METHOD_CALL`)

### Syntax
```solix
<callee-expr> ['<' <type-args...> '>'] '(' <arguments...> ')'
```

### Underlying Compiler & Runtime Mechanism
- Resolves callee signature matching argument count and types.
- If virtual, dispatches dynamically via vtable (`INVOKE_VIRTUAL`).
- If static, dispatches via `INVOKE_STATIC`.
- Template calls trigger monomorphization if concrete instantiation is not yet compiled.
- Return value pushed to operand stack.

### Valid Variations
1. **Instance Method Call**: `player.take_damage(25);`
2. **Static Method Call**: `Math.max(10, 20);`
3. **Generic Method Call with Explicit Type**: `Arrays.swap<int32>(arr, 0, 1);`
4. **Generic Method Call with Implicit Deduction**: `min(10, 20);`

### Invalid Variations & Expected Errors
1. **No Matching Method Signature**:
   - `player.take_damage("twenty");`  
     *Error*: `No matching method: Player.take_damage(char[])`
2. **Calling Method on Null Reference (Runtime)**:
   - `String s = (String)null; s.size();`  
     *Runtime Exception*: `NullPointer`

---

## 36. `IdentifierNode` (`NodeType::IDENTIFIER`)

### Syntax
```solix
<identifier>
```

### Underlying Compiler & Runtime Mechanism
- Resolved via `resolve_symbol`:
  1. Active local scopes (variables, parameters).
  2. Current class fields / methods (`this`).
  3. Imported symbols table.
  4. Active package prefix.
  5. Global scope exact match.
  6. Sub-namespace suffix search across all registered symbols.
- Emits ambiguity error if multiple packages define the same name without explicit import.

### Valid Variations
1. **Local Variable Identifier**: `x`
2. **Unqualified Unique Class Name**: `String s = ...;`
3. **Explicitly Imported Name**: `import core.String; String s = ...;`

### Invalid Variations & Expected Errors
1. **Undefined Identifier**:
   - `x = 10;` (without `int32 x;`)  
     *Error*: `Undefined identifier: x`
2. **Ambiguous Identifier Collision**:
   - `Item.code();` where both `pkg_a.Item` and `pkg_b.Item` exist  
     *Error*: `Ambiguous symbol 'Item': multiple candidates found (pkg_a.Item, pkg_b.Item). Specify full package or use import to disambiguate.`

---

## 37. `LiteralNode` (`NodeType::LITERAL`)

### Syntax
- Integer: `123`, `0xFF`
- Float: `3.14`, `0.5`
- Character: `'a'`, `'\n'`
- String: `"hello"`
- Boolean: `true`, `false`
- Null: `null`

### Underlying Compiler & Runtime Mechanism
- Primitive literals emitted directly as immediate bytecode operands (`PUSH_I32`, `PUSH_I64`, `PUSH_F64`).
- String literals interned in bytecode header constant pool.
- Null emitted as `PUSH_NULL` (reference address 0).

### Valid Variations
1. **Numeric Literals**: `100`, `0x1F`, `3.14159`
2. **Escaped Chars**: `'\n'`, `'\t'`, `'\\'`, `'\''`
3. **String Literals**: `"Solix Language"`
4. **Bool Literals**: `true`, `false`
5. **Null Literal**: `null`

### Invalid Variations & Expected Errors
1. **Unterminated String**:
   - `"hello`  
     *Error*: `Unterminated string literal`
2. **Empty Character Literal**:
   - `''`  
     *Error*: `Empty character literal`
3. **Multi-Character Character Literal**:
   - `'abc'`  
     *Error*: `Character literal contains multiple characters`
