# §22 ThrowStatement

## 1. Overview & Scope

A `ThrowStatement` initiates structured exception propagation in Solix. When executed, it abruptly terminates normal sequential instruction flow, transitions the Virtual Machine into the **Exception Unwinding State**, and transfers control to the enclosing block's exception cleanup trampoline to search for a matching `try-catch` recovery handler.

In Solix's memory-safe architecture, throwing an exception is not an uncontrolled fatal trap. It is a strictly typed, synchronous control-flow transfer that guarantees all active reference variables in intermediate lexical scopes are deterministically decremented and reclaimed via Automatic Reference Counting (ARC) before the exception is accepted by a handler.

### Syntactic Placement
A `ThrowStatement` is legally permitted inside any executable subroutine body (methods, functions, constructors, and lambdas). It may appear inside standard lexical blocks, loop bodies, conditional branches, or within `catch` blocks (enabling exception re-throwing).

---

## 2. Syntax & Production Rules

### Production Rules
```solix
ThrowStatement   ::= 'throw' Expression ';'
```

### Canonical Code Patterns
```solix
// 1. Throwing standard base Exception
throw new std.Exception("Operation failed");

// 2. Throwing domain-specific custom Exception
throw new DatabaseException("Connection timeout", 504);

// 3. Exception Re-Throwing within Catch Handler
try {
    process();
} catch (std.Exception err) {
    log_error(err);
    throw err; // Re-throws active exception instance
}
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Expression Scope & Reachability
- The operand expression following the `throw` keyword is evaluated in the immediate lexical scope of the throw statement.
- **Unreachable Code Invariant**: A `ThrowStatement` completes abruptly and unconditionally. Any statement occurring immediately after a `ThrowStatement` within the same sequential statement sequence is statically unreachable and triggers a dead-code warning or error.

### 3.2 Type Conformance (Static Hierarchy Constraint)
- The static type of the operand expression must resolve to a reference class that inherits directly or indirectly from `std.Exception` (or `std.Throwable`).
- Throwing primitive types (e.g. `throw 404;`) or unrelated class instances (e.g. `throw new String("err");`) is strictly rejected during semantic binding (`binder.cpp`).

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Normal Completion
- A `ThrowStatement` **never completes normally**. Its evaluation always results in an abrupt completion.

### 4.2 Abrupt Completion
Execution of a `ThrowStatement` proceeds through the following operational steps:
1. **Operand Evaluation**: The operand expression is evaluated to completion, pushing an object reference pointer onto the VM operand stack.
2. **Null Reference Guard**: If the evaluated reference is `null` (`0x0`), the VM immediately halts and raises a runtime `NullReferenceException`.
3. **Active Exception Registration**:
   - The VM executes `OpCode::THROW_EXCEPTION`.
   - The exception object pointer is popped from the operand stack and stored into the VM's dedicated `active_exception` register.
4. **Immediate Trampoline Branch**:
   - Following `OpCode::THROW_EXCEPTION`, the bytecode contains a 32-bit forward jump offset.
   - The VM sets its instruction pointer `ip` directly to this target, which points to the containing `BlockStatement`'s **Exception Cleanup Segment**.

### 4.3 Exception Unwinding & Trampolines
1. **Current Block Cleanup**:
   - The containing block's exception cleanup segment executes, issuing reverse-order `DEC_REF` instructions for all local reference variables declared prior to the throw statement.
2. **Next-Hop Dispatch**:
   - If the block is enclosed by a `TryStatement` (`BlockKind::TRY_BODY`), control branches immediately into the catch matching table.
   - If the block is a nested standard block (`BlockKind::NORMAL`), control branches to the parent block's cleanup segment.
   - If the block is a method root (`BlockKind::FUNCTION_BODY`), the segment emits `OpCode::JMP_TO_OUTER_CLEANUP`.
3. **Inter-Frame Unwinding**:
   - When `OpCode::JMP_TO_OUTER_CLEANUP` executes, the VM pops the current activation record, restores caller registers, and continues unwinding the caller frame until a matching catch handler is found or the call stack is exhausted.

---

## 5. Memory Model & ARC Invariants

### 5.1 Active Exception Ownership
- The `active_exception` register in the VM acts as an owning reference root.
- When an object reference is transferred into `active_exception`, its reference count remains positive (`ref_count >= 1`).
- When a matching catch block captures the exception, the object is stored into the catch parameter (`SET_LOCAL`), and `CLEAR_EXCEPTION` clears the register.

### 5.2 Bytecode Lowering & Patch Invariant
The compiler emits a forward-referenced placeholder during throw generation:
```cpp
void Assembler::visit(ThrowStatement& n) {
    if (n.exception_expression) {
        compile_expression(n.exception_expression.get());
    }
    emit_byte(static_cast<uint8_t>(OpCode::THROW_EXCEPTION));
    if (!exception_cleanup_patches.empty()) {
        exception_cleanup_patches.back().push_back(bytecode().size());
    }
    emit_int32(0xFFFFFFFF); // Patched by containing BlockStatement
}
```
The enclosing `BlockStatement` patches `0xFFFFFFFF` to point directly to its `cleanup_ip`.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Non-Exception Type Rejection
The operand of a throw statement must inherit from `std.Exception`.
```solix
void test_invalid_type() {
    throw 500; // Primitive cannot be thrown
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Cannot throw type 'int32': must inherit from 'std.Exception'
```

### Rule 6.2: Unrelated Class Reference Rejection
Attempting to throw a class instance that does not extend `std.Exception`.
```solix
class Widget {}

void test_unrelated() {
    throw new Widget();
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Cannot throw type 'Widget': must inherit from 'std.Exception'
```

### Rule 6.3: Missing Expression (Bare Throw)
Bare `throw;` without an expression operand is illegal in Solix.
```solix
void test_bare() {
    throw;
}
```
*Diagnostic Message*:
```text
[ERROR] parser.cpp: Syntax error: expected expression after 'throw'
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Throwing a Null Object Reference
Attempting to throw an expression that evaluates to `null` triggers an immediate runtime panic.
```solix
void test_null_throw() {
    std.Exception err = null;
    throw err;
}
```
*Runtime Fault*:
```text
[FATAL VM PANIC] NullReferenceException: Attempted to throw null exception reference
```

### Fault 7.2: Uncaught Exception Call Stack Exhaustion
If an exception propagates past the root activation frame of `main` without being handled by any catch clause, the VM terminates.
```solix
void main() {
    throw new std.Exception("Fatal root error");
}
```
*Runtime Fault*:
```text
[FATAL VM PANIC] Unhandled Exception: 'Fatal root error'
    at main() in test.slx:line 2
Process terminated with exit code 1
```

---

## 8. Conformance & Verification Examples

### Example 8.1: Pre-Throw Local Resource Deallocation
```solix
// Conformance Test: Variables declared prior to throw must be deallocated
void verify_throw_cleanup() {
    try {
        String pre = new String("must_be_freed");
        throw new std.Exception("abort");
        String unreachable = new String("never_allocated");
    } catch (std.Exception e) {
        // 'pre' must be completely deallocated before catch body begins
    }
}
```
*Verification Invariant*: Heap allocation counter verifies `pre` is deallocated by the block's exception cleanup segment before entering the catch handler.

### Example 8.2: Exception Re-Throwing Integrity
```solix
void verify_rethrow() {
    try {
        try {
            throw new std.Exception("original_cause");
        } catch (std.Exception e) {
            throw e; // Valid re-throw
        }
    } catch (std.Exception outer) {
        Console.println(outer.getMessage()); // Prints: "original_cause"
    }
}
```
*Verification Invariant*: The outer catch receives the identical exception instance. Refcounts remain balanced with zero memory leaks.
