# BinaryExpression (`NodeType::BINARY_EXPR`)

## 1. Description & Purpose

A `binary` expression combines two operand expressions using an infix operator to compute a single result value. Solix supports arithmetic operators (`+`, `-`, `*`, `/`, `%`), relational comparisons (`==`, `!=`, `<`, `<=`, `>`, `>=`), logical short-circuiting operators (`&&`, `||`), and bitwise operators (`&`, `|`, `^`, `<<`, `>>`). Binary expressions obey standard mathematical precedence and associativity rules.

## 2. Syntax & Grammar

```solix
<left-expr> <operator> <right-expr>
```
*Operators*: `+`, `-`, `*`, `/`, `%`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||`, `&`

## 3. Underlying Systems & Mechanics

- Resolves operator overload on left operand if class type.
- Emits dedicated typed ALU opcodes:
  - `int32`: `ADD_I32`, `SUB_I32`, `MUL_I32`, `DIV_I32`
  - `int64`: `ADD_I64`, `SUB_I64`, `MUL_I64`, `DIV_I64`
  - `float64`: `ADD_F64`, `SUB_F64`, `MUL_F64`, `DIV_F64`
- Reference equality: checks raw heap addresses.
- Null equality: supports null on either side.

## 4. Positive Test Scenarios (Valid Variations)

1. **Typed Primitive Arithmetic**: `int64 c = a + b; float64 d = x * y;`
2. **Subtype Null Equality**: `if (obj == null) ... if (null != obj) ...`
3. **Logical Short-Circuit**: `if (ptr != null && ptr.val > 0) { ... }`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Operands Type Mismatch**:
   - `int32 res = 10 + 5.5;`  
     *Error*: `Binary operands type mismatch: 'int32' vs 'float64'`
2. **Integer Division by Zero (Runtime)**:
   - `int32 q = 10 / 0;`  
     *Runtime Exception*: `DivideByZero`
3. **Comparing Incompatible Types**:
   - `if (new Dog() == new Engine())`  
     *Error*: `Cannot compare unrelated types 'Dog' and 'Engine'`
