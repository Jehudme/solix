# TryCatchFinallyStatement (`NodeType::TRY_STMT`, `CATCH_CLAUSE`)

## 1. Description & Purpose

The `try-catch-finally` statement provides structured error handling and guaranteed resource reclamation. Code within the `try` block is executed under an active exception handler table. If an exception occurs, matching `catch` clauses are evaluated in top-to-bottom order against the runtime exception's class hierarchy. An optional `finally` block is guaranteed to execute regardless of whether an exception occurred, was caught, or if an early jump (`return`, `break`) was initiated.

## 2. Syntax & Grammar

```solix
'try' <block> ('catch' '(' <type-info> <identifier> ')' <block>)* ['finally' <block>]
```

## 3. Underlying Systems & Mechanics

- Compiles exception handler table entry with bytecode boundaries `[try_start, try_end]`.
- For each `catch` clause, registers handler address and target `vtable_id`.
- Handled at runtime via `THROW_EXCEPTION`, `GET_EXCEPTION`, and `CLEAR_EXCEPTION` opcodes.
- On exception, VM checks if the thrown exception conforms to target `vtable_id` (via vtable inheritance tree).
- `finally` block compiles into a trampoline: executed on normal fallthrough, on uncaught exceptions during unwinding, and before executing any `return` inside `try` or `catch` (via `REGISTER_RETURN_CLEANUP` and `JMP_TO_OUTER_CLEANUP`).

## 4. Positive Test Scenarios (Valid Variations)

1. **Standard Try-Catch**: `try { risky(); } catch (IOException e) { log(e); }`
2. **Multi-Catch Hierarchy**:
   ```solix
   try {
       work();
   } catch (NullPointerException e) {
       handle_null();
   } catch (Exception e) {
       handle_general();
   }
   ```
3. **Try-Catch-Finally**:
   ```solix
   try {
       open();
   } catch (Exception e) {
       recover();
   } finally {
       close(); // Guaranteed execution
   }
   ```

## 5. Negative Test Scenarios (Invalid Variations)

1. **Try Without Catch or Finally**:
   - `try { work(); }`  
     *Error*: `Try statement must have at least one catch clause or finally block`
2. **Catching Non-Exception Type**:
   - `catch (int32 code) { ... }`  
     *Error*: `Catch parameter must inherit from Exception`
3. **Shadowed Unreachable Catch Block**:
   - `catch (Exception e) { ... } catch (NullPointerException npe) { ... }`  
     *Error*: `Catch block for 'NullPointerException' is unreachable (already caught by 'Exception')`
