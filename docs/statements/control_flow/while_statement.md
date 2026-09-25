# §15 WhileStatement

## 1. Overview & Scope

A `WhileStatement` implements pre-test iterative execution. Before each iteration, a boolean predicate expression is evaluated. If the condition evaluates to `true`, the loop body is executed, followed by an unconditional jump back to the condition evaluation. If the condition evaluates to `false`, loop execution terminates immediately, bypassing the body.

In Solix, `WhileStatement` loops coordinate with the compiler's label and patch management systems. The loop body establishes a loop context that permits `break` and `continue` statements, guaranteeing that any block-scoped reference variables allocated inside the body are cleaned up before breaking or continuing.

### Syntactic Placement
A `WhileStatement` is legally permitted within any subroutine body or lexical block.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
WhileStatement   ::= 'while' '(' Expression ')' Statement
```

### Canonical Code Patterns
```solix
// 1. Standard Counted Iteration
int32 i = 0;
while (i < 10) {
    Console.println(i);
    i++;
}

// 2. Sentinel Controlled Loop
while (stream.has_next()) {
    process(stream.read());
}

// 3. Infinite Loop with Break Escape
while (true) {
    if (is_complete()) {
        break;
    }
}
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Condition Scope & Boolean Invariant
- The condition expression is evaluated in the enclosing lexical scope before each iteration.
- **Strict Boolean Invariant**: The condition expression must evaluate to primitive `bool`. Non-boolean types trigger a compile-time type mismatch error.

### 3.2 Loop Body Declaration Space
- If the loop body is a `BlockStatement`, a new lexical scope is instantiated per iteration.
- Variables declared within the loop body are destroyed at the end of each iteration and re-instantiated in the subsequent iteration.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Normal Completion
1. **Loop Header (`loop_start_ip`)**: The condition expression is evaluated, pushing a `bool` onto the operand stack.
2. **Termination Check**:
   - The VM executes `OpCode::JUMP_IF_FALSE <loop_exit_ip>`.
   - If `false`, the VM jumps to `<loop_exit_ip>`, completing the loop normally.
3. **Body Execution**:
   - If `true`, statements within the loop body are executed sequentially.
4. **Loop Reiteration**:
   - Upon completing the loop body, the VM executes an unconditional `OpCode::JUMP <loop_start_ip>`.
   - Control transfers back to step 1.

### 4.2 Abrupt Completion
A `WhileStatement` completes abruptly if:
- A `break` statement executes within the body (transfers to `<loop_exit_ip>`).
- A `return` or `throw` statement executes within the body.

### 4.3 Continue Execution Flow
When a `continue` statement executes within the body:
- All active reference variables in intermediate blocks inside the loop are decremented via `DEC_REF`.
- The VM executes an unconditional jump directly to `<loop_start_ip>` to re-evaluate the condition.

---

## 5. Memory Model & ARC Invariants

### 5.1 Per-Iteration ARC Reclamation Invariant
- All reference variables declared inside the loop body block undergo deterministic `DEC_REF` deallocation at the closing brace of each iteration.
- Memory consumption remains constant ($O(1)$) across arbitrarily long iterations.

### 5.2 Compiler Lowering Structure (`assembler.cpp`)
```text
<loop_start_ip>:
  [ Evaluate Condition Expression ] ──► Pushes bool to Stack Top
  OpCode::JUMP_IF_FALSE <loop_exit_ip>
  [ Execute Loop Body Statements ]
  OpCode::JUMP <loop_start_ip>
<loop_exit_ip>: Normal Continuation
```

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Non-Boolean Condition Type
The condition must be of type `bool`.
```solix
void test_invalid_while() {
    int32 x = 10;
    while (x) { // Error: int32 not assignable to bool
        x--;
    }
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: While condition must be of type 'bool', got 'int32'
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Condition Evaluation Panic
Evaluating a condition that triggers a fault (e.g. null dereference `while (list.head.next != null)`) raises an immediate runtime panic.
*Runtime Fault*:
```text
[FATAL VM PANIC] NullReferenceException: Attempted to read field from null object reference
```

---

## 8. Conformance & Verification Examples

### Example 8.1: Memory Stability Across High Iteration Count
```solix
void verify_loop_memory_stability() {
    int32 count = 0;
    while (count < 100000) {
        String temp = new String("iteration_buffer");
        count++;
    }
    // Net heap memory must not grow monotonically
}
```
*Verification Invariant*: Heap allocation tracker confirms `temp` is deallocated at the conclusion of every single iteration. Net heap growth is 0.
