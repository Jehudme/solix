# DoWhileStatement (`NodeType::DO_WHILE_STMT`)

## 1. Description & Purpose

The `do-while` statement implements post-test repetitive execution. Unlike `while`, the loop body is guaranteed to execute at least once before the condition predicate is evaluated. At the conclusion of each iteration, the condition is checked; if `true`, execution jumps back to the loop header. `break` and `continue` statements within the body correctly respect this control flow topology.

## 2. Syntax & Grammar

```solix
'do' <statement> 'while' '(' <condition-expr> ')' ';'
```

## 3. Underlying Systems & Mechanics

- Emits loop body first without initial condition check.
- Emits condition check at bottom.
- Evaluates condition (`bool` required).
- Emits `JUMP_IF_TRUE` back to body start.
- Post-loop exit label.

## 4. Positive Test Scenarios (Valid Variations)

1. **Guaranteed Initial Iteration**:
   ```solix
   do {
       step();
   } while (should_continue());
   ```

## 5. Negative Test Scenarios (Invalid Variations)

1. **Missing `while` Keyword**:
   - `do { step(); }`  
     *Error*: `Expected 'while' after do body`
2. **Missing Trailing Semicolon**:
   - `do { step(); } while (flag)`  
     *Error*: `Expected ';' after do-while condition`
3. **Condition Is Not Boolean**:
   - `do { ... } while ("yes");`  
     *Error*: `Condition must be bool`
