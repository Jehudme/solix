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

### Case 3.3: Class and Qualified Type Alias [NOT IMPLEMENTED]
```solix
alias Text = solix.core.String;

void test() {
    Text t = new Text("hello");
}
```
*Expected Result*: `Text` resolves cleanly to `solix.core.String`.

### Case 3.4: Alias in Method Signature [NOT IMPLEMENTED]
```solix
alias ID = int64;

class User {
    ID id;
    ID getId() { return this.id; }
}
```
*Expected Result*: Compiles and binds method parameter and return types through alias.

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

### Case 4.3: Aliasing Undeclared Type [NOT IMPLEMENTED]
```solix
alias Bad = NonExistentType;
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot resolve alias target 'NonExistentType'
```

### Case 4.4: Duplicate Alias in Same Scope [NOT IMPLEMENTED]
```solix
alias Value = int32;
alias Value = float64;
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Duplicate declaration of alias 'Value'
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

### Case 3.3: Hierarchical Multi-Level Wildcard Import [NOT IMPLEMENTED]
```solix
import solix.collections.*;

void test() {
    List<int32> list = new List<int32>();
}
```
*Expected Result*: Wildcard import exposes all public types from nested package.

### Case 3.4: Qualified Access with Active Import [NOT IMPLEMENTED]
```solix
import solix.core.String;

