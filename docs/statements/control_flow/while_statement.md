# WhileStatement

## 1. Overview & Purpose

A `WhileStatement` implements pre-test repetitive execution. Before every iteration, the condition is evaluated. If `true`, the loop body executes; if `false`, execution exits the loop.

The loop body supports `break` (to exit immediately) and `continue` (to jump back to the condition check). Any reference variables declared inside the loop body are cleaned up via ARC at the end of each iteration.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
int32 i = 0;
while (i < 3) {
    i++;
}
```

```bytecode
// Compiled VM Bytecode
PUSH_CONST_I32 0
SET_LOCAL 1                 // i = 0

// --- Loop Start (<loop_start_ip>) ---
GET_LOCAL 1                 // Load i
PUSH_CONST_I32 3
LESS_I64                    // i < 3
JUMP_IF_FALSE <loop_exit_ip>// Exit loop if condition is false

// --- Loop Body ---
GET_LOCAL 1
INC_I64
SET_LOCAL 1                 // i++
JUMP <loop_start_ip>        // Loop back to condition

// --- Loop Exit (<loop_exit_ip>) ---
```

---

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [WhileStatement in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#whilestatement).
