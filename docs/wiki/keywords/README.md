# Solix Keyword Reference

This reference documents every reserved keyword in the Solix programming language. Keywords are identifiers that have special syntactic and semantic meaning to the compiler and may not be used as user-defined names. Each entry links to a dedicated page describing the keyword's grammar, semantics, compiler rules, code examples, and common pitfalls.

Solix keywords are divided into four categories:

- **Control Flow** — keywords that alter the execution path of a program
- **Declarations** — keywords that introduce new named entities (types, packages, imports)
- **Modifiers** — keywords that qualify declarations with visibility, storage, or behaviour annotations
- **Expressions** — keywords that appear within expressions to construct, test, or reference values

---

## Full Alphabetical Index

| Keyword | Category | Brief Description |
|---------|----------|-------------------|
| [`abstract`](modifiers/abstract.md) | Modifier | Marks a class or method as abstract; cannot be instantiated directly |
| [`alias`](declarations/alias.md) | Declaration | Introduces a type alias for an existing type |
| [`break`](control_flow/break.md) | Control Flow | Exits the nearest enclosing loop or `switch` statement |
| [`case`](control_flow/case.md) | Control Flow | Labels a branch inside a `switch` statement |
| [`catch`](control_flow/catch.md) | Control Flow | Catches an exception thrown within a preceding `try` block |
| [`class`](declarations/class.md) | Declaration | Declares a reference type with fields, methods, and optional inheritance |
| [`const`](modifiers/const.md) | Modifier | Declares a compile-time constant value |
| [`continue`](control_flow/continue.md) | Control Flow | Skips the rest of the current loop iteration and re-evaluates the loop condition |
| [`default`](control_flow/default.md) | Control Flow | Provides the fallback branch in a `switch` statement |
| [`do`](control_flow/do.md) | Control Flow | Begins a post-condition loop whose body executes at least once |
| [`else`](control_flow/else.md) | Control Flow | Provides an alternative branch when an `if` condition is false |
| [`enum`](declarations/enum.md) | Declaration | Declares an enumeration type with named constant values |
| [`extends`](expressions/extends.md) | Expression | Specifies that a class inherits from a superclass or an interface extends another interface |
| [`finally`](control_flow/finally.md) | Control Flow | Defines a block that runs unconditionally after a `try`/`catch` sequence |
| [`for`](control_flow/for.md) | Control Flow | Classical three-part counted loop (`init; condition; step`) |
| [`if`](control_flow/if.md) | Control Flow | Conditionally executes a block when an expression evaluates to `true` |
| [`implements`](expressions/implements.md) | Expression | Declares that a class conforms to one or more interfaces |
| [`import`](declarations/import.md) | Declaration | Brings an external package, class, or wildcard into the current compilation unit |
| [`inline`](modifiers/inline.md) | Modifier | Hints to the compiler that a method body should be inlined at call sites |
| [`instanceof`](expressions/instanceof.md) | Expression | Tests whether an object is an instance of a particular type at runtime |
| [`interface`](declarations/interface.md) | Declaration | Declares a purely abstract contract that classes may implement |
| [`internal`](modifiers/internal.md) | Modifier | Restricts visibility to the current package |
| [`native`](modifiers/native.md) | Modifier | Marks a method whose implementation is provided by native/platform code |
| [`new`](expressions/new.md) | Expression | Allocates and initialises a new heap object |
| [`operator`](expressions/operator.md) | Expression | Declares an operator overload method for a class |
| [`override`](modifiers/override.md) | Modifier | Asserts that a method replaces a virtual method from a superclass |
| [`package`](declarations/package.md) | Declaration | Declares the package namespace for the current source file |
| [`private`](modifiers/private.md) | Modifier | Restricts visibility to the declaring class only |
| [`protected`](modifiers/protected.md) | Modifier | Restricts visibility to the declaring class and its subclasses |
| [`public`](modifiers/public.md) | Modifier | Makes a declaration accessible from any compilation unit |
| [`return`](control_flow/return.md) | Control Flow | Exits the current method and optionally returns a value to the caller |
| [`sizeof`](expressions/sizeof.md) | Expression | Returns the byte size of a primitive type, class instance, or heap object |
| [`static`](modifiers/static.md) | Modifier | Associates a member with the class itself rather than with instances |
| [`super`](expressions/super.md) | Expression | Refers to the immediate superclass; used to call parent constructors or methods |
| [`switch`](control_flow/switch.md) | Control Flow | Multi-way branch that dispatches on the value of an expression |
| [`this`](expressions/this.md) | Expression | Refers to the current object instance within an instance method or constructor |
| [`throw`](control_flow/throw.md) | Control Flow | Raises an exception and transfers control to the nearest matching `catch` block |
| [`try`](control_flow/try.md) | Control Flow | Begins a guarded block where exceptions are caught |
| [`virtual`](modifiers/virtual.md) | Modifier | Marks a method as dynamically dispatched and overridable in subclasses |
| [`weak`](modifiers/weak.md) | Modifier | Declares a non-owning reference for breaking ARC retain cycles |
| [`while`](control_flow/while.md) | Control Flow | Repeats a block as long as a condition remains `true` |

---

## Category Navigation

- 📁 [Control Flow](control_flow/) — `if`, `else`, `while`, `do`, `for`, `switch`, `case`, `default`, `break`, `continue`, `return`, `try`, `catch`, `finally`, `throw`
- 📁 [Declarations](declarations/) — `class`, `interface`, `enum`, `package`, `import`, `alias`
- 📁 [Modifiers](modifiers/) — `public`, `private`, `protected`, `internal`, `static`, `inline`, `native`, `const`, `virtual`, `override`, `weak`, `abstract`
- 📁 [Expressions](expressions/) — `new`, `super`, `this`, `instanceof`, `sizeof`, `operator`, `extends`, `implements`
