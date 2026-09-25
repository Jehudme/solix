# §19 BreakStatement

## 1. Overview & Scope

A `BreakStatement` initiates an abrupt, non-local transfer of control that immediately terminates the innermost enclosing iteration statement (`while`, `do-while`, `for`) or `SwitchStatement`.

In Solix's Automatic Reference Counting (ARC) architecture, a break statement does not perform an unmonitored jump. When breaking out of nested lexical blocks inside a loop, the compiler walks the AST ancestor hierarchy, emitting inline `DEC_REF` instructions for all active reference variables across all intermediate blocks before issuing the final jump to the construct's exit boundary.

### Syntactic Placement
A `BreakStatement` is legally permitted **only** inside iteration statements (`while`, `do-while`, `for`) or `SwitchStatement` blocks. Using `break` outside of these constructs is a compile-time syntax/semantic error.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
BreakStatement   ::= 'break' ';'
```

### Canonical Code Patterns
```solix
// 1. Terminating Loop on Condition
while (has_more()) {
    Item item = read();
    if (item.is_poison()) {
        break; // Exits while loop
    }
}

// 2. Terminating Switch Case
switch (command) {
    case 1:
        start();
        break; // Prevents fall-through
}
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Enclosure Requirement
- A `BreakStatement` must be statically enclosed within at least one loop (`while`, `do-while`, `for`) or `switch` construct.
- Attempting to use `break` in a standalone block or method body outside a loop/switch produces a compile-time diagnostic.

### 3.2 Reachability Invariant
- A `BreakStatement` completes abruptly and unconditionally. Any statements following `break` within the same sequential statement block are statically unreachable.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Normal Completion
- A `BreakStatement` **never completes normally**. It always results in an abrupt completion.

### 4.2 Abrupt Completion
Execution of a `BreakStatement` proceeds through the following operational steps:
1. **Ancestor Scope Traversal**:
   - The compiler traces upward from the `BreakStatement` to the target enclosing loop or `switch` construct.
   - For every intermediate `BlockStatement` encountered along this path, the compiler emits `emit_cleanup_for_node(current)`.
2. **Intermediate ARC Cleanup**:
   - All local reference variables declared in intermediate blocks are decremented via `DEC_REF` in reverse declaration order.
3. **Target Jump Execution**:
   - The VM executes `OpCode::JUMP <loop_break_patch>`.
   - Control transfers immediately to the target construct's exit label.

### 4.3 Exception Unwinding & Trampolines
If a break occurs inside a `try` block nested within a loop, any associated `finally` block executes before the break transfer completes.

---

## 5. Memory Model & ARC Invariants

### 5.1 Intermediate Block Deallocation Invariant
- Breaking out of deeply nested blocks must release all intermediate reference objects.
- Example:
  ```solix
  while (true) {
      String outer = new String("outer");
      {
          String inner = new String("inner");
          break; // Must emit DEC_REF inner, then DEC_REF outer!
      }
  }
  ```
  The compiler guarantees that both `inner` and `outer` are decremented before the jump occurs.

### 5.2 Compiler Lowering Logic (`assembler.cpp`)
```cpp
void Assembler::visit(BreakStatement &node) {
  Node *current = node.parent;
  while (current &&
         current->node_type != NodeType::WHILE_STMT &&
         current->node_type != NodeType::FOR_STMT &&
         current->node_type != NodeType::DO_WHILE_STMT &&
         current->node_type != NodeType::SWITCH_STMT) {
    if (current->node_type == NodeType::BLOCK) {
      emit_cleanup_for_node(current);
    }
    current = current->parent;
  }
  emit_byte(static_cast<uint8_t>(OpCode::JUMP));
  loop_break_patches.back().push_back(bytecode().size());
  emit_int32(0xFFFFFFFF);
}
```

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Break Outside of Loop or Switch
Using `break` when no enclosing loop or switch construct exists is illegal.
```solix
void test_illegal_break() {
    int32 x = 10;
    break; // Error: break outside of loop or switch
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: 'break' statement not allowed outside of loop or switch
```

---

## 7. Runtime Fault Conditions

A `BreakStatement` itself generates no dynamic runtime faults; all constraints are verified statically during compilation.

---

## 8. Conformance & Verification Examples

### Example 8.1: Leak-Free Break from Deeply Nested Blocks
```solix
void verify_break_cleanup() {
    while (true) {
        String s1 = new String("s1");
        {
            String s2 = new String("s2");
            {
                String s3 = new String("s3");
                break; // Must clean up s3, s2, and s1
            }
        }
    }
}
```
*Verification Invariant*: Heap allocation tracker confirms `s3`, `s2`, and `s1` are completely reclaimed upon break execution. Net allocations = 0.
