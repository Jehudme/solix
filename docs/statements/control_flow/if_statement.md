# §14 IfStatement

## 1. Overview & Scope

An `IfStatement` provides conditional branching based on the evaluation of a boolean predicate expression. Depending on whether the condition evaluates to `true` or `false`, control transfers either to the `then` branch or to an optional `else` branch.

In Solix, conditional branching is strictly type-safe. The predicate condition must evaluate to the primitive boolean type `bool`. Truthy or falsy coercion of integers, floats, or pointers is strictly prohibited. The compiler lowers `IfStatement` constructs into conditional jump instructions (`JUMP_IF_FALSE`) with forward jump patching.

### Syntactic Placement
An `IfStatement` is legally permitted within any subroutine body, lexical block, or control-flow construct.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
IfStatement   ::= 'if' '(' Expression ')' Statement ('else' Statement)?
```

### Canonical Code Patterns
```solix
// 1. Basic Single Branch
if (is_active) {
    start_service();
}

// 2. Dual Branch (if-else)
if (count > 0) {
    process(count);
} else {
    log_empty();
}

// 3. Multi-Branch Chain (if-else if-else)
if (score >= 90) {
    grade = 'A';
} else if (score >= 80) {
    grade = 'B';
} else {
    grade = 'F';
}
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Predicate Scope & Type Constraint
- The condition expression is evaluated within the enclosing lexical scope.
- **Strict Boolean Invariant**: The condition expression must evaluate to primitive `bool`. If the condition evaluates to any other type (such as `int32`, `String`, or an object pointer), compilation halts with an explicit type mismatch diagnostic.

### 3.2 Branch Declaration Spaces
- If the `then` or `else` branch is a `BlockStatement`, each branch establishes an independent lexical scope.
- Variables declared in the `then` branch are completely invisible in the `else` branch and vice versa.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Normal Completion
1. **Condition Evaluation**: The predicate condition is evaluated to completion, pushing a `bool` value onto the operand stack.
2. **Conditional Branching**:
   - The VM executes `OpCode::JUMP_IF_FALSE <else_patch_ip>`.
   - The top boolean value is popped from the stack.
   - If `true`: Execution proceeds sequentially into the `then` branch. Upon completing the `then` branch:
     - If an `else` branch exists, the VM executes an unconditional `OpCode::JUMP <end_patch_ip>` to bypass the `else` branch.
   - If `false`: The VM jumps directly to `<else_patch_ip>`. If an `else` branch exists, it is executed; otherwise, execution resumes at `<end_patch_ip>`.
3. Normal sequential execution resumes after the `IfStatement`.

### 4.2 Abrupt Completion
An `IfStatement` completes abruptly if:
- Evaluation of the condition expression throws an exception.
- The executed branch (`then` or `else`) completes abruptly due to `return`, `break`, `continue`, or `throw`.

### 4.3 Exception Unwinding & Trampolines
If an executed branch completes abruptly due to `throw`, the branch's block unwinds its local variables before transferring control to the enclosing block's exception cleanup segment.

---

## 5. Memory Model & ARC Invariants

### 5.1 Branch Scope Isolation
- Any reference variables declared within the `then` branch block undergo ARC `DEC_REF` deallocation prior to jumping past the `else` branch.
- Any reference variables declared within the `else` branch block undergo ARC `DEC_REF` deallocation prior to reaching the continuation target.

### 5.2 Compiler Lowering Structure (`assembler.cpp`)
```text
[ Evaluate Condition Expression ] ──► Pushes bool to Stack Top
              │
              ▼
    OpCode::JUMP_IF_FALSE <else_ip>
              │
              ├── (If True) ──► [ Execute Then Branch ]
              │                        │
              │                        ▼
              │                 OpCode::JUMP <end_ip>
              │
              └── (If False) ──► <else_ip>:
                                [ Execute Else Branch (if present) ]
                                       │
                                       ▼
                                 <end_ip>: Normal Continuation
```

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Non-Boolean Condition Type
The condition expression must be of type `bool`. Numeric or pointer truthiness is rejected.
```solix
void test_invalid_cond() {
    int32 count = 5;
    if (count) { // Illegal: int32 cannot be coerced to bool
        Console.println("yes");
    }
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: If condition must be of type 'bool', got 'int32'
```

### Rule 6.2: Dangling Else Ambiguity Resolution
An `else` clause binds syntactically to the nearest preceding `if` statement lacking an `else`.

---

## 7. Runtime Fault Conditions

### Fault 7.1: Predicate Evaluation Fault
If evaluating the condition expression encounters an arithmetic error (e.g. `if (10 / 0 == 2)`), an `ArithmeticException` is raised before any branch executes.
*Runtime Fault*:
```text
[FATAL VM PANIC] ArithmeticException: Division by zero
```

---

## 8. Conformance & Verification Examples

### Example 8.1: Mutual Exclusion of Branch Deallocations
```solix
void verify_branch_cleanup(bool flag) {
    if (flag) {
        String s1 = new String("branch_a");
    } else {
        String s2 = new String("branch_b");
    }
    // Exactly one string is allocated and immediately freed
}
```
*Verification Invariant*: When `flag == true`, `s1` is deallocated at the end of the `then` block; `s2` is never created. Net heap allocation is 0.
