# LiteralNode (`NodeType::LITERAL`)

## 1. Description & Purpose

A `literal` node represents a constant scalar or string value embedded directly in the program source text. Solix supports integral literals (`int8`, `int16`, `int32`, `int64`, and unsigned variants), floating-point literals (`float32`, `float64`), boolean literals (`true`, `false`), character literals (`'a'`), string literals (`"text"`), and the `null` pointer literal. Literals are loaded onto the operand stack via immediate load opcodes (e.g. `PUSH_INT`, `PUSH_FLOAT`, `PUSH_NULL`).

## 2. Syntax & Grammar

- Integer: `123`, `0xFF`
- Float: `3.14`, `0.5`
- Character: `'a'`, `'\n'`
- String: `"hello"`
- Boolean: `true`, `false`
- Null: `null`

## 3. Underlying Systems & Mechanics

- Primitive literals emitted directly as immediate bytecode operands (`PUSH_I32`, `PUSH_I64`, `PUSH_F64`).
- String literals interned in bytecode header constant pool.
- Null emitted as `PUSH_NULL` (reference address 0).

## 4. Positive Test Scenarios (Valid Variations)

1. **Numeric Literals**: `100`, `0x1F`, `3.14159`
2. **Escaped Chars**: `'\n'`, `'\t'`, `'\\'`, `'\''`
3. **String Literals**: `"Solix Language"`
4. **Bool Literals**: `true`, `false`
5. **Null Literal**: `null`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Unterminated String**:
   - `"hello`  
     *Error*: `Unterminated string literal`
2. **Empty Character Literal**:
   - `''`  
     *Error*: `Empty character literal`
3. **Multi-Character Character Literal**:
   - `'abc'`  
     *Error*: `Character literal contains multiple characters`
