# §23 TryCatchFinallyStatement

## 1. Overview & Scope

A `TryCatchFinallyStatement` provides structured exception recovery, fault isolation, and deterministic resource finalization in Solix. It encloses an execution domain (`try`) whose abnormal terminations are intercepted and evaluated against typed exception handlers (`catch`), and guarantees the execution of an optional cleanup sequence (`finally`).

In Solix's Automatic Reference Counting (ARC) architecture, exception handling is fully integrated with the lexical scoping engine. Catch clauses receive ownership of exception references and safely deallocate them upon handler completion, while the `finally` block executes unconditionally even in the presence of non-local control transfers (`return`, `break`, `continue`) or unhandled exceptions.

### Syntactic Placement
A `TryCatchFinallyStatement` is legally permitted within any subroutine body (methods, functions, constructors) and may be nested arbitrarily inside other blocks, loops, or exception handlers.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
TryStatement          ::= 'try' BlockStatement (CatchClause+ FinallyClause? | FinallyClause)
CatchClause           ::= 'catch' '(' TypeSpecifier Identifier ')' BlockStatement
FinallyClause         ::= 'finally' BlockStatement
```

### Canonical Code Patterns
```solix
// 1. Basic Try-Catch Recovery
try {
    risky_operation();
} catch (std.Exception e) {
    Console.println(e.getMessage());
}

// 2. Multi-Catch Polymorphic Hierarchy
try {
    read_network_packet();
} catch (std.SocketTimeoutException e) {
    retry_connection();
} catch (std.IOException e) {
    log_io_failure(e);
} catch (std.Exception e) {
    log_general_error(e);
}

// 3. Try-Catch-Finally Resource Finalization
try {
    acquire_resource();
    use_resource();
} catch (std.Exception e) {
    handle_error(e);
} finally {
    release_resource(); // Guaranteed execution
}

// 4. Try-Finally (Guaranteed Cleanup without Local Catch)
try {
    lock();
    critical_section();
} finally {
    unlock(); // Executes even if critical_section throws or returns
}
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Try Block Declaration Space
- The `try` block establishes an isolated lexical declaration space (`BlockKind::TRY_BODY`).
- Variables declared inside the `try` block are strictly confined to that block and cannot be accessed inside any `catch` or `finally` blocks.

### 3.2 Catch Clause Declaration Space & Parameter Scope
- Each `CatchClause` declares exactly one exception parameter: `catch (TypeSpecifier Identifier)`.
- The exception parameter identifier is bound into the immediate scope of the catch block and allocated a dedicated frame register (`variable_memory_index`).
- The parameter is visible throughout the catch block and is deallocated via ARC upon catch block exit.
- Sibling catch clauses are independent declaration spaces; identical parameter names (e.g. `catch (A e)` and `catch (B e)`) do not conflict.

### 3.3 Static Catch Hierarchy & Ordering Invariant
- Solix evaluates catch clauses in top-to-bottom textual order.
- **Unreachable Catch Prohibition**: A catch clause declaring a superclass type (e.g. `std.Exception`) cannot appear *before* a catch clause declaring a subclass type (e.g. `std.IOException`). If a broader type precedes a narrower type, the second clause is statically unreachable, and compilation halts with an error.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Normal Completion
When no exception occurs:
1. The `try` block executes sequentially to normal completion.
2. The compiler emits an unconditional `JUMP` bypassing all `catch` clauses directly to the end of the construct (or to the `finally` block if present).
3. If a `finally` block is present, it is executed to normal completion.
4. Normal sequential execution resumes after the `TryCatchFinallyStatement`.

### 4.2 Abrupt Completion
A `TryCatchFinallyStatement` completes abruptly if:
- An exception occurs within the `try` block and is not caught by any matching catch clause.
- An exception occurs inside a `catch` block (or a caught exception is re-thrown).
- An exception occurs inside the `finally` block.
- A `return`, `break`, or `continue` statement executes within the `try`, `catch`, or `finally` blocks.

### 4.3 Exception Dispatch & Catch Matching Mechanics
When an exception is raised inside the `try` block:
1. **Protected Block Unwinding**: The `try` block's exception cleanup segment decrements all try-local reference variables and branches directly to the start of the catch matching table.
2. **Sequential Type Evaluation**:
   For each catch clause $C_1, C_2, \dots, C_k$:
   - The VM executes `OpCode::GET_EXCEPTION`, pushing the active exception reference.
   - The VM executes `OpCode::INSTANCEOF <target_vtable_id>`, checking if the exception conforms to the handler's type hierarchy.
   - `OpCode::JUMP_IF_FALSE`: If false, control branches to the evaluation of $C_{i+1}$.
   - If true (Match Found):
     1. The VM executes `OpCode::GET_EXCEPTION` and `OpCode::SET_LOCAL <param_index>`, binding the exception to the catch parameter.
     2. The VM executes `OpCode::CLEAR_EXCEPTION`, resetting the active exception state and returning the VM to normal execution mode.
     3. The catch block statements are executed.
     4. Upon catch block completion, the VM executes `GET_LOCAL <param_index>` followed by `OpCode::DEC_REF`, releasing the parameter reference.
     5. Control branches to the `finally` block (or construct exit).
