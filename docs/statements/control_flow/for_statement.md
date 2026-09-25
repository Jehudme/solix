# §17 ForStatement

## 1. Overview & Scope

A `ForStatement` provides structured iteration through three distinct control clauses: an optional initialization clause (which introduces and binds an induction variable within a dedicated loop scope), an optional termination condition predicate, and an optional iteration update expression.

In Solix, the `ForStatement` manages an internal lexical scope that encapsulates the induction variable, preventing it from polluting the outer enclosing scope. ARC cleanups for induction variables and loop body variables are strictly coordinated across all iteration cycles, early breaks, and continue statements.

### Syntactic Placement
A `ForStatement` is legally permitted within any subroutine body or lexical block.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
ForStatement   ::= 'for' '(' ForInit? ';' Expression? ';' Expression? ')' Statement
ForInit        ::= VariableDeclaration | ExpressionList
ExpressionList ::= Expression (',' Expression)*
```

### Canonical Code Patterns
```solix
// 1. Canonical Counted Loop
for (int32 i = 0; i < 10; i++) {
    Console.println(i);
}

// 2. Multiple Induction Updates
for (int32 i = 0, int32 j = 10; i < j; i++, j--) {
    process(i, j);
}

// 3. Infinite For Loop
for (;;) {
    if (should_stop()) break;
}
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Loop Header Scope
- The `ForStatement` introduces a dedicated lexical scope encompassing the initialization clause, condition, update expression, and loop body.
- Variables declared in `ForInit` are accessible throughout the condition, step expression, and loop body, but are completely inaccessible outside the `ForStatement`.

### 3.2 Condition Type Invariant
- If present, the condition expression must evaluate to primitive `bool`.
- If omitted, the condition defaults to constant `true`.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Normal Completion
1. **Scope Initialization**:
   - The loop scope is entered.
   - If `ForInit` is present, it is evaluated (allocating and initializing induction variables).
2. **Loop Condition (`loop_start_ip`)**:
   - If a condition is present, it is evaluated. If `false`, control jumps to `<loop_exit_ip>`.
3. **Body Execution**:
   - The loop body statement is executed.
4. **Step Expression (`step_ip`)**:
   - The update expression is evaluated, and any result is popped from the stack.
5. **Reiteration**:
   - Control jumps unconditionally back to `<loop_start_ip>`.
6. **Normal Exit**:
   - When the condition evaluates to `false`, control branches to `<loop_exit_ip>`.
   - The loop scope undergoes ARC cleanup, decrementing reference induction variables.
   - Control resumes at the subsequent statement.

### 4.2 Abrupt Completion
A `ForStatement` completes abruptly if:
- A `break` statement executes within the body (transfers to `<loop_exit_ip>` after cleaning up intermediate scopes).
- A `return` or `throw` statement executes.

### 4.3 Continue Execution Flow
When a `continue` statement executes within the body:
- All active reference variables in intermediate blocks inside the loop are decremented via `DEC_REF`.
- Control jumps directly to `<step_ip>` to execute the iteration update expression before re-testing the condition.

---

## 5. Memory Model & ARC Invariants

### 5.1 Induction Variable Lifecycle
- If `ForInit` declares a reference variable (e.g. `for (String s = get_first(); s != null; s = s.next())`), `s` receives `INC_REF` upon initialization.
- Upon loop exit (whether normal or via `break`), `s` is decremented via `DEC_REF`.

### 5.2 Compiler Lowering Structure (`assembler.cpp`)
```text
[ Execute ForInit (Loop Scope) ]
<loop_start_ip>:
  [ Evaluate Condition (if present) ] ──► Pushes bool
  OpCode::JUMP_IF_FALSE <loop_exit_ip>
  [ Execute Loop Body ]
<step_ip>: (Target of 'continue' statements)
  [ Evaluate Step Expression (if present) ]
  OpCode::JUMP <loop_start_ip>
<loop_exit_ip>: (Target of 'break' statements)
  [ Loop Scope Cleanup: DEC_REF Induction Variables ]
```

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Non-Boolean Condition Type
The condition expression must be of type `bool`.
```solix
void test_invalid_for() {
    for (int32 i = 0; i; i++) { // Error: i is not bool
    }
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: For condition must be of type 'bool', got 'int32'
```

### Rule 6.2: Induction Variable Leakage
Accessing an induction variable outside the for loop is rejected.
```solix
void test_leak() {
    for (int32 i = 0; i < 5; i++) {}
    Console.println(i); // Error: i is undefined
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Undefined identifier: i
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Step Expression Runtime Panic
An unhandled exception in the step expression raises a runtime fault and unwinds the loop.

---

## 8. Conformance & Verification Examples

### Example 8.1: Continue Correctly Advances Step Expression
```solix
void verify_continue_advances_step() {
    int32 hit_count = 0;
    for (int32 i = 0; i < 10; i++) {
        if (i % 2 == 0) {
            continue; // Must jump to i++, not bypass it!
        }
        hit_count++;
    }
    // hit_count must equal 5
}
```
*Verification Invariant*: `hit_count` equals 5. Disassembly verifies `continue` branches to `<step_ip>`.
