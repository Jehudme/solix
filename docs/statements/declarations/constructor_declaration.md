# ConstructorDeclaration (`NodeType::CONSTRUCTOR_DECL`)

## 1. Description & Purpose

The `constructor` declaration defines the initialization routine invoked immediately after an object instance is allocated in heap memory. Constructors share the exact name of their enclosing class and support parameter overloading, member initializer lists (`: super(args), field(val)`), and superclass constructor chaining. If a class defines no constructors, the compiler automatically synthesizes a default zero-parameter constructor.

## 2. Syntax & Grammar

```solix
[access-modifier] <class-name> '(' <parameters...> ')' [':' <init-item> (',' <init-item>)*] <block>
// where <init-item> is either 'super' '(' <args...> ')' or <field-name> '(' <expr> ')'
```

## 3. Underlying Systems & Mechanics

- Sets up activation frame with `this` at register 0.
- If `: super(...)` is present, compiles arguments and emits `INVOKE_DIRECT` to the base constructor.
- Evaluates member initializer items (`: field_name(expr)`), compiling them as direct assignments (`this.field_name = expr;`) in the constructor preamble.
- Injects field default initializers into the bytecode stream directly following the super call and member initializer list.
- Executes user constructor body.
- Returns `this` reference.

## 4. Positive Test Scenarios (Valid Variations)

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

## 5. Negative Test Scenarios (Invalid Variations)

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
