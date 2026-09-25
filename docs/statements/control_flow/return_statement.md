# §21 ReturnStatement

## 1. Overview & Scope

A `ReturnStatement` terminates execution of the current subroutine (method, function, constructor, or lambda), unwinds all active lexical scopes within the activation frame, decrements the subroutine's parameters and receiver object (`this`), and transfers control back to the caller frame with an optional return value.

In Solix's Automatic Reference Counting (ARC) architecture, returning from a subroutine is a coordinated, non-local transfer of control. Rather than simply resetting the stack pointer, the compiler orchestrates a systematic lexical scope traversal that decrements all active reference variables in reverse declaration order across all enclosing blocks, cleans up the function activation frame, and preserves the return value on the operand stack before emitting the VM `RETURN` opcode.

### Syntactic Placement
A `ReturnStatement` is legally permitted only inside subroutine bodies (methods, functions, constructors). It is **strictly prohibited** at global package scope and within class or interface member declaration blocks outside of methods.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
ReturnStatement   ::= 'return' Expression? ';'
```

### Canonical Code Patterns
```solix
// 1. Returning a value from non-void method
int32 get_count() {
    return 42;
}

// 2. Early return from void method
void process_item(bool is_valid) {
    if (!is_valid) {
        return; // Early exit
    }
    execute_work();
}

// 3. Returning heap-allocated reference object
String format_name(String first, String last) {
    String full = first + " " + last;
    return full; // Transferred to caller
}
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Return Expression Scope & Reachability
- The optional operand expression following `return` is evaluated within the immediate lexical scope of the return statement.
- **Unreachable Code Invariant**: A `ReturnStatement` completes abruptly and unconditionally. Any statement placed immediately following a `ReturnStatement` within the same sequential statement block is statically unreachable and triggers a dead-code warning or error.

### 3.2 Return Type Conformance
- If a method declares a non-void return type `T`, every return statement inside that method must provide an expression whose evaluated type is assignable to `T` (`is_assignable`).
- If a method declares a `void` return type, returning an expression is prohibited.
- In constructors, `return;` is permitted for early exit, but returning an expression is strictly prohibited.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Normal Completion
- A `ReturnStatement` **never completes normally**. Its evaluation always results in an abrupt completion.

### 4.2 Abrupt Completion
Execution of a `ReturnStatement` proceeds through the following operational steps:
1. **Return Value Evaluation**:
   - If an expression is present, it is evaluated to completion, leaving the resulting value on top of the VM operand stack.
2. **Lexical Scope Unwinding**:
   - The compiler traverses the AST parent chain from the `ReturnStatement` up to the containing `MethodDeclaration` or `ConstructorDeclaration`.
   - For every `BlockStatement` encountered along this path, the compiler emits inline `DEC_REF` cleanups for all local reference variables declared in that block in **reverse declaration order**.
3. **Function-Level Frame Cleanup**:
   - Once all intermediate blocks are unwound, `emit_cleanup_for_function` executes:
     - If the method is an instance method, slot 0 (the implicit `this` pointer) is decremented via `GET_LOCAL 0` + `OpCode::DEC_REF`.
     - For every formal parameter where `is_reference_type == true`, its slot is decremented via `GET_LOCAL <param_slot>` + `OpCode::DEC_REF`.
4. **Activation Record Teardown**:
   - The VM executes `OpCode::RETURN`.
   - The current activation frame is popped from the call stack.
   - The caller's instruction pointer `ip` is restored.
   - The caller reads the return value (if non-void) from the top of the operand stack.

### 4.3 Exception Unwinding & Trampolines
- If the evaluation of the return value expression throws an exception (e.g. `return risky_call();`), the return statement is aborted before frame cleanup begins.
- Control transfers immediately to the exception unwinding trampoline of the enclosing block.

---

## 5. Memory Model & ARC Invariants

### 5.1 Return Value Preservation Invariant
- When returning an object reference from a subroutine, the return value resides on the operand stack.
- Crucially, the returned object's reference count is maintained such that the destruction of intermediate local variables and parameters does not deallocate the returned instance before the caller receives it.

### 5.2 Receiver (`this`) and Parameter Cleanup Flow
```text
[ Evaluate Return Expression ] ──► Pushes Return Value to Stack Top
              │
              ▼
[ AST Parent Traversal ]       ──► For each intermediate BlockStatement:
                                   Emits GET_LOCAL + DEC_REF in reverse order
              │
              ▼
[ Function Frame Cleanup ]     ──► Emits GET_LOCAL 0 + DEC_REF (this pointer)
                                   Emits GET_LOCAL <slot> + DEC_REF for ref params
              │
              ▼
[ OpCode::RETURN ]             ──► Pops activation frame from call stack
                                   Caller consumes return value from stack top
```

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Missing Value in Non-Void Method
Returning without a value from a method declared with a non-void return type is illegal.
```solix
int32 compute() {
    return; // Error: must return a value
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Must return a value from non-void method
```

### Rule 6.2: Type Incompatible Return Value
Returning an expression whose type is not assignable to the method's declared return type.
```solix
int32 get_age() {
    return "twenty"; // Error: String cannot be converted to int32
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Return type mismatch: expected 'int32', got 'String'
```

### Rule 6.3: Returning Value from Void Method
Attempting to return an expression from a method with return type `void`.
```solix
void log_event() {
    return 100; // Error: cannot return value from void method
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Return type mismatch: expected 'void', got 'int32'
```

### Rule 6.4: Return at Global Package Scope
Using a `return` statement outside of any function or method body is prohibited.
```solix
package my_app;

int32 x = 10;
return; // Error: return at global scope
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Return statement not allowed outside of function or method body
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Operand Stack Underflow on Return
If internal VM bytecode corruption causes `OpCode::RETURN` to execute when a non-void return value was expected but the stack is empty, the VM raises an unrecoverable stack underflow panic.
*Runtime Fault*:
```text
[FATAL VM PANIC] StackUnderflowException: Attempted to pop return value from empty operand stack
```

---

## 8. Conformance & Verification Examples

### Example 8.1: Early Return from Nested Matrix Loops
```solix
// Conformance Test: Early return must unwind all intermediate blocks
int32 search_matrix(int32[][] grid, int32 target) {
    for (int32 r = 0; r < 5; r++) {
        String row_tag = new String("row_active");
        for (int32 c = 0; c < 5; c++) {
            String cell_tag = new String("cell_active");
            if (grid[r][c] == target) {
                return grid[r][c]; // Must decrement cell_tag and row_tag!
            }
        }
    }
    return -1;
}
```
*Verification Invariant*: Calling `search_matrix` when target matches immediately unwinds and decrements both `cell_tag` and `row_tag`. Zero heap memory is leaked.

### Example 8.2: Returning Reference Without Premature Deallocation
```solix
String build_message() {
    String msg = new String("Success");
    return msg; // Retains ref_count == 1 in caller frame
}

void verify_receiver() {
    String received = build_message();
    Console.println(received); // Prints: "Success"
}
```
*Verification Invariant*: `received` retains a valid heap reference. The object is not destroyed during the return sequence.
