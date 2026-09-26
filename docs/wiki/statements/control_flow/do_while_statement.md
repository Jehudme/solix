# DoWhileStatement

## 1. Overview & Purpose

A `DoWhileStatement` implements post-test iteration. Unlike `while`, the body executes at least once before the condition is evaluated. If the condition evaluates to `true`, control loops back to the start of the body; if `false`, the loop terminates.

`break` jumps past the condition check to the loop exit, while `continue` jumps directly to the condition evaluation at the foot of the loop.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
int32 count = 0;
do {
    count++;
} while (count < 3);
```

```bytecode
// Compiled VM Bytecode
PUSH_CONST_I32 0
SET_LOCAL 1                 // count = 0

// --- Loop Start (<loop_head_ip>) ---
GET_LOCAL 1
INC_I64
SET_LOCAL 1                 // count++

// --- Condition Evaluation (<condition_ip>) ---
GET_LOCAL 1
PUSH_CONST_I32 3
LESS_I64                    // count < 3
JUMP_IF_TRUE <loop_head_ip> // Loop back if true

// --- Loop Exit ---
```
