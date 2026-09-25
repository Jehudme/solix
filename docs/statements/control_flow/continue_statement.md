# ContinueStatement (`NodeType::CONTINUE_STMT`)

## 1. Description & Purpose

The `continue` statement abandons the remainder of the current loop iteration and immediately transfers control to the loop's continuation point (the condition check in `while`/`do-while`, or the step/update expression in `for`). Similar to `break`, any local reference variables declared inside the loop body prior to the `continue` statement are properly decremented and cleaned up via ARC before the jump occurs.

## 2. Syntax & Grammar

```solix
'continue' ';'
```

## 3. Underlying Systems & Mechanics

- Scans up AST for the innermost enclosing `for`, `while`, or `do-while`.
- Emits ARC `DEC_REF` cleanups for local variables active in the scopes within the current loop iteration.
- Emits unconditional `JUMP` to the step/condition evaluation label of the loop.

## 4. Positive Test Scenarios (Valid Variations)

1. **Skipping Iteration in For Loop**: `for (int32 i = 0; i < 10; i++) { if (skip[i]) continue; }`
2. **Skipping Iteration in While Loop**: `while (read()) { if (invalid) continue; }`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Continue Outside Loop**:
   - `void test() { continue; }`  
     *Error*: `Continue statement outside of loop`
2. **Continue Inside Switch (Without Outer Loop)**:
   - `switch (x) { case 1: continue; }`  
     *Error*: `Continue statement outside of loop`
