# WhileStatement (`NodeType::WHILE_STMT`)

## 1. Description & Purpose

The `while` statement implements pre-test repetitive execution. Before every iteration, the loop condition is evaluated. If the predicate evaluates to `true`, the loop body executes, followed by an unconditional jump back to the condition check. If the condition evaluates to `false`, control immediately bypasses the loop. Solix manages loop labels to support nested loops and correctly coordinate `break` and `continue` jumps.

## 2. Syntax & Grammar

```solix
'while' '(' <condition-expr> ')' <statement>
```

## 3. Underlying Systems & Mechanics

- Creates loop start label.
- Evaluates condition: must evaluate to `bool`.
- Emits `JUMP_IF_FALSE` to loop exit label.
- Loop body compiles with active loop context (for `break` and `continue` targets).
- Tail emits unconditional `JUMP` back to loop start.
- Loop exit label cleans up loop-level activation state.

## 4. Positive Test Scenarios (Valid Variations)

1. **Standard Counter Loop**: `while (i < 10) { i++; }`
2. **Loop with Embedded Break & Continue**:
   ```solix
   while (has_next()) {
       Item item = get_next();
       if (item.skip) continue; // item cleaned by ARC
       if (item.done) break;    // item cleaned by ARC
       process(item);
   }
   ```

## 5. Negative Test Scenarios (Invalid Variations)

1. **Condition Is Not Boolean**:
   - `while (100) { ... }`  
     *Error*: `Condition must be bool`
2. **Missing Condition Expression**:
   - `while () { ... }`  
     *Error*: `Expected expression in while condition`
