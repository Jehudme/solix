# ThrowStatement (`NodeType::THROW_STMT`)

## 1. Description, Purpose & Architectural Implementation

### Conceptual Overview
The `ThrowStatement` initiates structured exception propagation in Solix. When executed, it abruptly interrupts normal sequential control flow, stores an exception object reference into the virtual machine's active exception register, and begins unwinding the call stack until an appropriate `try-catch` handler is encountered.

In Solix, exception handling is strictly integrated with the Automatic Reference Counting (ARC) memory engine. A throw statement is not an uncontrolled jump—it is a meticulously coordinated event that guarantees all local variables in active scopes between the throw site and the catch handler are decremented and reclaimed, preventing memory leaks during abnormal termination.

---

### Key Conceptual Roles

#### 1. Abrupt Control Flow Disruption & Exception Raising
The primary role of the `ThrowStatement` is to signal an unrecoverable condition or domain-specific error that cannot be resolved at the immediate site of occurrence. Upon reaching `throw`, normal instruction sequencing terminates, and the runtime enters the **Exception Unwinding State**.

#### 2. Strict Class Hierarchy Type Safety
Solix strictly mandates that the thrown operand must evaluate to a reference type that inherits directly or indirectly from the standard base class `std.Exception` (or `std.Throwable`).
- Throwing primitive values (such as `throw 404;` or `throw true;`) is strictly forbidden by the compiler.
- Throwing unrelated reference types (such as `throw new String("error");`) is also prohibited.
- This uniform type hierarchy guarantees that catch clauses can safely inspect exception properties (such as error messages and stack traces) via polymorphism.

#### 3. Integration with Block Exception Cleanup Trampolines
The critical architectural link between `throw` and ARC is the **Exception Cleanup Trampoline**:
- When a `throw` statement is compiled, the compiler does not immediately jump to an unknown catch block.
- Instead, it emits a jump instruction whose target is patched to point directly to the containing `BlockStatement`'s **Exception Cleanup Segment**.
- This guarantees that any local variables allocated in the current block *prior* to the throw statement are properly decremented via `DEC_REF` before control leaves the block.

#### 4. Inter-Frame Stack Unwinding
If no enclosing `try-catch` block exists within the current function body:
- The block's cleanup segment terminates with `OpCode::JMP_TO_OUTER_CLEANUP`.
- The VM unwinds the current activation record, pops the frame from the call stack, decrements the function's arguments and `this` pointer, and continues searching for handlers in the caller's activation frame.
- If unwinding climbs to the entry point (`main`) without finding a matching catch handler, the VM prints the unhandled exception message and terminates the process with a non-zero exit code.

---

### How ThrowStatement Was Implemented in Solix

#### 1. Abstract Syntax Tree Representation (`statements.hpp`)
In the Solix AST, a throw statement contains a single child expression:
```cpp
struct ThrowStatement : public Node {
    std::unique_ptr<Node> exception_expression;

    ThrowStatement(const Token& t, std::unique_ptr<Node> expr)
        : Node(NodeType::THROW_STMT, t), exception_expression(std::move(expr)) {
        if (exception_expression) exception_expression->parent = this;
    }
};
```

#### 2. Semantic Analysis & Type Validation (`binder.cpp`)
During semantic analysis (`BinderPass::BIND_EXECUTION`):
1. **Expression Evaluation**: The binder evaluates the thrown expression:
   ```cpp
   TypeInfo expr_type = evaluate_expression(n.exception_expression.get());
   ```
2. **Hierarchy Verification**: The binder resolves `std.Exception` and verifies assignability:
   ```cpp
   TypeInfo exception_base{"std.Exception"};
   if (!is_assignable(exception_base, expr_type)) {
       record_error(&n, "Cannot throw type '" + expr_type.name +
                         "': must inherit from 'std.Exception'");
   }
   ```
3. If no expression is provided (e.g. bare `throw;`), a parser syntax error is generated.

#### 3. Bytecode Generation & Cleanup Patching (`assembler.cpp`)
The code generator translates `ThrowStatement` into an evaluation step, an opcode emission, and a forward jump registration:
```cpp
void Assembler::visit(ThrowStatement& n) {
    if (n.exception_expression) {
        compile_expression(n.exception_expression.get());
    }
    emit_byte(static_cast<uint8_t>(OpCode::THROW_EXCEPTION));
    if (!exception_cleanup_patches.empty()) {
        exception_cleanup_patches.back().push_back(bytecode().size());
    }
    emit_int32(0xFFFFFFFF); // 4-byte jump target placeholder
}
```
- **Forward Patch Registration**: The 4-byte placeholder (`0xFFFFFFFF`) is appended to `exception_cleanup_patches.back()`.
- When the enclosing `BlockStatement` finishes compiling its children, it resolves this placeholder to point directly to its `cleanup_ip`.

#### 4. VM Runtime Execution (`vm.cpp`)
When the VM executes `OpCode::THROW_EXCEPTION`:
1. It pops the operand from the top of the stack (the exception object reference).
2. It stores this reference into the VM's `active_exception` register.
3. It reads the 32-bit jump offset following the opcode.
4. It sets the instruction pointer `ip` to that offset, instantly redirecting execution into the block's cleanup segment.

