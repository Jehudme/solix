# §26 BinaryExpression

## 1. Overview & Scope

A `BinaryExpression` combines two operand expressions using an infix operator to compute a single result value. Solix supports arithmetic operators (`+`, `-`, `*`, `/`, `%`), relational comparisons (`==`, `!=`, `<`, `<=`, `>`, `>=`), logical short-circuiting operators (`&&`, `||`), and bitwise operators (`&`, `|`, `^`, `<<`, `>>`).

### Syntactic Placement
Permitted anywhere an expression is syntactically valid.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
BinaryExpression ::= Expression BinaryOperator Expression
BinaryOperator   ::= '+' | '-' | '*' | '/' | '%'
                   | '==' | '!=' | '<' | '<=' | '>' | '>='
                   | '&&' | '||' | '&' | '|' | '^' | '<<' | '>>'
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Type Checking & Overload Resolution
- Operands are validated for numeric type compatibility.
- If operands are user-defined class instances, the compiler checks for matching `OperatorDeclaration` overloads.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Evaluation Sequence
- For standard operators: Evaluates left operand, evaluates right operand, executes typed ALU opcode (e.g. `ADD_I64`, `ADD_F64`, `EQ_I64`).
- For `&&` and `||`: Evaluates left operand; if outcome is determined, short-circuits evaluation of the right operand.

---

## 5. Memory Model & ARC Invariants

### 5.1 Stack Delta
- Consumes 2 stack operands and pushes 1 result operand.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Operator Type Mismatch
```solix
bool b = "hello" - 5; // Error: operator '-' undefined
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Operator '-' cannot be applied to types 'String' and 'int32'
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Integer Division by Zero
Dividing by zero raises `ArithmeticException`.

---

## 8. Conformance & Verification Examples

### Example 8.1: Logical Short-Circuit
```solix
bool res = false && (10 / 0 == 1); // Does not divide by zero!
```
*Verification Invariant*: `res` is `false`; no exception is thrown.
