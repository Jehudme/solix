# OperatorDeclaration (`NodeType::OPERATOR`)

## 1. Description & Purpose

The `operator` declaration enables operator overloading for user-defined classes, allowing customized behavior when instances are used with mathematical or assignment operators. Solix strictly regulates operator overloading, permitting only the canonical arithmetic operators (`+`, `-`, `*`, `/`) and assignment (`=`). Operator methods are compiled into specialized member functions and dispatched dynamically or statically depending on receiver context.

## 2. Syntax & Grammar

```solix
[access-modifier] <return-type> 'operator' <operator-symbol> '(' <parameter> ')' <block>
// where <operator-symbol> is one of: '+', '-', '*', '/', '='
```

## 3. Underlying Systems & Mechanics

- Compiles as a specialized member method with mangled name `operator<op>(param_type)`.
- Specifically supported operators: `+`, `-`, `*`, `/`, and `=`.
- When the parser encounters binary operators (`a + b`, `a = b`) where `a` is a class type, the binder transforms the binary expression into a method call on `a` passing `b`.
- ARC retains return values from operators properly.

## 4. Positive Test Scenarios (Valid Variations)

1. **Addition Overload**: `public Vector2 operator+(Vector2 other) { return new Vector2(this.x + other.x, this.y + other.y); }`
2. **Subtraction Overload**: `public Vector2 operator-(Vector2 other) { return new Vector2(this.x - other.x, this.y - other.y); }`
3. **Multiplication Overload**: `public Vector2 operator*(float64 scalar) { return new Vector2(this.x * scalar, this.y * scalar); }`
4. **Division Overload**: `public Vector2 operator/(float64 scalar) { return new Vector2(this.x / scalar, this.y / scalar); }`
5. **Assignment Overload**: `public Vector2 operator=(Vector2 other) { this.x = other.x; this.y = other.y; return this; }`

## 5. Negative Test Scenarios (Invalid Variations)

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
