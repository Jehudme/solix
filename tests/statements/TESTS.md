# Solix Test Specification

This document is the definitive master test specification for the Solix language test suite (Phase 8). It defines the comprehensive matrix of **Positive Test Scenarios** (valid syntax, semantics, and runtime execution guarantees) and **Negative Test Scenarios** (expected compile-time errors and runtime exceptions) for every construct in the language.

## Table of Contents

- **Part I: Compilation Unit & Module Statements**
  - [AliasStatement](#aliasstatement)
  - [ImportStatement](#importstatement)
  - [PackageStatement](#packagestatement)
- **Part II: Declarations**
  - [ClassDeclaration](#classdeclaration)
  - [ConstructorDeclaration](#constructordeclaration)
  - [EnumDeclaration](#enumdeclaration)
  - [FieldDeclaration](#fielddeclaration)
  - [InterfaceDeclaration](#interfacedeclaration)
  - [MethodDeclaration](#methoddeclaration)
  - [OperatorDeclaration](#operatordeclaration)
- **Part III: Control Flow & Execution Statements**
  - [BlockStatement](#blockstatement)
  - [BreakStatement](#breakstatement)
  - [ContinueStatement](#continuestatement)
  - [DoWhileStatement](#dowhilestatement)
  - [ExpressionStatement](#expressionstatement)
  - [ForStatement](#forstatement)
  - [IfStatement](#ifstatement)
  - [ReturnStatement](#returnstatement)
  - [SwitchStatement](#switchstatement)
  - [ThrowStatement](#throwstatement)
  - [TryCatchFinallyStatement](#trycatchfinallystatement)
  - [VariableDeclarationStatement](#variabledeclarationstatement)
  - [WhileStatement](#whilestatement)
- **Part IV: Expressions & Operators**
  - [ArrayAccessExpression](#arrayaccessexpression)
  - [ArrayCreationExpression](#arraycreationexpression)
  - [ArrayLiteralExpression](#arrayliteralexpression)
  - [AssignmentExpression](#assignmentexpression)
  - [BinaryExpression](#binaryexpression)
  - [CastExpression](#castexpression)
  - [IdentifierNode](#identifiernode)
  - [InstanceOfExpression](#instanceofexpression)
  - [LiteralNode](#literalnode)
  - [MemberAccessExpression](#memberaccessexpression)
  - [MethodCallExpression](#methodcallexpression)
  - [NewInstanceExpression](#newinstanceexpression)
  - [TernaryExpression](#ternaryexpression)
  - [UnaryExpression](#unaryexpression)

---

# Part I: Compilation Unit & Module Statements

## AliasStatement

*Specification Reference*: [AliasStatement](statements/modules/alias_statement.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Primitive Synonym [IMPLEMENTED]
```solix
alias Byte = uint8;

void test() {
    Byte b = 255;
}
```
*Expected Result*: `Byte` compiles as a raw `uint8` with zero wrapper overhead.

### Case 3.2: Parameterized Generic Alias [IMPLEMENTED]
```solix
alias StringMap<V> = Map<String, V>;

void test() {
    StringMap<int32> map = new StringMap<int32>();
}
```
*Expected Result*: Substituted to `Map<String, int32>`; compiles cleanly.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Circular Alias Definition [IMPLEMENTED]
```solix
alias A = B;
alias B = A; // Error: circular alias
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Circular alias detected in 'A'
```

### Case 4.2: Generic Parameter Arity Mismatch [IMPLEMENTED]
```solix
alias Pair<K, V> = Map<K, V>;

void test() {
    Pair<String> bad; // Error: expected 2 parameters
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Alias 'Pair' expects 2 generic type arguments, got 1
```

---

## ImportStatement

*Specification Reference*: [ImportStatement](statements/modules/import_statement.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Selective Import [IMPLEMENTED]
```solix
import std.collections.List;

void test() {
    List items = new List();
}
```
*Expected Result*: Compiles cleanly; `List` resolves to `std.collections.List`.

### Case 3.2: Wildcard Import [IMPLEMENTED]
```solix
import std.io.*;

void test() {
    Console.println("Hello");
}
```
*Expected Result*: `Console` resolves to `std.io.Console`.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Importing Non-Existent Package [IMPLEMENTED]
```solix
import invalid.pkg.Foo;
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot resolve import 'invalid.pkg.Foo': package or symbol not found
```

### Case 4.2: Ambiguous Symbol Collision [IMPLEMENTED]
```solix
import pkg_a.*; // defines Token
import pkg_b.*; // also defines Token

void test() {
    Token t; // Error: ambiguous
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Reference to 'Token' is ambiguous: matches 'pkg_a.Token' and 'pkg_b.Token'
```

---

## PackageStatement

*Specification Reference*: [PackageStatement](statements/modules/package_statement.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Hierarchical Multi-Level Package [IMPLEMENTED]
```solix
package std.collections.generic;

class CustomList {}
```
*Expected Result*: Registered as `std.collections.generic.CustomList`; compiles cleanly.

### Case 3.2: Intra-Package Unqualified Access [IMPLEMENTED]
Two files sharing the same package can reference each other without imports.
```solix
// File 1
package app.models;
class User { String name; }

// File 2
package app.models;
class Account { User owner; }
```
*Expected Result*: `Account` resolves `User` without needing an `import` statement.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Package Statement Not First [IMPLEMENTED]
```solix
import std.io;
package app; // Error: package must appear before imports
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: 'package' statement must be the first statement in the file
```

### Case 4.2: Duplicate Package Statement [IMPLEMENTED]
```solix
package alpha;
package beta; // Error: duplicate
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Only one 'package' statement is allowed per file
```

---

# Part II: Declarations

## ClassDeclaration

*Specification Reference*: [ClassDeclaration](statements/declarations/class_declaration.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Single Inheritance and Dynamic Dispatch [IMPLEMENTED]
```solix
class Animal { String sound() { return "generic"; } }
class Cat extends Animal { String sound() { return "meow"; } }

void test() {
    Animal pet = new Cat();
    Console.println(pet.sound()); // Prints: meow
}
```
*Expected Result*: Dispatches dynamically to `Cat.sound()`.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Circular Class Inheritance [IMPLEMENTED]
```solix
class A extends B {}
class B extends A {}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Circular inheritance detected for class 'A'
```

### Case 4.2: Unimplemented Abstract Method [IMPLEMENTED]
```solix
abstract class Shape { abstract float64 area(); }
class Circle extends Shape {} // Error: missing area()
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Class 'Circle' must implement abstract method 'area()' from 'Shape'
```

---

## ConstructorDeclaration

*Specification Reference*: [ConstructorDeclaration](statements/declarations/constructor_declaration.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Chained Super Constructor [IMPLEMENTED]
```solix
class Base { int32 id; Base(int32 id) { this.id = id; } }
class Sub extends Base { Sub(int32 id) : super(id) {} }
```
*Expected Result*: Invokes `Base` constructor before running `Sub` constructor.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Constructor Name Mismatch [IMPLEMENTED]
```solix
class Widget {
    Gadget() {} // Error: name mismatch
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Constructor name 'Gadget' does not match enclosing class 'Widget'
```

---

## EnumDeclaration

*Specification Reference*: [EnumDeclaration](statements/declarations/enum_declaration.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Enum Equality and Switch [IMPLEMENTED]
```solix
enum Color { RED, GREEN, BLUE }

void test(Color c) {
    if (c == Color.RED) {
        Console.println("Red");
    }
}
```
*Expected Result*: Compiles and compares correctly via `EQ_I64`.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Duplicate Enum Member [IMPLEMENTED]
```solix
enum State { READY, READY }
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Duplicate enum member 'READY' in enum 'State'
```

---

## FieldDeclaration

*Specification Reference*: [FieldDeclaration](statements/declarations/field_declaration.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Cycle Breaking with Weak References [IMPLEMENTED]
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

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Duplicate Field Identifier [IMPLEMENTED]
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

### Case 4.2: Field Access on Null Reference (Runtime Fault) [IMPLEMENTED]
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

---

## InterfaceDeclaration

*Specification Reference*: [InterfaceDeclaration](statements/declarations/interface_declaration.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Multiple Interface Conformance [NOT IMPLEMENTED]
```solix
interface Printable { void print(); }
interface Serializable { void save(); }

class Doc implements Printable, Serializable {
    void print() { Console.println("print"); }
    void save() { Console.println("save"); }
}
```
*Expected Result*: Compiles cleanly; instances can be passed to functions taking either `Printable` or `Serializable`.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Interface Method with Body [NOT IMPLEMENTED]
```solix
interface Reader {
    int32 read() { return 0; } // Error: method cannot have body
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Interface methods cannot have a body
```

---

## MethodDeclaration

*Specification Reference*: [MethodDeclaration](statements/declarations/method_declaration.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Virtual Method Overriding [IMPLEMENTED]
```solix
class Parent { String name() { return "parent"; } }
class Child extends Parent { String name() { return "child"; } }

void test() {
    Parent p = new Child();
    Console.println(p.name()); // Prints: child
}
```
*Expected Result*: Dispatches dynamically to `Child.name()`.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Abstract Method with Body [IMPLEMENTED]
```solix
abstract class Base {
    abstract void run() {} // Error: cannot have body
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Abstract method 'run' cannot have a body
```

---

## OperatorDeclaration

*Specification Reference*: [OperatorDeclaration](statements/declarations/operator_declaration.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Vector Addition Overload [IMPLEMENTED]
```solix
class Vector {
    int32 x;
    Vector(int32 x) { this.x = x; }
    Vector operator+(Vector other) {
        return new Vector(this.x + other.x);
    }
}

void test() {
    Vector v1 = new Vector(10);
    Vector v2 = new Vector(20);
    Vector v3 = v1 + v2; // Calls operator+
}
```
*Expected Result*: `v3.x` equals 30.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Unsupported Operator Overload [IMPLEMENTED]
```solix
class Test {
    bool operator&&(Test other) {} // Error: unsupported
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Operator '&&' cannot be overloaded
```

---

# Part III: Control Flow & Execution Statements

## BlockStatement

*Specification Reference*: [BlockStatement](statements/control_flow/block_statement.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Empty and Nested Empty Blocks [IMPLEMENTED]
Empty blocks compile to zero runtime instructions and produce zero stack delta.
```solix
void test() {
    {}
    {
        {}
        { {} }
    }
}
```
*Expected Result*: Compiles and runs cleanly; stack remains balanced.

### Case 3.2: Lexical Variable Shadowing [IMPLEMENTED]
An inner block can shadow an outer variable with a different type.
```solix
int32 value = 10;
{
    String value = new String("shadow");
    Console.println(value); // Prints: shadow
}
Console.println(value);     // Prints: 10
```
*Expected Result*: Outputs `"shadow"` then `10`. The inner `String` is freed at the inner closing brace; outer `int32` is unaffected.

### Case 3.3: Strict LIFO Destruction of Multiple Reference Objects [IMPLEMENTED]
Multiple objects in a block are decremented in reverse declaration order.
```solix
{
    String first = new String("first");
    String second = new String("second");
    String third = new String("third");
}
```
*Expected Result*: Bytecode executes `DEC_REF third`, then `DEC_REF second`, then `DEC_REF first`.

### Case 3.4: Early Return from Nested Blocks [IMPLEMENTED]
Returning from inside deep blocks unwinds all intermediate scopes.
```solix
int32 compute(bool early) {
    String a = new String("a");
    {
        String b = new String("b");
        if (early) {
            return 100; // Must clean up b and a before RETURN!
        }
    }
    return 0;
}
```
*Expected Result*: Calling `compute(true)` frees `b` and `a` with 0 memory leaks.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Accessing Block-Scoped Variable Outside Its Block [IMPLEMENTED]
```solix
void test() {
    {
        int32 temp = 100;
    }
    int32 leak = temp; // Error: temp does not exist
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Undefined identifier: temp
```

### Case 4.2: Duplicate Variable in Same Immediate Scope [IMPLEMENTED]
```solix
void test() {
    {
        int32 score = 10;
        float64 score = 20.0; // Error: duplicate declaration
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Variable 'score' is already defined in the current scope
```

### Case 4.3: Unclosed Block (Missing Brace) [IMPLEMENTED]
```solix
void test() {
    {
        int32 x = 1;
// Missing '}'
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Syntax error: expected '}' before end of file
```

### Case 4.4: Stray Extra Closing Brace [IMPLEMENTED]
```solix
void test() {
    { int32 x = 1; }
    } // Extra brace
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Syntax error: unexpected token '}'
```

---

## BreakStatement

*Specification Reference*: [BreakStatement](statements/control_flow/break_statement.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Breaking Out of Deep Nested Blocks [IMPLEMENTED]
```solix
while (true) {
    String s1 = new String("s1");
    {
        String s2 = new String("s2");
        break; // Cleans up s2 and s1
    }
}
```
*Expected Result*: `s2` and `s1` are freed cleanly; 0 memory leaks.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Break Outside Loop or Switch [IMPLEMENTED]
```solix
void test() {
    int32 x = 10;
    break; // Error: break not inside loop/switch
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: 'break' statement not allowed outside of loop or switch
```

---

## ContinueStatement

*Specification Reference*: [ContinueStatement](statements/control_flow/continue_statement.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Continue Advances Loop Variable [IMPLEMENTED]
```solix
int32 hits = 0;
for (int32 i = 0; i < 6; i++) {
    if (i % 2 == 0) continue;
    hits++;
}
```
*Expected Result*: `hits` equals 3 (processed for 1, 3, 5).

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Continue Outside Loop [IMPLEMENTED]
```solix
void test() {
    continue; // Error: not in loop
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: 'continue' statement not allowed outside of loop
```

---

## DoWhileStatement

*Specification Reference*: [DoWhileStatement](statements/control_flow/do_while_statement.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Guaranteed Initial Pass with False Condition [IMPLEMENTED]
```solix
int32 ran = 0;
do {
    ran++;
} while (false);
```
*Expected Result*: `ran` equals 1; body ran exactly once.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Accessing Body Variable in Condition [IMPLEMENTED]
Variables declared inside the `do` block are not in scope in the condition.
```solix
void test() {
    do {
        int32 inner = 10;
    } while (inner > 0); // Error: inner undefined
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Undefined identifier: inner
```

---

## ExpressionStatement

*Specification Reference*: [ExpressionStatement](statements/control_flow/expression_statement.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Method Calls and Assignments [IMPLEMENTED]
```solix
int32 x = 0;
x = 10;
x++;
Console.println(x);
```
*Expected Result*: Evaluates side effects sequentially; stack depth remains 0 at every statement boundary.

### Case 3.2: Immediate Temporary Reclamation [IMPLEMENTED]
```solix
String generate() { return new String("temporary"); }

void test() {
    generate(); // Call and discard
}
```
*Expected Result*: Returned `String` is decremented via `DEC_REF` immediately; 0 heap leaks.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Missing Semicolon [IMPLEMENTED]
```solix
void test() {
    x = 10 // Missing ';'
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Syntax error: expected ';' after expression
```

### Case 4.2: Method Call on Null Reference (Runtime Fault) [IMPLEMENTED]
```solix
void test() {
    String s = null;
    s.length(); // Null pointer dereference
}
```
*Expected Runtime Exception*:
```text
[FATAL VM PANIC] NullReferenceException: Attempted to invoke method on null object reference
```

---

## ForStatement

*Specification Reference*: [ForStatement](statements/control_flow/for_statement.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Standard For Loop with Continue [IMPLEMENTED]
```solix
int32 evens = 0;
for (int32 i = 0; i < 10; i++) {
    if (i % 2 != 0) continue; // Jumps to i++
    evens++;
}
```
*Expected Result*: `evens` equals 5.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Induction Variable Leakage [IMPLEMENTED]
```solix
void test() {
    for (int32 i = 0; i < 5; i++) {}
    Console.println(i); // Error: i is out of scope
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Undefined identifier: i
```

---

## IfStatement

*Specification Reference*: [IfStatement](statements/control_flow/if_statement.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Single Branch If [IMPLEMENTED]
```solix
int32 x = 10;
if (x > 5) {
    x = 20;
}
```
*Expected Result*: `x` becomes 20.

### Case 3.2: If-Else Chain [IMPLEMENTED]
```solix
int32 score = 85;
char grade = 'F';
if (score >= 90) {
    grade = 'A';
} else if (score >= 80) {
    grade = 'B';
} else {
    grade = 'C';
}
```
*Expected Result*: `grade` evaluates to `'B'`.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Non-Boolean Condition Type [IMPLEMENTED]
```solix
void test() {
    int32 count = 1;
    if (count) { // Error: int32 not allowed as condition
        Console.println("yes");
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: If condition must be of type 'bool', got 'int32'
```

---

## ReturnStatement

*Specification Reference*: [ReturnStatement](statements/control_flow/return_statement.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Early Return from Nested Scopes [IMPLEMENTED]
```solix
int32 find(bool fast) {
    String a = new String("a");
    {
        String b = new String("b");
        if (fast) return 1;
    }
    return 0;
}
```
*Expected Result*: Returns 1; both `b` and `a` are deallocated.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Missing Return Value in Non-Void Method [IMPLEMENTED]
```solix
int32 get_val() {
    return; // Error: must return a value
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Must return a value from non-void method
```

### Case 4.2: Return Type Mismatch [IMPLEMENTED]
```solix
int32 get_num() {
    return "text"; // Error: String cannot convert to int32
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Return type mismatch: expected 'int32', got 'String'
```

---

## SwitchStatement

*Specification Reference*: [SwitchStatement](statements/control_flow/switch_statement.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Switch with Explicit Break [IMPLEMENTED]
```solix
int32 value = 2;
String name = "";
switch (value) {
    case 1: name = "one"; break;
    case 2: name = "two"; break;
    default: name = "other"; break;
}
```
*Expected Result*: `name` equals `"two"`.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Duplicate Case Constant [IMPLEMENTED]
```solix
void test(int32 x) {
    switch (x) {
        case 1: break;
        case 1: break; // Error: duplicate
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Duplicate case value '1' in switch statement
```

---

## ThrowStatement

*Specification Reference*: [ThrowStatement](statements/control_flow/throw_statement.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Throw Handled by Catch [IMPLEMENTED]
```solix
bool caught = false;
try {
    throw new std.Exception("test");
} catch (std.Exception e) {
    caught = true;
}
```
*Expected Result*: `caught` is `true`.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Throwing Primitive Value [IMPLEMENTED]
```solix
void test() {
    throw 404; // Error: cannot throw primitive
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot throw type 'int32': must inherit from 'std.Exception'
```

### Case 4.2: Throwing Null Reference at Runtime [IMPLEMENTED]
```solix
void test() {
    std.Exception e = null;
    throw e; // Throws NullReferenceException
}
```
*Expected Runtime Exception*:
```text
[FATAL VM PANIC] NullReferenceException: Attempted to throw null exception reference
```

---

## TryCatchFinallyStatement

*Specification Reference*: [TryCatchFinallyStatement](statements/control_flow/try_catch_finally_statement.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Specific Catch Hierarchy [IMPLEMENTED]
```solix
class CustomError extends std.Exception { CustomError() : super("Custom") {} }

int32 result = 0;
try {
    throw new CustomError();
} catch (CustomError c) {
    result = 1; // Matches here
} catch (std.Exception e) {
    result = 2;
}
```
*Expected Result*: `result` equals 1.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Unreachable Catch Clause [IMPLEMENTED]
```solix
class SubErr extends std.Exception {}

void test() {
    try {
        work();
    } catch (std.Exception e) {
    } catch (SubErr s) { // Error: unreachable catch
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Unreachable catch clause: 'SubErr' is already handled by preceding catch for 'std.Exception'
```

---

## VariableDeclarationStatement

*Specification Reference*: [VariableDeclarationStatement](statements/control_flow/variable_declaration.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Primitive and Reference Declarations [IMPLEMENTED]
```solix
int32 a = 1;
float64 b = 2.5;
bool c = true;
String s = new String("hello");
int32[] nums = new int32[5];
```
*Expected Result*: Compiles cleanly; each variable is assigned a contiguous slot in the activation frame.

### Case 3.2: Polymorphic Upcasting [IMPLEMENTED]
Assigning a derived class instance to a base class variable.
```solix
class Animal {}
class Cat extends Animal {}

void test() {
    Animal a = new Cat(); // Valid polymorphic binding
}
```
*Expected Result*: Passes type checking; `Cat` instance stored in `Animal` slot.

### Case 3.3: Interface Binding [NOT IMPLEMENTED]
```solix
interface Printable { void print(); }
class Doc implements Printable { void print() {} }

void test() {
    Printable p = new Doc();
}
```
*Expected Result*: Binds instance to interface variable with valid VTable slot.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Type Mismatch in Initializer [IMPLEMENTED]
```solix
void test() {
    int32 x = "hello"; // Incompatible types
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Type mismatch in variable declaration: expected 'int32', got 'String'
```

### Case 4.2: Duplicate Declaration in Same Scope [IMPLEMENTED]
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

### Case 4.3: Unknown Type Name [IMPLEMENTED]
```solix
void test() {
    NonExistentType obj = null;
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Unknown type 'NonExistentType'
```

### Case 4.4: Declaring Variable as Void [IMPLEMENTED]
```solix
void test() {
    void placeholder;
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Variable cannot be of type 'void'
```

---

## WhileStatement

*Specification Reference*: [WhileStatement](statements/control_flow/while_statement.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Standard Counted Loop [IMPLEMENTED]
```solix
int32 total = 0;
int32 i = 1;
while (i <= 5) {
    total = total + i;
    i++;
}
```
*Expected Result*: `total` equals 15.

### Case 3.2: While Loop with Break and Continue [IMPLEMENTED]
```solix
int32 sum = 0;
int32 i = 0;
while (true) {
    i++;
    if (i % 2 == 0) continue;
    if (i > 5) break;
    sum = sum + i;
}
```
*Expected Result*: `sum` equals 1 + 3 + 5 = 9.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Non-Boolean Loop Condition [IMPLEMENTED]
```solix
void test() {
    int32 x = 5;
    while (x) { // Error: x is not bool
        x--;
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: While condition must be of type 'bool', got 'int32'
```

---

# Part IV: Expressions & Operators

## ArrayAccessExpression

*Specification Reference*: [ArrayAccessExpression](statements/expressions/array_access_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: In-Bounds Read and Write [IMPLEMENTED]
```solix
int32[] buffer = new int32[3];
buffer[0] = 100;
buffer[1] = 200;
int32 sum = buffer[0] + buffer[1]; // 300
```
*Expected Result*: `sum` equals 300.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Index Out of Bounds (Runtime Fault) [IMPLEMENTED]
```solix
int32[] data = new int32[2];
int32 fail = data[5]; // Out of bounds
```
*Expected Runtime Exception*:
```text
[FATAL VM PANIC] IndexOutOfBoundsException: Index 5 out of bounds for array length 2
```

---

## ArrayCreationExpression

*Specification Reference*: [ArrayCreationExpression](statements/expressions/array_creation_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Zero-Initialized Array [IMPLEMENTED]
```solix
int32[] data = new int32[5];
int32 first = data[0]; // 0
```
*Expected Result*: `first` is 0.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Negative Size at Runtime (Runtime Fault) [IMPLEMENTED]
```solix
int32[] bad = new int32[-1];
```
*Expected Runtime Exception*:
```text
[FATAL VM PANIC] NegativeArraySizeException: Attempted to create array with negative size -1
```

---

## ArrayLiteralExpression

*Specification Reference*: [ArrayLiteralExpression](statements/expressions/array_literal_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Dual Literal Syntax [IMPLEMENTED]
```solix
int32[] a = [10, 20];
int32[] b = {10, 20}; // Both bracket and brace syntax supported
```
*Expected Result*: Both arrays initialize with identical elements.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Incompatible Literal Elements [IMPLEMENTED]
```solix
var arr = [10, "text"]; // Incompatible types
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Incompatible types in array literal
```

---

## AssignmentExpression

*Specification Reference*: [AssignmentExpression](statements/expressions/assignment_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Chained Assignment [NOT IMPLEMENTED]
```solix
int32 a; int32 b; int32 c;
a = b = c = 10;
```
*Expected Result*: `a`, `b`, and `c` all equal 10.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Assigning to Literal / RValue [NOT IMPLEMENTED]
```solix
void test() {
    10 = x; // Error: invalid lvalue
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Invalid assignment target
```

---

## BinaryExpression

*Specification Reference*: [BinaryExpression](statements/expressions/binary_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Short-Circuit Logical AND [NOT IMPLEMENTED]
```solix
bool result = false && (10 / 0 == 0); // Division by zero avoided
```
*Expected Result*: `result` is `false`.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Division by Zero (Runtime Fault) [NOT IMPLEMENTED]
```solix
void test() {
    int32 x = 10 / 0;
}
```
*Expected Runtime Exception*:
```text
[FATAL VM PANIC] ArithmeticException: Division by zero
```

---

## CastExpression

*Specification Reference*: [CastExpression](statements/expressions/cast_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Valid Downcast [NOT IMPLEMENTED]
```solix
Animal a = new Dog();
Dog d = (Dog)a; // Succeeds
```
*Expected Result*: `d` holds a valid reference to `Dog`.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Bad Downcast (Runtime Fault) [NOT IMPLEMENTED]
```solix
Animal a = new Cat();
Dog d = (Dog)a; // Fails!
```
*Expected Runtime Exception*:
```text
[FATAL VM PANIC] TypeCastException: Cannot cast 'Cat' to 'Dog'
```

---

## IdentifierNode

*Specification Reference*: [IdentifierNode](statements/expressions/identifier_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Local Resolution Precedence [NOT IMPLEMENTED]
```solix
int32 val = 100;
{
    int32 val = 200;
    Console.println(val); // Resolves to local slot (200)
}
```
*Expected Result*: Prints 200.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Undefined Identifier [NOT IMPLEMENTED]
```solix
void test() {
    int32 a = unknown_var;
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Undefined identifier: unknown_var
```

---

## InstanceOfExpression

*Specification Reference*: [InstanceOfExpression](statements/expressions/instanceof_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Safe Null Evaluation [NOT IMPLEMENTED]
```solix
Animal a = null;
bool check = a instanceof Dog; // false, no panic!
```
*Expected Result*: `check` is `false`.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Primitive Target [NOT IMPLEMENTED]
```solix
bool b = 10 instanceof int32; // Error
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: 'instanceof' cannot be applied to primitive types
```

---

## LiteralNode

*Specification Reference*: [LiteralNode](statements/expressions/literal_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: All Literal Types [NOT IMPLEMENTED]
```solix
int64 big = 10000000000L;
char letter = 'Z';
String empty = "";
```
*Expected Result*: All primitives compile and load accurately.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Integer Literal Overflow [NOT IMPLEMENTED]
```solix
int32 x = 99999999999999999999;
```
*Expected Compiler Diagnostic*:
```text
[ERROR] lexer.cpp: Integer literal out of range for type 'int32'
```

---

## MemberAccessExpression

*Specification Reference*: [MemberAccessExpression](statements/expressions/member_access_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Chained Member Access [NOT IMPLEMENTED]
```solix
class Address { String city; }
class Person { Address addr; }

void test(Person p) {
    String city = p.addr.city;
}
```
*Expected Result*: Resolves offsets sequentially; compiles cleanly.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Member Access on Null Reference (Runtime Fault) [NOT IMPLEMENTED]
```solix
Person p = null;
String c = p.addr; // Throws NullReferenceException
```
*Expected Runtime Exception*:
```text
[FATAL VM PANIC] NullReferenceException: Attempted to read property from null object reference
```

---

## MethodCallExpression

*Specification Reference*: [MethodCallExpression](statements/expressions/method_call_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Overloaded Method Selection [NOT IMPLEMENTED]
```solix
class Printer {
    void print(int32 x) { Console.println("int"); }
    void print(String s) { Console.println("str"); }
}

void test() {
    Printer p = new Printer();
    p.print(42);       // Calls print(int32)
    p.print("hello");  // Calls print(String)
}
```
*Expected Result*: First prints `"int"`, second prints `"str"`.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: No Matching Overload [NOT IMPLEMENTED]
```solix
void test(Printer p) {
    p.print(true); // Error: no boolean overload
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: No matching overload for method 'print' with arguments (bool)
```

---

## NewInstanceExpression

*Specification Reference*: [NewInstanceExpression](statements/expressions/new_instance_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Instantiation with Overloaded Constructor [NOT IMPLEMENTED]
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

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Instantiating Abstract Class [NOT IMPLEMENTED]
```solix
abstract class Base {}
Base b = new Base(); // Error
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot instantiate abstract class 'Base'
```

---

## TernaryExpression

*Specification Reference*: [TernaryExpression](statements/expressions/ternary_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Safe Guard with Null Check [NOT IMPLEMENTED]
```solix
String s = null;
int32 len = (s != null) ? s.length() : 0; // Short-circuits; does not call s.length()!
```
*Expected Result*: `len` equals 0; no null pointer exception occurs.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Non-Boolean Condition [NOT IMPLEMENTED]
```solix
int32 res = 5 ? 1 : 2; // Error
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Ternary condition must be of type 'bool', got 'int32'
```

---

## UnaryExpression

*Specification Reference*: [UnaryExpression](statements/expressions/unary_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Postfix vs Prefix [NOT IMPLEMENTED]
```solix
int32 x = 5;
int32 post = x++; // post = 5, x = 6
int32 pre = ++x;  // pre = 7, x = 7
```
*Expected Result*: `post` is 5, `pre` is 7.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Increment on Constant [NOT IMPLEMENTED]
```solix
++10; // Error
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Invalid operand for increment operator: expected lvalue
```

---
