# UnaryExpression (`NodeType::UNARY_EXPR`)

## 1. Description & Purpose

A `unary` expression applies an operation to a single operand. Solix supports prefix logical negation (`!`), bitwise inversion (`~`), arithmetic negation (`-`), unary plus (`+`), as well as prefix and postfix increment (`++`) and decrement (`--`) operators. Prefix operations update and yield the new value, whereas postfix operations yield the original value before mutating the underlying lvalue.

## 2. Syntax & Grammar

```solix
('-' | '+' | '!' | '++' | '--') <expr>   // Prefix
<expr> ('++' | '--')                    // Postfix
```

## 3. Underlying Systems & Mechanics

- Negation (`-`): emits `NEG_I32`, `NEG_I64`, or `NEG_F64`.
- Inversion (`!`): requires `bool`, emits `LOGICAL_NOT`.
- Postfix `++`: evaluates value, duplicates, increments lvalue in place, leaves original value on stack.
- Prefix `++`: increments lvalue in place, leaves incremented value on stack.

## 4. Positive Test Scenarios (Valid Variations)

1. **Prefix Increment / Decrement**: `++count; --count;`
2. **Postfix Increment / Decrement**: `int32 old = count++;`
3. **Numeric Negation**: `int64 neg = -big_val;`
4. **Boolean Inversion**: `bool opposite = !flag;`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Unary Not on Non-Bool**:
   - `int32 x = !0;`  
     *Error*: `Unary operator '!' requires bool operand`
2. **Incrementing Literal or R-Value**:
   - `5++;` or `(a + b)++;`  
     *Error*: `Increment/decrement operand must be an assignable variable (lvalue)`
