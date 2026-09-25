# §18 SwitchStatement

## 1. Overview & Scope

A `SwitchStatement` provides multi-way control-flow branching based on the value of a selector expression. The selector expression is evaluated once and compared sequentially against literal constant expressions in `CaseStatement` clauses. When a match occurs, execution transfers to the corresponding case body. If no case matches and an optional `default` clause is present, control branches to the default body.

In Solix, switch statements support integer primitives, characters, and enumeration types. Solix implements fall-through execution semantics: once a case matches, execution proceeds sequentially into subsequent cases unless an explicit `BreakStatement` is executed.

### Syntactic Placement
A `SwitchStatement` is legally permitted within any subroutine body or lexical block.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
SwitchStatement   ::= 'switch' '(' Expression ')' '{' CaseClause* '}'
CaseClause        ::= ('case' Expression ':' | 'default' ':') Statement*
```

### Canonical Code Patterns
```solix
// 1. Integral Primitive Switch with Breaks
switch (status_code) {
    case 200:
        Console.println("OK");
        break;
    case 404:
        Console.println("Not Found");
        break;
    default:
        Console.println("Unknown Status");
        break;
}

// 2. Intentional Fall-Through
switch (level) {
    case 3:
        grant_admin();
    case 2:
        grant_editor();
    case 1:
        grant_viewer();
        break;
}
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Selector Type Constraints
- The selector expression must evaluate to an integral primitive type (`int8`, `int16`, `int32`, `int64`, `uint8`, `uint16`, `uint32`, `uint64`), `char`, or an `enum` type. Floating-point and reference types are prohibited.

### 3.2 Case Constant Invariants
- Each `case` expression must be a compile-time constant literal or enum member.
- Duplicate case constant values within the same switch statement are rejected at compile time.
- At most one `default` clause is permitted per switch statement.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Normal Completion
1. **Selector Evaluation**: The selector expression is evaluated, leaving its value on top of the operand stack.
2. **Case Comparison Dispatch**:
   - For each case:
     - The selector value is duplicated via `OpCode::DUP`.
     - The case constant expression is evaluated.
     - The VM executes `OpCode::EQ_I64`.
     - `OpCode::JUMP_IF_TRUE <case_body_ip>`: If match, control branches to that case's body.
3. **Default / Exit Fall-Through**:
   - If no case matches:
     - If a `default` clause exists, control branches to `<default_body_ip>`.
     - Otherwise, the selector value is popped via `OpCode::POP`, and control jumps to `<switch_exit_ip>`.
4. **Body Execution & Fall-Through**:
   - Once a branch is taken, statements execute sequentially. If no `break` is encountered, execution falls through into subsequent case statements.
5. **Switch Exit**:
   - Upon encountering `break`, control jumps directly to `<switch_exit_ip>`.
   - The selector value is cleaned up from the operand stack.

### 4.2 Abrupt Completion
A `SwitchStatement` completes abruptly if a contained statement completes abruptly via `return` or `throw`.

---

## 5. Memory Model & ARC Invariants

### 5.1 Operand Stack Selector Management
- The selector value remains on the stack during dispatch comparisons.
- Before jumping to `<switch_exit_ip>` or executing case bodies, the compiler ensures operand stack balance by consuming or popping temporary comparisons.

### 5.2 Break ARC Cleanup
- Executing `break` within a case block cleans up any block-scoped reference variables declared within that case prior to branching to `<switch_exit_ip>`.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Duplicate Case Value
Declaring two identical case values in the same switch statement is rejected.
```solix
void test_duplicate_case(int32 x) {
    switch (x) {
        case 1: break;
        case 1: break; // Error: duplicate case value
    }
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Duplicate case value '1' in switch statement
```

### Rule 6.2: Multiple Default Clauses
A switch statement can contain at most one default clause.
```solix
void test_multiple_default(int32 x) {
    switch (x) {
        default: break;
        default: break; // Error: multiple default clauses
    }
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Switch statement already contains a default clause
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Selector Evaluation Panic
If the selector expression triggers an unhandled arithmetic fault, an exception is raised prior to case matching.

---

## 8. Conformance & Verification Examples

### Example 8.1: Fall-Through Accumulation
```solix
int32 verify_fallthrough(int32 tier) {
    int32 score = 0;
    switch (tier) {
        case 1: score += 10;
        case 2: score += 20;
        case 3: score += 30; break;
        default: score = 0; break;
    }
    return score;
}
```
*Verification Invariant*: Calling `verify_fallthrough(1)` executes cases 1, 2, and 3, returning 60. Calling with 2 returns 50.
