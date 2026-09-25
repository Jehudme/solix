# §16 DoWhileStatement

## 1. Overview & Scope

A `DoWhileStatement` implements post-test iterative execution. Unlike the `WhileStatement`, the loop body of a `DoWhileStatement` is guaranteed to execute at least once before the termination condition is evaluated. At the conclusion of each iteration, the boolean predicate condition is checked; if `true`, control loops back to the beginning of the body; if `false`, the loop terminates.

In Solix, `DoWhileStatement` coordinates with loop label registers to correctly support `break` (jumping past the condition to the loop exit) and `continue` (jumping directly to the condition evaluation at the foot of the loop).

### Syntactic Placement
A `DoWhileStatement` is legally permitted within any subroutine body or lexical block.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
DoWhileStatement   ::= 'do' Statement 'while' '(' Expression ')' ';'
```

### Canonical Code Patterns
```solix
// 1. Guaranteed Single Execution
int32 attempts = 0;
do {
    attempts++;
    try_connect();
} while (attempts < 3 && !is_connected());

// 2. Interactive Input Loop
int32 choice;
do {
    choice = read_user_choice();
} while (choice != 0);
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Condition Scope & Boolean Invariant
- The condition expression is evaluated after each execution of the loop body.
- **Strict Boolean Invariant**: The condition expression must evaluate to primitive `bool`.
- Note: Variables declared inside the loop body block are *not* in scope within the `while(...)` condition expression at the foot of the loop.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Normal Completion
1. **Body Execution (`loop_start_ip`)**: Statements within the loop body are executed sequentially.
2. **Condition Target (`condition_ip`)**:
   - The condition expression is evaluated, pushing a `bool` onto the operand stack.
3. **Reiteration Check**:
   - The VM executes `OpCode::JUMP_IF_TRUE <loop_start_ip>`.
   - If `true`, the VM loops back to step 1.
   - If `false`, execution falls through to `<loop_exit_ip>`.
4. Normal sequential execution resumes after the `DoWhileStatement`.

### 4.2 Abrupt Completion
A `DoWhileStatement` completes abruptly if:
- A `break` statement executes within the body (transfers to `<loop_exit_ip>`).
- A `return` or `throw` statement executes within the body.

### 4.3 Continue Execution Flow
When `continue` executes within the body:
- All active reference variables in intermediate blocks are decremented via `DEC_REF`.
- The VM executes an unconditional jump directly to `<condition_ip>` to evaluate the condition.

---

## 5. Memory Model & ARC Invariants

### 5.1 Per-Iteration ARC Reclamation
- Variables declared inside the body block are decremented at the end of each iteration before the condition check executes.

### 5.2 Compiler Lowering Structure (`assembler.cpp`)
```text
<loop_start_ip>:
  [ Execute Loop Body Statements ]
<condition_ip>: (Target of 'continue' statements)
  [ Evaluate Condition Expression ] ──► Pushes bool to Stack Top
  OpCode::JUMP_IF_TRUE <loop_start_ip>
<loop_exit_ip>: (Target of 'break' statements)
```

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Body-Declared Variable Inaccessibility in Condition
Variables declared inside the `do` block cannot be accessed within the `while` condition.
```solix
void test_scope_leak() {
    do {
        int32 local_state = read();
    } while (local_state != 0); // Error: local_state not visible here
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Undefined identifier: local_state
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Condition Evaluation Panic
Evaluating a condition that triggers a runtime fault raises an exception and unwinds the loop.

---

## 8. Conformance & Verification Examples

### Example 8.1: Guaranteed Single Execution with False Condition
```solix
void verify_single_execution() {
    int32 count = 0;
    do {
        count++;
    } while (false);
    // count must equal 1
}
```
*Verification Invariant*: `count` equals 1. Body executed exactly once despite condition evaluating to false.