void test() {
    solix.core.String s1 = new solix.core.String("qualified");
    String s2 = new String("unqualified");
}
```
*Expected Result*: Both fully qualified and imported unqualified names resolve to same type.

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

### Case 4.3: Misplaced Import Statement [NOT IMPLEMENTED]
```solix
class Foo {}
import solix.core.String; // Error: imports must precede declarations
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Import statements must appear before class declarations
```

### Case 4.4: Importing Non-Existent Member from Existing Package [NOT IMPLEMENTED]
```solix
import solix.core.FakeSymbol;
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Symbol 'FakeSymbol' not found in package 'solix.core'
```

### Case 4.5: Import Statement Inside Class Body [NOT IMPLEMENTED]
```solix
class Foo {
    import solix.core.String;
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Import statements must appear before class declarations
```

### Case 4.6: Import Statement Inside Function Body [NOT IMPLEMENTED]
```solix
void test() {
    import solix.core.String;
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Import statements must appear before class declarations
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

### Case 3.3: Package Isolation Across Unimported Namespaces [NOT IMPLEMENTED]
```solix
package alpha;
public class Secret {
    public int32 code;
}
```
*Expected Result*: Types declared in package `alpha` are isolated and require explicit import in package `beta`.

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

### Case 4.3: Invalid Package Identifier Syntax [NOT IMPLEMENTED]
```solix
package 123.invalid; // Error: numeric start in package segment
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Expected identifier in package statement, got number
```

### Case 4.4: Package Statement Inside Class Body [NOT IMPLEMENTED]
```solix
class Foo {
    package invalid.placement;
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: 'package' statement must be the first statement in the file
```

### Case 4.5: Package Statement Inside Function Body [NOT IMPLEMENTED]
```solix
void test() {
    package invalid.placement;
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: 'package' statement must be the first statement in the file
```

---

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

### Case 3.2: Multi-Level Inheritance Chain [NOT IMPLEMENTED]
```solix
class GrandParent {
    public int32 a;
}
class Parent extends GrandParent {
    public int32 b;
}
class Child extends Parent {
    public int32 c;
}

static int32 main() {
    Child obj = new Child();
    obj.a = 1;
    obj.b = 2;
    obj.c = 3;
    return (obj.a + obj.b + obj.c) == 6 ? 0 : 1;
}
```
*Expected Result*: Derived class inherits all ancestor fields across multi-tier hierarchy.

### Case 3.3: Abstract Class Extension and Implementation [NOT IMPLEMENTED]
```solix
abstract class Shape {
    public abstract int32 getArea();
}

class Square extends Shape {
    public int32 side;
    public Square(int32 s) { this.side = s; }
    public override int32 getArea() { return this.side * this.side; }
}

static int32 main() {
    Shape s = new Square(5);
    return s.getArea() == 25 ? 0 : 1;
}
```
*Expected Result*: Concrete class implements abstract methods and allows polymorphic dispatch.

### Case 3.4: Class Access Modifiers (Public vs Internal) [NOT IMPLEMENTED]
```solix
public class ExportedService {
    public int32 serve() { return 100; }
}

internal class InternalHelper {
    public int32 help() { return 200; }
}
```
*Expected Result*: `public` class exported outside package; `internal` class accessible within compilation unit.

---

### Case 3.6: Nested Class Declaration Inside Class [NOT IMPLEMENTED]
```solix
class Outer {
    public int32 outer_val;

    public class Inner {
        public int32 inner_val;
        public Inner(int32 v) { this.inner_val = v; }
    }
}

static int32 main() {
    Outer.Inner obj = new Outer.Inner(42);
    return obj.inner_val == 42 ? 0 : 1;
}
```
*Expected Result*: Nested class compiles and can be instantiated via qualified outer class name.

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

### Case 4.3: Multiple Class Inheritance Disallowed [NOT IMPLEMENTED]
```solix
class A {}
class B {}
class C extends A, B {} // Error: Solix enforces single inheritance
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Expected '{' after class inheritance clause
```

### Case 4.4: Extending Non-Class or Primitive Type [NOT IMPLEMENTED]
```solix
class Invalid extends int32 {} // Error: cannot extend primitive
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot extend non-class type 'int32'
```

### Case 4.5: Instantiating Abstract Class Directly [NOT IMPLEMENTED]
```solix
abstract class AbstractBase {}

void test() {
    AbstractBase a = new AbstractBase();
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot instantiate abstract class 'AbstractBase'
```

### Case 4.6: Class Declared Inside Function Body [NOT IMPLEMENTED]
```solix
void test() {
    class LocalClass {
        int32 x;
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Classes cannot be declared inside a function or method body
```

### Case 4.7: Class Declared Inside Control Flow Block [NOT IMPLEMENTED]
```solix
void test(bool cond) {
    if (cond) {
        class BlockClass {}
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Classes cannot be declared inside a block or control flow statement
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

### Case 3.2: Overloaded Constructors with Varying Arity [NOT IMPLEMENTED]
```solix
class Point {
    public int32 x;
    public int32 y;
    public Point() { this.x = 0; this.y = 0; }
    public Point(int32 x, int32 y) { this.x = x; this.y = y; }
}

static int32 main() {
    Point p1 = new Point();
    Point p2 = new Point(10, 20);
    return (p1.x == 0 && p2.x == 10 && p2.y == 20) ? 0 : 1;
}
```
*Expected Result*: Overloaded constructors resolve correctly by argument count and types.

### Case 3.3: Protected Constructor for Subclass Construction [NOT IMPLEMENTED]
```solix
class BaseAuth {
    protected BaseAuth() {}
}
class UserAuth extends BaseAuth {
    public UserAuth() { super(); }
}
```
*Expected Result*: Protected constructor accessible from subclass constructor.

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

### Case 4.2: Constructor with Explicit Return Type [NOT IMPLEMENTED]
```solix
class Widget {
    void Widget() {} // Error: constructors cannot declare return type
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Constructors must not specify a return type
```

### Case 4.3: Invoking Super Constructor Out of Order [NOT IMPLEMENTED]
```solix
class Base {}
class Sub extends Base {
    public int32 val;
    public Sub() {
        this.val = 42;
        super(); // Error: super() must be first statement
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Call to 'super()' must be the first statement in constructor
```

### Case 4.4: Constructor Declared Outside Any Class at Top Level [NOT IMPLEMENTED]
```solix
StandaloneConstructor() {
    // Error: constructor outside class
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Constructors can only be declared inside a class body
```

### Case 4.5: Constructor Declared Inside Method Body [NOT IMPLEMENTED]
```solix
void test() {
    MyClass() {} // Error: constructor inside method
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Constructors can only be declared inside a class body
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

### Case 3.2: Enum as Method Parameter and Return Value [NOT IMPLEMENTED]
```solix
enum State { PENDING, ACTIVE, CLOSED }

class Task {
    public State state;
    public void setState(State s) { this.state = s; }
    public State getState() { return this.state; }
}

static int32 main() {
    Task t = new Task();
    t.setState(State.ACTIVE);
    return t.getState() == State.ACTIVE ? 0 : 1;
}
```
*Expected Result*: Enum values passed and returned cleanly with full type safety.

---

### Case 3.4: Nested Enum Declaration Inside Class [NOT IMPLEMENTED]
```solix
class Window {
    public enum State { MINIMIZED, MAXIMIZED, NORMAL }
    public State state;
}

static int32 main() {
    Window w = new Window();
    w.state = Window.State.MAXIMIZED;
    return w.state == Window.State.MAXIMIZED ? 0 : 1;
}
```
*Expected Result*: Nested enum inside class compiles cleanly and qualifies with enclosing class.

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

### Case 4.2: Implicit Integer Assignment to Enum [NOT IMPLEMENTED]
```solix
enum Status { OK, FAIL }

void test() {
    Status s = 1; // Error: cannot implicitly convert int32 to Status
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot convert type 'int32' to enum 'Status'
```

### Case 4.3: Comparing Incompatible Enum Types [NOT IMPLEMENTED]
```solix
enum Fruit { APPLE, ORANGE }
enum Animal { CAT, DOG }

void test() {
    bool b = (Fruit.APPLE == Animal.CAT); // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Operator '==' cannot be applied to incompatible enums 'Fruit' and 'Animal'
```

### Case 4.4: Enum Declared Inside Function Body [NOT IMPLEMENTED]
```solix
void test() {
    enum LocalState { ON, OFF } // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Enums cannot be declared inside a function or method body
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

### Case 3.2: Field Access Modifiers (Public, Private, Protected) [NOT IMPLEMENTED]
```solix
class Account {
    public int32 id;
    private int32 secret;
    protected int32 balance;

    public Account(int32 id, int32 sec, int32 bal) {
        this.id = id;
        this.secret = sec;
        this.balance = bal;
    }
    public int32 getSecret() { return this.secret; }
}
```
*Expected Result*: Enforces lexical visibility while permitting authorized access within class scope.

### Case 3.3: Static Class Fields Shared Across Instances [NOT IMPLEMENTED]
```solix
class Counter {
    public static int32 count;
}

static int32 main() {
    Counter.count = 10;
    Counter c = new Counter();
    Counter.count++;
    return Counter.count == 11 ? 0 : 1;
}
```
*Expected Result*: Static field retains singular memory index shared globally.

### Case 3.4: Constant Field Declaration [NOT IMPLEMENTED]
```solix
class Config {
    public const int32 MAX_USERS = 500;
}

static int32 main() {
    return Config.MAX_USERS == 500 ? 0 : 1;
}
```
*Expected Result*: Const field inlines or preserves immutable compile-time value.

### Case 3.5: In-Class Field Initialization with String Literal [NOT IMPLEMENTED]
```solix
alias String = solix.core.String;

class Entity {
    private String label = "DefaultLabel";

    public String getLabel() {
        return this.label;
    }
}

static int32 main() {
    Entity e = new Entity();
    return e.getLabel().equals(new String("DefaultLabel")) ? 0 : 1;
}
```
*Expected Result*: String literal `"DefaultLabel"` is implicitly converted or wrapped into a `solix.core.String` instance.

### Case 3.6: Static Field Initialization with Object Instantiation [IMPLEMENTED]
```solix
alias String = solix.core.String;

class Config {
    public static String tag = new String("production");

    public static String getTag() {
        return tag;
    }
}

static int32 main() {
    return Config.getTag().equals(new String("production")) ? 0 : 1;
}
```
*Expected Result*: Static field `tag` initialized with `new String("production")` evaluates cleanly during program initialization.

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

### Case 4.3: Accessing Private Field Outside Class [NOT IMPLEMENTED]
```solix
class Vault {
    private int32 passcode;
}

void test() {
    Vault v = new Vault();
    int32 x = v.passcode; // Error: private field inaccessible
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot access private member 'passcode' of class 'Vault'
```

### Case 4.4: Modifying Const Field [NOT IMPLEMENTED]
```solix
class Constants {
    public const int32 RATE = 5;
}

void test() {
    Constants.RATE = 10; // Error: cannot assign to const field
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot assign to read-only constant field 'RATE'
```

### Case 4.5: Incompatible Field Initializer Type [NOT IMPLEMENTED]
```solix
class Model {
    public int32 count = "invalid"; // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Incompatible initializer for field 'count': expected 'int32', got 'String'
```

### Case 4.7: Field Access Modifier Applied to Local Variable [NOT IMPLEMENTED]
```solix
void test() {
    public int32 x = 10; // Error: access modifiers cannot be applied to local variables
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Access modifiers ('public', 'private', 'protected') are not allowed on local variables
```

### Case 4.8: Standalone Field Declared at File Top Level [NOT IMPLEMENTED]
```solix
public int32 globalField = 42; // Error: Solix requires fields to be in a class
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Variable declarations with access modifiers must be inside a class body
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

### Case 3.2: Interface Hierarchy with Sub-Interface Extension [NOT IMPLEMENTED]
```solix
interface Reader {
    int32 read();
}
interface AdvancedReader extends Reader {
    void reset();
}
```
*Expected Result*: `AdvancedReader` inherits abstract method requirement `read()`.

### Case 3.3: Polymorphic Method Invocation via Interface Variable [NOT IMPLEMENTED]
```solix
interface Worker {
    int32 work();
}
class Robot implements Worker {
    public int32 work() { return 99; }
}

static int32 main() {
    Worker w = new Robot();
    return w.work() == 99 ? 0 : 1;
}
```
*Expected Result*: Calls `work()` through interface vtable dispatch.

---

### Case 3.4: Nested Interface Declaration Inside Class [NOT IMPLEMENTED]
```solix
class Button {
    public interface OnClickListener {
        void onClick();
    }
}
```
*Expected Result*: Interface declared as a static member of a class compiles cleanly.

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

### Case 4.2: Class Incompletely Implementing Interface [NOT IMPLEMENTED]
```solix
interface Service {
    void start();
    void stop();
}
class IncompleteService implements Service {
    public void start() {} // Error: missing stop()
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Class 'IncompleteService' does not implement interface method 'stop()'
```

### Case 4.3: Interface Containing State Fields [NOT IMPLEMENTED]
```solix
interface BadInterface {
    int32 stateField; // Error: interfaces cannot contain instance state
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Interfaces must not declare instance fields
```

### Case 4.5: Interface Declared Inside Function Body [NOT IMPLEMENTED]
```solix
void test() {
    interface LocalInterface {} // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Interfaces cannot be declared inside a function or method body
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

### Case 3.2: Static Utility Method Invocation [NOT IMPLEMENTED]
```solix
class MathUtil {
    public static int32 add(int32 a, int32 b) {
        return a + b;
    }
}

static int32 main() {
    return MathUtil.add(20, 22) == 42 ? 0 : 1;
}
```
*Expected Result*: Static method invoked directly without class instance allocation.

### Case 3.3: Method Overloading with Multiple Types [NOT IMPLEMENTED]
```solix
class Calculator {
    public int32 compute(int32 x) { return x * 2; }
    public float64 compute(float64 x) { return x * 2.0; }
}

static int32 main() {
    Calculator c = new Calculator();
    return (c.compute(10) == 20 && c.compute(1.5) == 3.0) ? 0 : 1;
}
```
*Expected Result*: Dispatches to correct overload based on argument types.

### Case 3.4: Protected Method Accessible in Subclass [NOT IMPLEMENTED]
```solix
class BaseWorker {
    protected int32 getCode() { return 100; }
}
class DerivedWorker extends BaseWorker {
    public int32 test() { return this.getCode(); }
}
```
*Expected Result*: Subclass accesses protected superclass method without error.

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

### Case 4.2: Calling Private Method from Outside Class [NOT IMPLEMENTED]
```solix
class Encapsulated {
    private void secret() {}
}

void test() {
    Encapsulated e = new Encapsulated();
    e.secret(); // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot access private method 'secret' of class 'Encapsulated'
```

### Case 4.3: Missing Return Statement in Non-Void Method [NOT IMPLEMENTED]
```solix
int32 badMethod(bool flag) {
    if (flag) {
        return 1;
    }
    // Error: missing return path
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Not all control paths return a value in function 'badMethod'
```

### Case 4.4: Incompatible Override Signature Return Type [NOT IMPLEMENTED]
```solix
class SuperClass {
    public virtual int32 getValue() { return 0; }
}
class SubClass extends SuperClass {
    public override String getValue() { return "bad"; } // Error: return type mismatch
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Overriding method 'getValue' has incompatible return type 'String' (expected 'int32')
```

### Case 4.7: Nested Method Declaration Inside Another Method [NOT IMPLEMENTED]
```solix
void outer() {
    void inner() {} // Error: nested functions are not supported
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Methods cannot be declared inside another method
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

### Case 3.2: Overloading Subtraction and Equality Operators [NOT IMPLEMENTED]
```solix
class Complex {
    public int32 re;
    public int32 im;
    public Complex(int32 r, int32 i) { this.re = r; this.im = i; }

    public Complex operator-(Complex rhs) {
        return new Complex(this.re - rhs.re, this.im - rhs.im);
    }
    public bool operator==(Complex rhs) {
        return this.re == rhs.re && this.im == rhs.im;
    }
}

static int32 main() {
    Complex c1 = new Complex(10, 5);
    Complex c2 = new Complex(4, 2);
    Complex res = c1 - c2;
    return (res == new Complex(6, 3)) ? 0 : 1;
}
```
*Expected Result*: Binary `-` and `==` operators overload seamlessly.

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

### Case 4.2: Binary Operator Declared with Wrong Arity [NOT IMPLEMENTED]
```solix
class Vector {
    public Vector operator+(Vector a, Vector b) {} // Error: member operator+ takes 1 argument
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Member binary operator '+' must take exactly 1 argument
```

### Case 4.4: Operator Overload Declared at Top Level [NOT IMPLEMENTED]
```solix
public int32 operator+(int32 a, int32 b) {
    return a + b;
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Operator overloads can only be declared inside a class body
```

### Case 4.5: Operator Overload Declared Inside Method Body [NOT IMPLEMENTED]
```solix
void test() {
    operator+(int32 a) {}
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Operator overloads can only be declared inside a class body
```

---

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

### Case 4.6: Bare Execution Block Directly in Class Body [NOT IMPLEMENTED]
```solix
class BadClass {
    {
        int32 x = 5; // Error: executable block outside method
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Executable blocks are not allowed directly in class body
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

### Case 3.2: Break in For Loop Preserves State [NOT IMPLEMENTED]
```solix
static int32 main() {
    int32 sum = 0;
    for (int32 i = 0; i < 10; i++) {
        if (i == 5) {
            break;
        }
        sum += i;
    }
    return sum == 10 ? 0 : 1;
}
```
*Expected Result*: Loop terminates when `i == 5`, producing sum 0+1+2+3+4 = 10.

### Case 3.3: Break Inside Switch Statement [NOT IMPLEMENTED]
```solix
static int32 main() {
    int32 x = 2;
    int32 res = 0;
    switch (x) {
        case 1: res = 10; break;
        case 2: res = 20; break;
        default: res = 30; break;
    }
    return res == 20 ? 0 : 1;
}
```
*Expected Result*: Break transfers control out of switch construct cleanly.

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

### Case 4.2: Break at Function Top Level [NOT IMPLEMENTED]
```solix
void test() {
    break; // Error: break not enclosed in loop or switch
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: 'break' statement outside of loop or switch
```

### Case 4.3: Break Statement Placed Directly in Class Body [NOT IMPLEMENTED]
```solix
class BadClass {
    break; // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Statements are not allowed directly in class body
```

### Case 4.4: Break Inside If Not Enclosed in Loop or Switch [NOT IMPLEMENTED]
```solix
void test(bool flag) {
    if (flag) {
        break; // Error: no enclosing loop
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: 'break' statement outside of loop or switch
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

### Case 3.2: Continue in While Loop [NOT IMPLEMENTED]
```solix
static int32 main() {
    int32 i = 0;
    int32 count = 0;
    while (i < 10) {
        i++;
        if (i % 2 == 0) {
            continue;
        }
        count++;
    }
    return count == 5 ? 0 : 1;
}
```
*Expected Result*: Bypasses odd body executions, executing exactly 5 times.

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

### Case 4.2: Continue Inside Switch Not in Loop [NOT IMPLEMENTED]
```solix
void test(int32 x) {
    switch (x) {
        case 1:
            continue; // Error: continue cannot target switch
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: 'continue' statement outside of loop
```

### Case 4.3: Continue Statement Placed Directly in Class Body [NOT IMPLEMENTED]
```solix
class BadClass {
    continue; // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Statements are not allowed directly in class body
```

### Case 4.4: Continue Inside If Not Enclosed in Loop [NOT IMPLEMENTED]
```solix
void test(bool flag) {
    if (flag) {
        continue; // Error: no enclosing loop
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: 'continue' statement outside of loop
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

### Case 3.2: Multi-Pass Iteration and Condition Evaluation [NOT IMPLEMENTED]
```solix
static int32 main() {
    int32 sum = 0;
    int32 i = 1;
    do {
        sum += i;
        i++;
    } while (i <= 5);
    return sum == 15 ? 0 : 1;
}
```
*Expected Result*: Executes body 5 times and exits with sum 15.

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

### Case 4.2: Non-Boolean Condition in Do-While [NOT IMPLEMENTED]
```solix
void test() {
    do {} while (42); // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Do-while loop condition must be of type 'bool', got 'int32'
```

### Case 4.3: Do-While Statement Placed Directly in Class Body [NOT IMPLEMENTED]
```solix
class BadClass {
    do {} while (true); // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Statements are not allowed directly in class body
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

### Case 3.3: Chained Fluent Method Calls [NOT IMPLEMENTED]
```solix
class Builder {
    public int32 val;
    public Builder add(int32 x) { this.val += x; return this; }
}

static int32 main() {
    Builder b = new Builder();
    b.add(10).add(20).add(30);
    return b.val == 60 ? 0 : 1;
}
```
*Expected Result*: Chained calls execute sequentially as single expression statement.

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

### Case 4.3: Incomplete Expression Statement [NOT IMPLEMENTED]
```solix
void test() {
    int32 x = ; // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Expected expression, got ';'
```

### Case 4.4: Expression Statement Directly in Class Body [NOT IMPLEMENTED]
```solix
class BadClass {
    10 + 20; // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Statements are not allowed directly in class body
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

### Case 3.2: Empty Header Clauses for Infinite Loop with Break [NOT IMPLEMENTED]
```solix
static int32 main() {
    int32 count = 0;
    for (;;) {
        count++;
        if (count == 5) {
            break;
        }
    }
    return count == 5 ? 0 : 1;
}
```
*Expected Result*: `for (;;)` runs infinitely until terminated by internal `break`.

### Case 3.3: Nested For Loops for Matrix Summation [NOT IMPLEMENTED]
```solix
static int32 main() {
    int32 total = 0;
    for (int32 i = 0; i < 3; i++) {
        for (int32 j = 0; j < 3; j++) {
            total += 1;
        }
    }
    return total == 9 ? 0 : 1;
}
```
*Expected Result*: Inner and outer loop induction variables remain isolated, summing to 9.

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

### Case 4.2: Non-Boolean Condition in For Loop [NOT IMPLEMENTED]
```solix
void test() {
    for (int32 i = 0; 100; i++) {} // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Loop condition must be of type 'bool', got 'int32'
```

### Case 4.3: For Loop Placed Directly in Class Body [NOT IMPLEMENTED]
```solix
class BadClass {
    for (int32 i = 0; i < 10; i++) {} // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Statements are not allowed directly in class body
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

### Case 3.3: Complex Short-Circuit Logical Condition [NOT IMPLEMENTED]
```solix
static int32 main() {
    int32 a = 10;
    int32 b = 20;
    bool executed = false;
    if (a == 10 && (b == 20 || false)) {
        executed = true;
    }
    return executed ? 0 : 1;
}
```
*Expected Result*: Evaluates grouped boolean logic and enters true branch.

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

### Case 4.2: Incompatible Assignment in Condition [NOT IMPLEMENTED]
```solix
void test() {
    int32 x = 0;
    if (x = 5) {} // Error: assignment yields int32, not bool
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: If condition must be of type 'bool', got 'int32'
```

### Case 4.3: If Statement Placed Directly in Class Body [NOT IMPLEMENTED]
```solix
class BadClass {
    if (true) {} // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Statements are not allowed directly in class body
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

### Case 3.2: Return from Inside Try-Finally Executing Finally First [NOT IMPLEMENTED]
```solix
static int32 result = 0;

static int32 compute() {
    try {
        return 42;
    } finally {
        result = 100;
    }
}

static int32 main() {
    int32 val = compute();
    return (val == 42 && result == 100) ? 0 : 1;
}
```
*Expected Result*: Returns 42 while reliably executing `finally` block before stack unwind completes.

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

### Case 4.3: Returning Value from Void Method [NOT IMPLEMENTED]
```solix
void test() {
    return 42; // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot return a value from a void method
```

### Case 4.5: Return Statement Placed at File Top Level [NOT IMPLEMENTED]
```solix
return 0; // Error: return at top level outside any function
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: 'return' statement outside of function or method body
```

### Case 4.6: Return Statement Placed Directly in Class Body [NOT IMPLEMENTED]
```solix
class BadClass {
    return 0; // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Statements are not allowed directly in class body
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

### Case 3.2: Switch on Strongly Typed Enum [NOT IMPLEMENTED]
```solix
enum Status { PENDING, APPROVED, REJECTED }

static int32 evaluate(Status s) {
    switch (s) {
        case Status.PENDING: return 1;
        case Status.APPROVED: return 2;
        case Status.REJECTED: return 3;
        default: return 0;
    }
}

static int32 main() {
    return evaluate(Status.APPROVED) == 2 ? 0 : 1;
}
```
*Expected Result*: Evaluates enum constants in switch case selection.

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

### Case 4.2: Variable Expression in Case Label [NOT IMPLEMENTED]
```solix
void test(int32 x, int32 dynamicVal) {
    switch (x) {
        case dynamicVal: break; // Error: case label must be compile-time constant
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Case label must be a constant literal
```

### Case 4.4: Switch Statement Placed Directly in Class Body [NOT IMPLEMENTED]
```solix
class BadClass {
    switch (1) { case 1: break; } // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Statements are not allowed directly in class body
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

### Case 3.2: Rethrowing Caught Exception Instance [NOT IMPLEMENTED]
```solix
static int32 main() {
    bool caught_outer = false;
    try {
        try {
            throw new Exception("inner");
        } catch (Exception e) {
            throw e; // Rethrow
        }
    } catch (Exception outer) {
        caught_outer = true;
    }
    return caught_outer ? 0 : 1;
}
```
*Expected Result*: Rethrown exception safely propagates to enclosing catch handler.

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

### Case 4.3: Throwing Uninstantiated Class Identifier [NOT IMPLEMENTED]
```solix
void test() {
    throw Exception; // Error: expected instance, got type identifier
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot throw non-instantiated type 'Exception'
```

### Case 4.5: Throw Statement Placed Directly in Class Body [NOT IMPLEMENTED]
```solix
class BadClass {
    throw new Exception("bad"); // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Statements are not allowed directly in class body
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

### Case 3.2: Try Block with Only Finally Clause [NOT IMPLEMENTED]
```solix
static int32 cleanup_marker = 0;

static void work() {
    try {
        cleanup_marker += 10;
    } finally {
        cleanup_marker += 20;
    }
}

static int32 main() {
    work();
    return cleanup_marker == 30 ? 0 : 1;
}
```
*Expected Result*: Executes try followed by finally block without catch clause.

### Case 3.3: Exception in Catch with Guaranteed Finally Execution [NOT IMPLEMENTED]
```solix
static bool finally_ran = false;

static void faulty() {
    try {
        throw new Exception("one");
    } catch (Exception e) {
        throw new Exception("two");
    } finally {
        finally_ran = true;
    }
}

static int32 main() {
    try {
        faulty();
    } catch (Exception e) {}
    return finally_ran ? 0 : 1;
}
```
*Expected Result*: `finally` executes before the second exception escapes the frame.

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

### Case 4.2: Catching Non-Exception Type [NOT IMPLEMENTED]
```solix
void test() {
    try {} catch (int32 x) {} // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Catch type must derive from 'Exception', got 'int32'
```

### Case 4.3: Duplicate Catch Clause for Same Type [NOT IMPLEMENTED]
```solix
void test() {
    try {}
    catch (Exception e) {}
    catch (Exception e2) {} // Error: duplicate catch
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Duplicate catch clause for type 'Exception'
```

### Case 4.5: Try-Catch Statement Placed Directly in Class Body [NOT IMPLEMENTED]
```solix
class BadClass {
    try {} catch (Exception e) {} // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Statements are not allowed directly in class body
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

### Case 3.4: Multiple Declarations on Single Line [NOT IMPLEMENTED]
```solix
static int32 main() {
    int32 a = 1, b = 2, c = 3;
    return (a + b + c) == 6 ? 0 : 1;
}
```
*Expected Result*: Emits sequential local allocations for comma-separated variables.

### Case 3.5: Constant Local Variable Declaration [NOT IMPLEMENTED]
```solix
static int32 main() {
    const int32 LIMIT = 100;
    return LIMIT == 100 ? 0 : 1;
}
```
*Expected Result*: Marks local variable immutable in scope frame.

### Case 3.6: File-Scope Global Variable Declaration [IMPLEMENTED]
```solix
int32 globalCounter = 45;

public class Main {
    public static int32 main() {
        return globalCounter == 45 ? 0 : 1;
    }
}
```
*Expected Result*: Top-level file-scope variable `globalCounter` is allocated as a static global and accessible across functions and classes.

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

### Case 4.5: Reassigning Const Local Variable [NOT IMPLEMENTED]
```solix
void test() {
    const int32 x = 10;
    x = 20; // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot assign to const variable 'x'
```

### Case 4.6: Assigning Null to Primitive Type [NOT IMPLEMENTED]
```solix
void test() {
    int32 x = null; // Error: primitive types are non-nullable
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot assign 'null' to primitive type 'int32'
```

### Case 4.7: Solitary Variable Declaration in If Without Braces [NOT IMPLEMENTED]
```solix
void test() {
    if (true)
        int32 x = 5; // Error: variable declaration cannot be solitary statement of branch
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Variable declarations are not allowed as immediate solitary branch statements without a block
```

### Case 4.8: Solitary Variable Declaration in While Without Braces [NOT IMPLEMENTED]
```solix
void test() {
    while (true)
        int32 x = 5; // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Variable declarations are not allowed as immediate solitary loop statements without a block
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

### Case 3.3: While Loop with Complex Short-Circuit Condition [NOT IMPLEMENTED]
```solix
static int32 main() {
    int32 count = 0;
    int32 limit = 5;
    while (count < 10 && count < limit) {
        count++;
    }
    return count == 5 ? 0 : 1;
}
```
*Expected Result*: Terminates once compound condition evaluates to false.

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

### Case 4.2: While Loop Missing Condition Parentheses [NOT IMPLEMENTED]
```solix
void test() {
    while true {} // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Expected '(' after 'while'
```

### Case 4.3: While Loop Placed Directly in Class Body [NOT IMPLEMENTED]
```solix
class BadClass {
    while (true) {} // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Statements are not allowed directly in class body
```

---

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

### Case 3.2: Multi-Dimensional Array Access [NOT IMPLEMENTED]
```solix
static int32 main() {
    int32[][] grid = new int32[][](2);
    grid[0] = new int32[](2);
    grid[0][1] = 42;
    return grid[0][1] == 42 ? 0 : 1;
}
```
*Expected Result*: Successfully reads and writes through nested array index chains.

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

### Case 4.2: Non-Integer Array Subscript [NOT IMPLEMENTED]
```solix
void test(int32[] arr) {
    int32 v = arr["key"]; // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Array index must be int32, got 'String'
```

### Case 4.4: Subscript Access on Null Reference at Runtime [NOT IMPLEMENTED]
```solix
static int32 main() {
    int32[] arr = null;
    int32 v = arr[0]; // Runtime Fault
    return 0;
}
```
*Expected Result*: Throws `NullPointerException`.

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

### Case 3.2: Reference Type Array Allocation [NOT IMPLEMENTED]
```solix
class Item {
    public int32 id;
}

static int32 main() {
    Item[] items = new Item[](3);
    return items[0] == null ? 0 : 1;
}
```
*Expected Result*: Reference type array initializes elements to `null`.

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

### Case 4.2: Array Creation with Missing Size [NOT IMPLEMENTED]
```solix
void test() {
    int32[] arr = new int32[](); // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Array allocation requires size expression
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

### Case 3.2: Nested 2D Array Literal [NOT IMPLEMENTED]
```solix
static int32 main() {
    int32[][] matrix = {{1, 2}, {3, 4}};
    return (matrix[0][0] + matrix[1][1]) == 5 ? 0 : 1;
}
```
*Expected Result*: Correctly constructs multidimensional array literal.

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

### Case 4.2: Array Literal with Mixed Incompatible Types [NOT IMPLEMENTED]
```solix
void test() {
    var arr = {1, "two", true}; // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Incompatible element types in array literal
```

---

## AssignmentExpression

*Specification Reference*: [AssignmentExpression](statements/expressions/assignment_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Chained Assignment [IMPLEMENTED]
```solix
int32 a; int32 b; int32 c;
a = b = c = 10;
```
*Expected Result*: `a`, `b`, and `c` all equal 10.

---

### Case 3.2: Compound Assignments (+=, -=, *=, /=) [NOT IMPLEMENTED]
```solix
static int32 main() {
    int32 x = 10;
    x += 5;  // 15
    x -= 3;  // 12
    x *= 2;  // 24
    x /= 4;  // 6
    return x == 6 ? 0 : 1;
}
```
*Expected Result*: In-place arithmetic compound assignment computes cleanly.

### Case 3.3: Assigning to Object Field Target [NOT IMPLEMENTED]
```solix
class Box {
    public int32 weight;
}

static int32 main() {
    Box b = new Box();
    b.weight = 50;
    return b.weight == 50 ? 0 : 1;
}
```
*Expected Result*: Correctly emits member setter bytecode sequence.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Assigning to Literal / RValue [IMPLEMENTED]
```solix
void test() {
    10 = x; // Error: invalid lvalue
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Invalid assignment target
```

### Case 4.2: Compound Assignment Type Mismatch [NOT IMPLEMENTED]
```solix
void test() {
    int32 x = 10;
    x += "text"; // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot apply operator '+=' to types 'int32' and 'String'
```

---

## BinaryExpression

*Specification Reference*: [BinaryExpression](statements/expressions/binary_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Short-Circuit Logical AND [IMPLEMENTED]
```solix
bool result = false && (10 / 0 == 0); // Division by zero avoided
```
*Expected Result*: `result` is `false`.

---

### Case 3.2: Short-Circuit Logical OR [NOT IMPLEMENTED]
```solix
static int32 side_effects = 0;
static bool get_false() { side_effects++; return false; }
static bool get_true() { side_effects++; return true; }

static int32 main() {
    bool res = get_true() || get_false();
    return (res && side_effects == 1) ? 0 : 1;
}
```
*Expected Result*: Right-hand operand of `||` is never evaluated when left is true.

### Case 3.3: Relational Comparisons (<, <=, >, >=) [NOT IMPLEMENTED]
```solix
static int32 main() {
    int32 a = 10;
    int32 b = 20;
    if (a < b && a <= 10 && b > a && b >= 20) {
        return 0;
    }
    return 1;
}
```
*Expected Result*: All relational comparisons evaluate correctly.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Division by Zero (Runtime Fault) [IMPLEMENTED]
```solix
void test() {
    int32 x = 10 / 0;
}
```
*Expected Runtime Exception*:
```text
[FATAL VM PANIC] ArithmeticException: Division by zero
```

### Case 4.2: Incompatible Arithmetic Types [NOT IMPLEMENTED]
```solix
void test() {
    int32 x = 10 + true; // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot apply operator '+' to types 'int32' and 'bool'
```

---

## CastExpression

*Specification Reference*: [CastExpression](statements/expressions/cast_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Valid Downcast [IMPLEMENTED]
```solix
Animal a = new Dog();
Dog d = (Dog)a; // Succeeds
```
*Expected Result*: `d` holds a valid reference to `Dog`.

---

### Case 3.2: Numeric Widening and Narrowing Conversions [NOT IMPLEMENTED]
```solix
static int32 main() {
    int32 small = 42;
    int64 big = (int64)small;
    int8 tiny = (int8)small;
    return (big == 42 && tiny == 42) ? 0 : 1;
}
```
*Expected Result*: Widens and truncates integer values explicitly.

---

### Case 3.4: Casting Null Literal to Reference Type [NOT IMPLEMENTED]
```solix
class Person {}

static int32 main() {
    Person p = (Person)null;
    return p == null ? 0 : 1;
}
```
*Expected Result*: Casting `null` to a class reference type succeeds cleanly and preserves null.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Bad Downcast (Runtime Fault) [IMPLEMENTED]
```solix
Animal a = new Cat();
Dog d = (Dog)a; // Fails!
```
*Expected Runtime Exception*:
```text
[FATAL VM PANIC] TypeCastException: Cannot cast 'Cat' to 'Dog'
```

### Case 4.2: Compile-Time Rejection of Unrelated Class Cast [NOT IMPLEMENTED]
```solix
class Cat {}
class Dog {}

void test() {
    Cat c = new Cat();
    Dog d = (Dog)c; // Error: incompatible class hierarchies
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot cast between unrelated types 'Cat' and 'Dog'
```

### Case 4.4: Cast Expression with Non-Type Identifier [NOT IMPLEMENTED]
```solix
void test(int32 x) {
    int32 y = (123)x; // Error: 123 is not a type
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Expected type name in cast expression
```

---

## IdentifierNode

*Specification Reference*: [IdentifierNode](statements/expressions/identifier_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Local Resolution Precedence [IMPLEMENTED]
```solix
int32 val = 100;
{
    int32 val = 200;
    Console.println(val); // Resolves to local slot (200)
}
```
*Expected Result*: Prints 200.

---

### Case 3.2: Explicit Member Access via this Identifier [NOT IMPLEMENTED]
```solix
class ScopeTest {
    public int32 val;
    public void setVal(int32 val) {
        this.val = val; // Disambiguates parameter vs instance field
    }
}
```
*Expected Result*: Explicit `this` properly distinguishes field from local variable.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Undefined Identifier [IMPLEMENTED]
```solix
void test() {
    int32 a = unknown_var;
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Undefined identifier: unknown_var
```

### Case 4.2: Accessing Local Variable Before Declaration [NOT IMPLEMENTED]
```solix
void test() {
    x = 10; // Error: x not yet declared
    int32 x = 0;
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Undeclared identifier 'x'
```

### Case 4.3: Using This Keyword Outside Any Class [NOT IMPLEMENTED]
```solix
void test() {
    int32 x = this.val; // Error: 'this' outside class
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Keyword 'this' is only valid within non-static class member methods
```

### Case 4.4: Using This Keyword Inside Static Method [NOT IMPLEMENTED]
```solix
class Example {
    public static void staticMethod() {
        int32 x = this.val; // Error: 'this' in static context
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Keyword 'this' cannot be used in a static method
```

---

## InstanceOfExpression

*Specification Reference*: [InstanceOfExpression](statements/expressions/instanceof_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Safe Null Evaluation [IMPLEMENTED]
```solix
Animal a = null;
bool check = a instanceof Dog; // false, no panic!
```
*Expected Result*: `check` is `false`.

---

### Case 3.2: InstanceOf Subclass Evaluates True for Superclass [NOT IMPLEMENTED]
```solix
class Base {}
class Sub extends Base {}

static int32 main() {
    Base obj = new Sub();
    if (obj instanceof Base && obj instanceof Sub) {
        return 0;
    }
    return 1;
}
```
*Expected Result*: Instance of derived class returns true for both base and derived tests.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Primitive Target [IMPLEMENTED]
```solix
bool b = 10 instanceof int32; // Error
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: 'instanceof' cannot be applied to primitive types
```

### Case 4.2: InstanceOf with Undeclared Type Name [NOT IMPLEMENTED]
```solix
void test(Object o) {
    bool b = o instanceof NonExistentClass; // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot resolve type 'NonExistentClass' in instanceof expression
```

---

## LiteralNode

*Specification Reference*: [LiteralNode](statements/expressions/literal_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: All Literal Types [IMPLEMENTED]
```solix
int64 big = 10000000000L;
char letter = 'Z';
String empty = "";
```
*Expected Result*: All primitives compile and load accurately.

---

### Case 3.2: Hexadecimal and Binary Numeric Literals [NOT IMPLEMENTED]
```solix
static int32 main() {
    int32 hex = 0x2A;    // 42
    int32 bin = 0b101010; // 42
    return (hex == 42 && bin == 42) ? 0 : 1;
}
```
*Expected Result*: Hex (`0x`) and Binary (`0b`) integer prefixes parse correctly.

### Case 3.3: Character Literals with Escapes [NOT IMPLEMENTED]
```solix
static int32 main() {
    char newline = '
';
    char tab = '	';
    char quote = ''';
    return 0;
}
```
*Expected Result*: Character escape sequences produce valid 8-bit scalar values.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Integer Literal Overflow [IMPLEMENTED]
```solix
int32 x = 99999999999999999999;
```
*Expected Compiler Diagnostic*:
```text
[ERROR] lexer.cpp: Integer literal out of range for type 'int32'
```

### Case 4.2: Unterminated String Literal [NOT IMPLEMENTED]
```solix
void test() {
    String s = "unterminated;
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] lexer.cpp: Unterminated string literal
```

---

## MemberAccessExpression

*Specification Reference*: [MemberAccessExpression](statements/expressions/member_access_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Chained Member Access [IMPLEMENTED]
```solix
class Address { String city; }
class Person { Address addr; }

void test(Person p) {
    String city = p.addr.city;
}
```
*Expected Result*: Resolves offsets sequentially; compiles cleanly.

---

### Case 3.2: Static Field Access on Class Name [NOT IMPLEMENTED]
```solix
class MathConstants {
    public static int32 SCALE = 100;
}

static int32 main() {
    return MathConstants.SCALE == 100 ? 0 : 1;
}
```
*Expected Result*: Emits `GET_GLOBAL` targeting static class field.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Member Access on Null Reference (Runtime Fault) [IMPLEMENTED]
```solix
Person p = null;
String c = p.addr; // Throws NullReferenceException
```
*Expected Runtime Exception*:
```text
[FATAL VM PANIC] NullReferenceException: Attempted to read property from null object reference
```

### Case 4.2: Accessing Non-Existent Member Field [NOT IMPLEMENTED]
```solix
class Empty {}

void test() {
    Empty e = new Empty();
    int32 x = e.unknownField; // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Class 'Empty' has no member named 'unknownField'
```

### Case 4.4: Using Super Keyword in Non-Derived Class [NOT IMPLEMENTED]
```solix
class BaseOnly {
    public void test() {
        super.doSomething(); // Error: class does not extend another class
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Keyword 'super' cannot be used in a class with no superclass
```

---

## MethodCallExpression

*Specification Reference*: [MethodCallExpression](statements/expressions/method_call_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Overloaded Method Selection [IMPLEMENTED]
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

### Case 3.2: Calling Inherited Superclass Method [NOT IMPLEMENTED]
```solix
class BaseCalc {
    public int32 add(int32 a, int32 b) { return a + b; }
}
class AdvancedCalc extends BaseCalc {
    public int32 doubleAdd(int32 a, int32 b) {
        return this.add(a, b) * 2;
    }
}

static int32 main() {
    AdvancedCalc calc = new AdvancedCalc();
    return calc.doubleAdd(3, 4) == 14 ? 0 : 1;
}
```
*Expected Result*: Inherited methods dispatched without error.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: No Matching Overload [IMPLEMENTED]
```solix
void test(Printer p) {
    p.print(true); // Error: no boolean overload
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: No matching overload for method 'print' with arguments (bool)
```

### Case 4.2: Method Call with Incorrect Argument Count [NOT IMPLEMENTED]
```solix
class Calculator {
    public int32 compute(int32 a, int32 b) { return a + b; }
}

void test() {
    Calculator c = new Calculator();
    c.compute(10); // Error: expected 2 arguments, got 1
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: No matching overload for method 'compute' with arguments (int32)
```

---

## NewInstanceExpression

*Specification Reference*: [NewInstanceExpression](statements/expressions/new_instance_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Instantiation with Overloaded Constructor [IMPLEMENTED]
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

### Case 3.2: Instantiating Class with In-Class Field Initializers [NOT IMPLEMENTED]
```solix
class Config {
    public int32 timeout = 3000;
    public bool enabled = true;
}

static int32 main() {
    Config c = new Config();
    return (c.timeout == 3000 && c.enabled == true) ? 0 : 1;
}
```
*Expected Result*: Default field initializers execute upon instance creation.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Instantiating Abstract Class [IMPLEMENTED]
```solix
abstract class Base {}
Base b = new Base(); // Error
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot instantiate abstract class 'Base'
```

### Case 4.2: Calling Non-Existent Constructor Overload [NOT IMPLEMENTED]
```solix
class Box {
    public Box(int32 w) {}
}

void test() {
    Box b = new Box("bad"); // Error: no ctor taking String
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: No matching constructor: Box.ctor(String)
```

---

## TernaryExpression

*Specification Reference*: [TernaryExpression](statements/expressions/ternary_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Safe Guard with Null Check [IMPLEMENTED]
```solix
String s = null;
int32 len = (s != null) ? s.length() : 0; // Short-circuits; does not call s.length()!
```
*Expected Result*: `len` equals 0; no null pointer exception occurs.

---

### Case 3.2: Nested Ternary Evaluation [NOT IMPLEMENTED]
```solix
static int32 classify(int32 x) {
    return x > 0 ? 1 : (x < 0 ? -1 : 0);
}

static int32 main() {
    return (classify(10) == 1 && classify(-5) == -1 && classify(0) == 0) ? 0 : 1;
}
```
*Expected Result*: Nested ternary expressions evaluate in correct order.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Non-Boolean Condition [IMPLEMENTED]
```solix
int32 res = 5 ? 1 : 2; // Error
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Ternary condition must be of type 'bool', got 'int32'
```

### Case 4.2: Mismatched Branch Types [NOT IMPLEMENTED]
```solix
void test(bool cond) {
    int32 val = cond ? 42 : "string"; // Error: incompatible branch types
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Ternary branches must have the same type
```

---

## UnaryExpression

*Specification Reference*: [UnaryExpression](statements/expressions/unary_expression.md)

### Positive Test Scenarios (Valid Variations)

### Case 3.1: Postfix vs Prefix [IMPLEMENTED]
```solix
int32 x = 5;
int32 post = x++; // post = 5, x = 6
int32 pre = ++x;  // pre = 7, x = 7
```
*Expected Result*: `post` is 5, `pre` is 7.

---

### Case 3.2: Unary Negation on Numeric Expressions [NOT IMPLEMENTED]
```solix
static int32 main() {
    int32 x = 42;
    int32 neg = -x;
    return neg == -42 ? 0 : 1;
}
```
*Expected Result*: Evaluates `-x` to -42.

### Case 3.3: Logical NOT on Boolean Variable [NOT IMPLEMENTED]
```solix
static int32 main() {
    bool active = false;
    bool inverted = !active;
    return inverted == true ? 0 : 1;
}
```
*Expected Result*: Inverts boolean operand with `LOGICAL_NOT` opcode.

---

### Negative Test Scenarios (Expected Errors & Faults)

### Case 4.1: Increment on Constant [IMPLEMENTED]
```solix
++10; // Error
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Invalid operand for increment operator: expected lvalue
```

### Case 4.2: Unary Minus on Non-Numeric Operand [NOT IMPLEMENTED]
```solix
void test() {
    String s = -"text"; // Error
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot apply unary operator '-' to type 'String'
```

---

