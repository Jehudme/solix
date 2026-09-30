# `operator`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `operator` |
| Category | Expression / Declaration |
| Context | Inside class bodies declaring custom operator overloads |
| Related | [`class`](../declarations/class.md) |

`operator` is a keyword used to declare custom operator overloading methods within Solix classes. It allows user-defined classes (such as math vectors, complex numbers, matrices, or custom strings) to redefine standard arithmetic, relational, and assignment operators (`+`, `-`, `*`, `/`, `==`, `!=`, `=`) using natural, expressive operator syntax.

## 2. Permitted Contexts (Syntax & Grammar)

```
OperatorDeclaration : [AccessModifier] ReturnType 'operator' OverloadableOp '(' ParameterList ')' Block
OverloadableOp : '+' | '-' | '*' | '/' | '%' | '==' | '!=' | '<' | '<=' | '>' | '>=' | '='
```

- Permitted only within class declarations.
- Must be an instance method (takes `this` as the left-hand operand).
- Binary operators take exactly one parameter (the right-hand operand).
- Overloading assignment `operator=` returns a reference to the modified instance (`this`).

## 3. Semantics & Compiler Rules

- **Name Mangling**: In the compiler binder, an operator method is mangled as `ClassName.operator+(RightType)`.
- **Desugaring**: When the compiler encounters an expression `a + b` where `a` is of a class type, it transforms the expression into a method call `a.operator+(b)`.
- **Operator Precedence**: Custom operator overloads retain the exact same language precedence and associativity as built-in primitive operators.
- **Copy Assignment**: `operator=` allows defining deep copy semantics when assigning one instance to another.

## 4. Code Examples

### Basic Usage

```solix
package solix.math;

public class Vector2D {
    public float64 x;
    public float64 y;

    public Vector2D(float64 x, float64 y) {
        this.x = x;
        this.y = y;
    }

    public Vector2D operator+(Vector2D other) {
        return new Vector2D(this.x + other.x, this.y + other.y);
    }

    public Vector2D operator-(Vector2D other) {
        return new Vector2D(this.x - other.x, this.y - other.y);
    }
}
```

### Idiomatic Usage

```solix
package solix.text;

public class CustomBuffer {
    private char[] buffer;
    private int32 length;

    public CustomBuffer() {
        this.buffer = new char[16];
        this.length = 0;
    }

    // Overloading copy assignment operator
    public CustomBuffer operator=(CustomBuffer other) {
        this.buffer = new char[other.length];
        for (int32 i = 0; i < other.length; i = i + 1) {
            this.buffer[i] = other.buffer[i];
        }
        this.length = other.length;
        return this;
    }

    // Overloading equality comparison
    public bool operator==(CustomBuffer other) {
        if (this.length != other.length) {
            return false;
        }
        for (int32 i = 0; i < this.length; i = i + 1) {
            if (this.buffer[i] != other.buffer[i]) {
                return false;
            }
        }
        return true;
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Declaring a static operator overload | `Operator overloads must be instance methods` |
| Overloading an unsupported token (`operator&&`) | `Token '&&' cannot be overloaded` |
| Binary operator having wrong number of parameters | `Operator overload '+' requires exactly 1 parameter` |

## 6. Related Keywords & Guides

- [`class`](../declarations/class.md) — Class definitions hosting operator methods
- [Operator Declaration Spec](../../../spec/statements/declarations/operator_declaration.md) — Specification and bytecode lowering of operators
