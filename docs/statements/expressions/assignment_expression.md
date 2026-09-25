# §24 AssignmentExpression

## 1. Overview & Scope

An `AssignmentExpression` evaluates a right-hand side (RHS) expression and stores the computed value into an lvalue storage location (a local variable register, a class instance field, or an array element subscript). Solix supports standard assignment (`=`) and compound assignment operators (`+=`, `-=`, `*=`, `/=`).

In Solix's Automatic Reference Counting (ARC) architecture, assignment to an lvalue holding a reference type is an ownership-transfer operation. The compiler emits instructions to increment the reference counter of the incoming value (`INC_REF`) and decrement the reference counter of the displaced previous value (`DEC_REF`), ensuring that object graphs update deterministically without leaks.

### Syntactic Placement
An `AssignmentExpression` may appear anywhere an expression is syntactically permitted, frequently wrapped inside an `ExpressionStatement`.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
AssignmentExpression ::= LValue AssignmentOperator Expression
AssignmentOperator   ::= '=' | '+=' | '-=' | '*=' | '/='
LValue               ::= Identifier
                       | Expression '.' Identifier
                       | Expression '[' Expression ']'
```

### Canonical Code Patterns
```solix
// 1. Variable Assignment
counter = 42;

// 2. Member Field Assignment
user.email = new String("user@solix.dev");

// 3. Array Element Assignment
buffer[0] = 255;

// 4. Compound Assignment
total += item_price;
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 LValue Validity & Type Checking
- The target of an assignment must be a valid, mutable lvalue. Literals, temporary results, and function call returns cannot be assigned to.
- The static type of the RHS expression must be assignable to the static type of the lvalue (`is_assignable`).

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Execution Sequence
1. The RHS expression is evaluated, leaving its value on top of the operand stack.
2. If assigning to a reference-type lvalue:
   - The VM executes `OpCode::INC_REF` for the incoming reference.
   - The previous reference stored in the lvalue is decremented via `DEC_REF`.
3. The value is stored into the target storage location (`SET_LOCAL`, `SET_PROPERTY`, or `SET_ARRAY_ELEMENT`).
4. The assigned value remains on the operand stack as the result of the assignment expression (enabling chained assignment `a = b = c`).

---

## 5. Memory Model & ARC Invariants

### 5.1 Ownership Transfer Invariant
- Overwriting a reference variable decrements the prior object's `ref_count`. If `ref_count == 0`, the prior object is immediately deallocated.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Invalid Assignment Target
Assigning to an rvalue (e.g. literal) is illegal.
```solix
void test_invalid_lvalue() {
    10 = x; // Error: invalid target
}
```
*Diagnostic Message*:
```text
[ERROR] parser.cpp: Invalid assignment target
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Assignment to Field on Null Receiver
Writing to `obj.field = val` when `obj` is `null` raises a `NullReferenceException`.

---

## 8. Conformance & Verification Examples

### Example 8.1: Chained Assignment
```solix
int32 a; int32 b; int32 c;
a = b = c = 10;
```
*Verification Invariant*: `a`, `b`, and `c` all equal 10.
