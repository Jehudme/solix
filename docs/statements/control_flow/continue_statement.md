# §20 ContinueStatement

## 1. Overview & Scope

A `ContinueStatement` abandons the remainder of the current loop iteration and immediately transfers control to the continuation point of the innermost enclosing iteration statement (`while`, `do-while`, or `for`).

In Solix's Automatic Reference Counting (ARC) architecture, executing `continue` from inside nested blocks within a loop body properly decrements and releases all local reference variables declared up to that point before jumping to the loop's step or condition evaluation, preventing resource accumulation across loop cycles.

### Syntactic Placement
A `ContinueStatement` is legally permitted **only** inside iteration statements (`while`, `do-while`, `for`). Using `continue` inside `switch` statements (unless enclosed in a loop) or inside standard function bodies is a compile-time error.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
ContinueStatement   ::= 'continue' ';'
```

### Canonical Code Patterns
```solix
// 1. Skipping Iterations in While Loop
int32 i = 0;
while (i < 10) {
    i++;
    if (i % 2 == 0) {
        continue; // Advances to condition check
    }
    process_odd(i);
}

// 2. Skipping Iterations in For Loop
for (int32 i = 0; i < 100; i++) {
    if (should_skip(i)) {
        continue; // Advances to step expression (i++)
    }
    process(i);
}
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Loop Enclosure Invariant
- A `ContinueStatement` must be statically enclosed within an iteration statement (`while`, `do-while`, or `for`).
- Unlike `break`, `continue` cannot target a `switch` statement.

### 3.2 Reachability Invariant
- Statements following `continue` in the same sequential block are statically unreachable dead code.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Normal Completion
- A `ContinueStatement` **never completes normally**. It always results in an abrupt completion.

### 4.2 Abrupt Completion
Execution of a `ContinueStatement` proceeds through the following operational steps:
1. **Ancestor Scope Traversal**:
   - The compiler traces upward to the innermost enclosing loop construct.
   - For every intermediate `BlockStatement` encountered along this path, the compiler emits `emit_cleanup_for_node(current)`.
2. **Intermediate ARC Cleanup**:
   - All local reference variables declared in intermediate blocks are decremented via `DEC_REF`.
3. **Continuation Branch**:
   - The VM executes `OpCode::JUMP <loop_continue_patch>`.
   - **Target Destinations**:
     - In `for` loops: Jumps directly to `<step_ip>` (evaluates update expression, then condition).
     - In `while` loops: Jumps directly to `<loop_start_ip>` (re-evaluates condition).
     - In `do-while` loops: Jumps directly to `<condition_ip>` (evaluates condition).

---

## 5. Memory Model & ARC Invariants

### 5.1 Clean Iteration Invariant
- Intermediate reference objects declared prior to the `continue` statement are deallocated immediately.
- Memory consumption does not leak across repeated continue jumps.

### 5.2 Compiler Lowering Logic (`assembler.cpp`)
```cpp
void Assembler::visit(ContinueStatement &node) {
  Node *current = node.parent;
  while (current &&
         current->node_type != NodeType::WHILE_STMT &&
         current->node_type != NodeType::FOR_STMT &&
         current->node_type != NodeType::DO_WHILE_STMT) {
    if (current->node_type == NodeType::BLOCK) {
      emit_cleanup_for_node(current);
    }
    current = current->parent;
  }
  emit_byte(static_cast<uint8_t>(OpCode::JUMP));
  loop_continue_patches.back().push_back(bytecode().size());
  emit_int32(0xFFFFFFFF);
}
```

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Continue Outside of Loop
Using `continue` when not enclosed by a loop construct is illegal.
```solix
void test_illegal_continue() {
    int32 x = 10;
    continue; // Error: continue outside of loop
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: 'continue' statement not allowed outside of loop
```

---

## 7. Runtime Fault Conditions

A `ContinueStatement` generates no dynamic runtime faults; verification is performed statically.

---

## 8. Conformance & Verification Examples

### Example 8.1: Continue Advances For Loop Step Expression
```solix
void verify_continue_cycle() {
    int32 count = 0;
    for (int32 i = 0; i < 5; i++) {
        String temp = new String("iter_temp");
        if (i >= 0) {
            continue; // Must clean up 'temp' and execute i++
        }
        count++;
    }
}
```
*Verification Invariant*: Disassembly confirms `temp` is decremented via `DEC_REF` prior to jumping to `<step_ip>`. Loop iterates exactly 5 times; 0 heap leaks.
