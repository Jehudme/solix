# IfStatement (`NodeType::IF_STMT`)

## 1. Description & Purpose

The `if` statement provides conditional branching based on the evaluation of a boolean predicate expression. If the condition evaluates to `true`, the `then` branch executes; otherwise, control transfers to an optional `else` branch (which may chain into additional `else if` conditions). The compiler generates conditional jump instructions (`JMP_FALSE` / `JMP_IF_ZERO`) to redirect instruction pointer flow dynamically.

## 2. Syntax & Grammar

```solix
'if' '(' <condition-expr> ')' <then-statement> ['else' <else-statement>]
```

## 3. Underlying Systems & Mechanics

- Evaluates condition: must evaluate strictly to `bool`.
- Emits conditional jump `JUMP_IF_FALSE` to the `else` label or exit label.
- Any reference variables declared inside the `then` branch have their ARC cleanups injected before jumping to exit.
- `else` branch followed by exit label.

## 4. Positive Test Scenarios (Valid Variations)

1. **Single Branch**: `if (is_valid) { execute(); }`
2. **Two-Way Branch**: `if (flag) { do_a(); } else { do_b(); }`
3. **Multi-Way Else-If Chain**:
   ```solix
   if (score >= 90) { grade = 'A'; }
   else if (score >= 80) { grade = 'B'; }
   else { grade = 'C'; }
   ```

## 5. Negative Test Scenarios (Invalid Variations)

1. **Condition Is Not Boolean**:
   - `if (1) { ... }`  
     *Error*: `Condition must be bool`
   - `if (new Object()) { ... }`  
     *Error*: `Condition must be bool`
2. **Missing Parentheses**:
   - `if x > 0 { ... }`  
     *Error*: `Expected '(' after 'if'`