3. **Unhandled Exception Fall-Through**:
   - If no catch clause matches, the final branch executes `OpCode::JMP_TO_OUTER_CLEANUP` (or executes `finally` before propagating upward), unwinding to the caller activation frame.

---

## 5. Memory Model & ARC Invariants

### 5.1 Catch Parameter ARC Lifecycle
- **Ownership Handshake**: When an exception matches a catch clause, its pointer is written to the parameter register slot via `SET_LOCAL`.
- **Guaranteed Decrement**: Upon completion of the catch block (whether through normal exit or early return), the compiler emits `GET_LOCAL` + `DEC_REF` for the parameter.
- This guarantees that caught exceptions are deterministically deallocated without relying on garbage collection passes.

### 5.2 Bytecode Lowering Flowchart
```text
+-------------------------------------------------------------------+
| 1. Try Body Statements (Normal Execution)                         |
+-------------------------------------------------------------------+
| 2. OpCode::JUMP <normal_exit_target>                              |
+===================================================================+
| 3. Catch Matching Table (Target of Try Block Unwinding)           |
|    Clause 1:                                                      |
|      OpCode::GET_EXCEPTION                                        |
|      OpCode::INSTANCEOF <vtable_id_1>                             |
|      OpCode::JUMP_IF_FALSE <clause_2_target>                      |
|      OpCode::GET_EXCEPTION                                        |
|      OpCode::SET_LOCAL <param_slot>                               |
|      OpCode::CLEAR_EXCEPTION                                      |
|      [Compile Clause 1 Body]                                      |
|      OpCode::GET_LOCAL <param_slot>                               |
|      OpCode::DEC_REF                                              |
|      OpCode::JUMP <finally_target>                                |
|    Clause 2: ...                                                  |
|    If No Match: OpCode::JMP_TO_OUTER_CLEANUP                      |
+===================================================================+
| 4. Finally Block Statements (Executed Unconditionally)            |
+-------------------------------------------------------------------+
| 5. <normal_exit_target>: Normal Continuation                      |
+-------------------------------------------------------------------+
```

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Try Construct Without Handlers
A `try` block must be accompanied by at least one `catch` clause or a `finally` block.
```solix
void test_missing_handler() {
    try {
        int32 x = 1;
    } // Error: missing catch or finally
}
```
*Diagnostic Message*:
```text
[ERROR] parser.cpp: Syntax error: 'try' statement must have at least one 'catch' or 'finally' block
```

### Rule 6.2: Catch Parameter Type Non-Exception Rejection
Catch parameters must declare a type that extends `std.Exception`.
```solix
class NotAnException {}

void test_invalid_catch_type() {
    try {
        run();
    } catch (NotAnException e) { // Illegal catch type
    }
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Catch parameter type 'NotAnException' must inherit from 'std.Exception'
```

### Rule 6.3: Unreachable Catch Clause Detection
Declaring a catch clause for a base class prior to a derived class clause is illegal.
```solix
class SubException extends std.Exception { SubException() : super("Sub") {} }

void test_unreachable() {
    try {
        run();
    } catch (std.Exception e) {
        Console.println("Base");
    } catch (SubException s) { // Unreachable!
        Console.println("Sub");
    }
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Unreachable catch clause: 'SubException' is already handled by preceding catch for 'std.Exception'
```

### Rule 6.4: Accessing Try-Local Variable from Catch or Finally
Variables declared inside the `try` block are out of scope in `catch` and `finally` blocks.
```solix
void test_scope_leak() {
    try {
        int32 secret = 42;
    } catch (std.Exception e) {
        Console.println(secret); // Illegal: secret is out of scope
    }
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Undefined identifier: secret
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Secondary Exception Inside Catch Block
If an exception is raised while executing inside a catch block, the original exception is superseded, and the VM begins unwinding for the secondary exception.
```solix
void test_catch_fault() {
    try {
        throw new std.Exception("First");
    } catch (std.Exception e) {
        throw new std.Exception("Secondary"); // Replaces unwinding exception
    }
}
```
*Runtime Fault*:
```text
[FATAL VM PANIC] Unhandled Exception: 'Secondary'
    at test_catch_fault() in test.slx:line 5
```

---

## 8. Conformance & Verification Examples

### Example 8.1: Polymorphic Subclass Match
```solix
// Conformance Test: Most specific catch clause must capture subclass
class CustomA extends std.Exception { CustomA() : super("A") {} }
class CustomB extends std.Exception { CustomB() : super("B") {} }

int32 result = 0;
try {
    throw new CustomB();
} catch (CustomA a) {
    result = 1;
} catch (CustomB b) {
    result = 2; // Must match here
} catch (std.Exception e) {
    result = 3;
}
```
*Verification Invariant*: `result` equals 2. Catch parameter `b` is cleaned up via `DEC_REF` upon exit; zero memory leaks.

### Example 8.2: Unconditional Finally Execution on Early Return
```solix
bool finally_executed = false;

int32 verify_finally_return() {
    try {
        return 42; // Initiates return
    } finally {
        finally_executed = true; // Must execute prior to caller return
    }
}
```
*Verification Invariant*: `verify_finally_return()` returns 42 and sets `finally_executed` to `true`.
