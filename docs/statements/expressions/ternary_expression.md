# §25 TernaryExpression

## 1. Overview & Scope

A `TernaryExpression` (`condition ? true_expr : false_expr`) is an inline conditional operator that yields one of two expressions based on the boolean outcome of a predicate condition. Evaluation is strictly lazy and short-circuiting: only the branch corresponding to the evaluated condition is executed.

### Syntactic Placement
Permitted anywhere an expression is syntactically valid.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
TernaryExpression ::= Expression '?' Expression ':' Expression
```

### Canonical Code Patterns
```solix
String status = is_connected ? "Online" : "Offline";
int32 max_val = a > b ? a : b;
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Condition & Branch Type Unification
- The condition expression must evaluate to primitive `bool`.
- The true and false branches must unify to a common ancestor type (`is_assignable`).

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Short-Circuit Execution
1. Evaluates condition expression.
2. Emits `OpCode::JUMP_IF_FALSE <false_branch_ip>`.
3. If true, evaluates true expression, emits `OpCode::JUMP <end_ip>`.
4. If false, evaluates false expression.
5. The result of the executed branch remains on the operand stack.

---

## 5. Memory Model & ARC Invariants

### 5.1 Lazy Evaluation Invariant
- The unselected branch is never evaluated. Any allocations or ARC increments in the unselected branch do not occur.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Non-Boolean Condition
```solix
int32 x = 5 ? 1 : 2; // Error: condition not bool
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Ternary condition must be of type 'bool', got 'int32'
```

---

## 7. Runtime Fault Conditions

Faults in the evaluated branch propagate normally; faults in the unselected branch are never triggered.

---

## 8. Conformance & Verification Examples

### Example 8.1: Short-Circuit Side-Effect Avoidance
```solix
String safe = (obj != null) ? obj.name : "default";
```
*Verification Invariant*: When `obj == null`, `obj.name` is never evaluated, preventing `NullReferenceException`.
