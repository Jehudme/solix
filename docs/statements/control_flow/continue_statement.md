# ContinueStatement

## 1. Overview & Purpose

A `ContinueStatement` (`continue;`) skips the remainder of the current loop iteration and immediately jumps to the loop's continuation point (the condition check in `while`/`do-while`, or the step expression in `for`).

Intermediate reference variables declared inside the loop body prior to the `continue` statement are cleaned up via `DEC_REF` before the jump occurs.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
while (i < 5) {
    String s = new String("skip");
    i++;
    continue; // Jump to condition
}
```

```bytecode
// Compiled VM Bytecode
// --- Continue Execution ---
GET_LOCAL 2                 // Clean up s
DEC_REF
JUMP <loop_condition_ip>    // Jump to condition check
```

---

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [ContinueStatement in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#continuestatement).
