# BinaryExpression

## 1. Overview & Purpose

A `BinaryExpression` combines two operands using an infix operator (arithmetic, relational, logical, bitwise). Solix executes typed ALU opcodes (`ADD_I64`, `ADD_F64`, `EQ_I64`, etc.) and short-circuits `&&` and `||`.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
int32 sum = a + b;
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 1                 // Load a
GET_LOCAL 2                 // Load b
ADD_I64                     // Pops 2, pushes sum
SET_LOCAL 3                 // sum
```
