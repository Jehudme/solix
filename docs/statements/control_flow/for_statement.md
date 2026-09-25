# ForStatement (`NodeType::FOR_STMT`)

## 1. Description & Purpose

The `for` statement provides structured iteration through three distinct control clauses: an initialization statement (which creates a scoped induction variable), a termination condition predicate, and an iteration/update expression executed after each loop body pass. The initialization scope encapsulates the loop variable so it does not leak into the enclosing scope, and ARC automatically disposes of loop-scoped references upon loop termination.

## 2. Syntax & Grammar

```solix
'for' '(' [<init-stmt>] ';' [<condition-expr>] ';' [<step-expr>] ')' <statement>
```

## 3. Underlying Systems & Mechanics

- Pushes for-loop lexical scope.
- Compiles `init-stmt` (e.g. `int32 i = 0`).
- Emits loop start label.
- Compiles `condition-expr` (defaults to `true` if omitted). Emits `JUMP_IF_FALSE` to exit.
- Compiles body.
- Compiles `step-expr` (target for `continue`).
- Emits `JUMP` to condition check.
- Emits exit label; cleans up loop-scoped variables (like `i`).

## 4. Positive Test Scenarios (Valid Variations)

1. **Standard 3-Clause Loop**: `for (int32 i = 0; i < len; i++) { ... }`
2. **Omitted Clauses**:
   - `for (;;) { if (done) break; }`
3. **Multiple Step Expressions**: `for (int32 i = 0; i < 10; i++, j--) { ... }`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Condition Is Not Boolean**:
   - `for (int32 i = 0; i; i++)`  
     *Error*: `Condition must be bool`
2. **Loop Variable Leaking to Outer Scope**:
   - `for (int32 i = 0; i < 10; i++) {} i = 5;`  
     *Error*: `Undefined identifier: i`