---

## 2. Syntax & Grammar

### Syntax Forms
In Solix, a throw statement consists of the `throw` keyword followed by an expression yielding an exception object, terminated with a semicolon:

```solix
// 1. Throwing standard Exception
throw new std.Exception("An error occurred");

// 2. Throwing domain-specific custom Exception
throw new DatabaseConnectionException("Failed to connect to host", 500);

// 3. Re-throwing a caught Exception reference
try {
    perform_risky_task();
} catch (std.Exception err) {
    Console.println("Logging error before rethrow");
    throw err; // Re-throws active exception
}
```

---

## 3. Underlying Systems & VM Mechanics

### Exception Propagation Sequence
```text
+-----------------------------------------------------------------------+
| 1. Throw Expression Evaluated                                         |
|    - Evaluates 'new MyException()' onto stack                         |
+-----------------------------------------------------------------------+
| 2. OpCode::THROW_EXCEPTION                                            |
|    - Pops object ref and stores in VM active_exception                |
|    - Jumps to containing BlockStatement cleanup_ip                    |
+-----------------------------------------------------------------------+
| 3. Block Exception Cleanup Segment                                    |
|    - Emits DEC_REF for all active local references in reverse order   |
+-----------------------------------------------------------------------+
| 4. Next Hop Unwinding Jump                                            |
|    - Traverses up to TryStatement catch matching table                |
|    - If no catch matches -> OpCode::JMP_TO_OUTER_CLEANUP              |
+-----------------------------------------------------------------------+
| 5. VM Frame Unwinding                                                 |
|    - Caller frame resumes unwinding until top-level or handled        |
+-----------------------------------------------------------------------+
```

---

## 4. Positive Test Scenarios (Valid Variations)

### Scenario 4.1: Throw Caught by Immediate Try-Catch Block
Throwing an exception inside a try block that is immediately caught by a matching catch clause.
```solix
bool caught = false;
try {
    throw new std.Exception("test exception");
} catch (std.Exception e) {
    caught = true;
}
// 'caught' must be true, zero memory leaks
```
*Verification*: Catch block executes; `active_exception` is cleared; program continues normally.

### Scenario 4.2: Throwing Subclass Exception Handled by Base Catch
Throwing a derived exception type caught by a base `std.Exception` handler.
```solix
class CustomError extends std.Exception {
    int32 code;
    CustomError(int32 code) : super("Custom Error") {
        this.code = code;
    }
}

void test_subclass_throw() {
    try {
        throw new CustomError(404);
    } catch (std.Exception e) {
        Console.println("Caught base exception");
    }
}
```
*Verification*: `INSTANCEOF` check in catch dispatch evaluates to `true`; catch block successfully handles `CustomError`.

### Scenario 4.3: Local Variable Reclamation Prior to Unwinding
Local reference variables declared before the throw statement must be deallocated before the catch block executes.
```solix
void test_throw_cleanup() {
    try {
        String s1 = new String("pre-throw resource");
        throw new std.Exception("abort");
        String s2 = new String("unreachable"); // Never executed
    } catch (std.Exception e) {
        // s1 must have been decremented by the block's cleanup segment
    }
}
```
*Verification*: VM heap tracker verifies `s1` was destroyed. No memory is leaked.

### Scenario 4.4: Re-throwing an Exception Reference
A caught exception reference can be re-thrown out of the catch block to be handled by an outer try-catch.
```solix
void test_rethrow() {
    try {
        try {
            throw new std.Exception("nested error");
        } catch (std.Exception inner) {
            Console.println("Caught inner, rethrowing...");
            throw inner; // Re-throw
        }
    } catch (std.Exception outer) {
        Console.println("Caught outer successfully");
    }
}
```
*Verification*: Outer catch receives the exact same exception instance. Refcounts remain balanced.

---

## 5. Negative Test Scenarios (Invalid Variations)

### Scenario 5.1: Throwing Primitive Scalar Value
Attempting to throw an integer, float, or boolean must be rejected by the compiler.
```solix
void test_throw_primitive() {
    throw 500; // Error: cannot throw primitive type
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot throw type 'int32': must inherit from 'std.Exception'
```

### Scenario 5.2: Throwing Unrelated Class Reference
Attempting to throw an instance of a class that does not inherit from `std.Exception`.
```solix
class Widget {}

void test_throw_non_exception_class() {
    throw new Widget(); // Error: Widget is not an Exception
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot throw type 'Widget': must inherit from 'std.Exception'
```

### Scenario 5.3: Empty Bare Throw Statement
Solix does not support bare `throw;` without an expression operand.
```solix
void test_bare_throw() {
    throw; // Error: missing expression
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Syntax error: expected expression after 'throw'
```

### Scenario 5.4: Throwing Null Reference at Runtime
Throwing an uninitialized or null reference must trigger a runtime `NullReferenceException` during throw processing.
```solix
void test_throw_null() {
    std.Exception null_ex = null;
    throw null_ex; // Runtime error
}
```
*Expected Runtime Exception*:
```text
[FATAL VM PANIC] NullReferenceException: Attempted to throw null exception reference
```
