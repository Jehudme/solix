# ExpressionStatement (`NodeType::EXPR_STMT`)

## 1. Description & Purpose

An `expression statement` wraps an expression to evaluate it purely for its side effects (such as variable assignments, method invocations, or increment/decrement operations). Any computed result produced by the expression is systematically discarded from the VM operand stack (via a `POP` opcode) to preserve stack alignment across sequential statement execution.

## 2. Syntax & Grammar

```solix
<expression> ';'
```

## 3. Underlying Systems & Mechanics

- Evaluates expression for side effects (assignment, method call, increment).
- If the evaluated expression leaves an unused return value on the VM operand stack, the assembler emits a `POP` opcode to maintain balanced stack height.

## 4. Positive Test Scenarios (Valid Variations)

1. **Method Call Side Effect**: `Console.println("Log");`
2. **Increment / Decrement Expression**: `counter++; --index;`
3. **Assignment Expression**: `total = a + b;`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Missing Semicolon**:
   - `Console.println("Log")`  
     *Error*: `Expected ';' after expression`
2. **Naked Meaningless Expression**:
   - `42 + 10;` (evaluated and discarded with warning)
