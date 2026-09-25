# TryCatchFinallyStatement (`NodeType::TRY_STMT`, `CATCH_CLAUSE`)

## 1. Description, Purpose & Architectural Implementation

### Conceptual Overview
The `TryCatchFinallyStatement` provides structured, deterministic exception handling, error recovery, and resource finalization in Solix. It encapsulates risky operations within a protected boundary (`try`), matches occurring runtime errors against typed handlers (`catch`), and guarantees the execution of cleanup routines (`finally`).

In Solix, exception handling is fundamentally intertwined with the Automatic Reference Counting (ARC) runtime. Rather than treating exceptions as an asynchronous trap, the Solix architecture treats them as synchronous control-flow transitions. The try-catch-finally construct coordinates with the lexical block unwinding trampolines, ensuring that all local variables, exception parameters, and frame allocations are properly cleaned up regardless of whether an exception occurs, is caught, or is re-thrown.

---

### Key Conceptual Roles

#### 1. Protected Execution Boundary (`try`)
The `try` block establishes a protected execution domain (`BlockKind::TRY_BODY`). Any exception raised by a `throw` statement, or any runtime fault triggered inside this block (or within functions invoked by this block), transfers control to the construct's catch dispatch table instead of aborting the program.

#### 2. Polymorphic Exception Dispatch (`catch`)
Solix supports multiple sequential `catch` clauses per try block. Each clause declares an exception parameter with a specific type:
- When an exception arrives at the catch matching table, the VM evaluates the runtime exception instance against the catch parameter's type using VTable hierarchy querying (`INSTANCEOF`).
- Clauses are evaluated in **top-to-bottom order**. The first catch clause whose type is an ancestor of (or identical to) the runtime exception's class will capture it.
- Once a match occurs, the exception object is stored in the catch clause's local variable slot, the VM's active exception register is cleared (`CLEAR_EXCEPTION`), and normal sequential execution resumes inside the catch body.
- When the catch body concludes, the catch parameter is automatically decremented (`DEC_REF`), preventing the exception object from leaking.

#### 3. Unconditional Resource Finalization (`finally`)
The optional `finally` block provides an absolute guarantee of execution:
- If the `try` block executes normally without errors, `finally` executes immediately afterward.
- If an exception is caught and handled by a `catch` block, `finally` executes after the catch body concludes.
- Even if an early transfer of control (`return`, `break`, `continue`) is initiated inside the `try` or `catch` blocks, the `finally` block is executed before control transfers to the ultimate destination.

#### 4. Unhandled Exception Propagation
If an exception occurs within the `try` block and none of the declared `catch` clauses match the exception's type:
- Control bypasses all catch bodies.
- The compiler emits an unconditional jump to the outer scope's exception cleanup segment (`JMP_TO_OUTER_CLEANUP` or outer patch jump).
- The exception continues unwinding enclosing blocks and caller stack frames until a matching handler is found.

---

### How TryCatchFinally Was Implemented in Solix

#### 1. Abstract Syntax Tree Representation (`statements.hpp`)
```cpp
struct CatchClause : public Node {
    TypeInfo type_info;
    std::string var_name;
    std::unique_ptr<Node> body;
    int32_t variable_memory_index = -1;
    int32_t target_vtable_id = -1;

    CatchClause(const Token& t, TypeInfo type, const std::string& name,
                std::unique_ptr<Node> b)
        : Node(NodeType::CATCH_CLAUSE, t), type_info(std::move(type)),
          var_name(name), body(std::move(b)) {
        if (body) body->parent = this;
    }
};

struct TryStatement : public Node {
    std::unique_ptr<Node> try_block;
    std::vector<std::unique_ptr<Node>> catch_clauses;
    std::unique_ptr<Node> finally_block;

    TryStatement(const Token& t, std::unique_ptr<Node> try_b,
                 std::vector<std::unique_ptr<Node>> catches,
                 std::unique_ptr<Node> finally_b = nullptr)
        : Node(NodeType::TRY_STMT, t), try_block(std::move(try_b)),
          catch_clauses(std::move(catches)), finally_block(std::move(finally_b)) {
        if (try_block) try_block->parent = this;
        for (auto& c : catch_clauses) if (c) c->parent = this;
        if (finally_block) finally_block->parent = this;
    }
};
```

#### 2. Semantic Analysis & Type Validation (`binder.cpp`)
During semantic analysis (`BinderPass::BIND_EXECUTION`):
1. **Protected Block Binding**: The binder visits `try_block` with its `block_kind` marked as `BlockKind::TRY_BODY`.
2. **Catch Clause Validation**:
   - The binder resolves each catch clause's `type_info`. It verifies that the type inherits from `std.Exception`.
   - It retrieves the class's unique `target_vtable_id` used for runtime polymorphism checks.
   - It allocates a local stack slot for the exception variable (`variable_memory_index = local_variable_index++`).
   - It binds the catch block body in a child scope containing the catch parameter.
3. **Unreachable Catch Detection**:
   - The binder tracks the hierarchy of catch types. If a catch clause for a base class (e.g. `std.Exception`) appears *before* a catch clause for a derived class (e.g. `std.IOException`), the compiler emits a compile-time diagnostic indicating the second catch clause is unreachable.

#### 3. Bytecode Emission & Dispatch Mechanics (`assembler.cpp`)
The code generator lays out the bytecode in sequential stages:
1. **Compile Protected Try Block**: Calls `compile_node(n.try_block.get())`.
2. **Emit Normal Exit Jump**: Emits `OpCode::JUMP <normal_exit_patch>` to skip over the catch clauses if no error occurs.
3. **Wire Unwinding Entry**: If an exception was thrown in the `try_block`, its exception cleanup segment jumps to the start of the catch matching table (`catch_start_ip`).
4. **Catch Matching Table**:
   For each catch clause:
   ```bytecode
   OpCode::GET_EXCEPTION                   // Push active exception onto operand stack
   OpCode::INSTANCEOF <target_vtable_id>   // Check if exception inherits from handler type
   OpCode::JUMP_IF_FALSE <next_catch_patch>// If false, jump to evaluate next catch clause
   OpCode::GET_EXCEPTION                   // Push exception again
   OpCode::SET_LOCAL <variable_memory_index> // Store into catch variable slot
   OpCode::CLEAR_EXCEPTION                 // Reset VM active exception state
   <compile catch_clause->body>            // Execute handler body
   OpCode::GET_LOCAL <variable_memory_index> // Push catch parameter
   OpCode::DEC_REF                         // Decrement catch parameter reference count
   OpCode::JUMP <end_try_patch>            // Jump to end of try construct
   ```
5. **Unhandled Fall-Through**:
   If none of the catch clauses match, the last false jump branches to:
   ```bytecode
   OpCode::JMP_TO_OUTER_CLEANUP            // Unwind to caller stack frame
   ```
6. **Patch End Targets & Finally Block**:
   - All successful completions (`normal_exit_patch` and `end_try_patches`) converge on `end_ip`.
   - If a `finally_block` is present, it is compiled immediately at `end_ip`.

---

## 2. Syntax & Grammar

### Syntax Forms
Solix requires a `try` block to be followed by at least one `catch` clause or a `finally` block (or both):

```solix
// 1. Standard Try-Catch
try {
    dangerous_operation();
} catch (std.Exception e) {
    Console.println(e.getMessage());
}

// 2. Multi-Catch Hierarchy
try {
    process_file("data.txt");
} catch (std.FileNotFoundException e) {
    Console.println("File not found");
} catch (std.IOException e) {
    Console.println("IO failure");
} catch (std.Exception e) {
    Console.println("Generic error");
}

// 3. Try-Catch-Finally
try {
    FileStream stream = new FileStream("data.bin");
    stream.read();
} catch (std.Exception e) {
    Console.println("Error reading stream");
} finally {
    Console.println("Cleanup executed unconditionally");
}

// 4. Try-Finally (No catch)
try {
    acquire_lock();
    critical_section();
} finally {
    release_lock();
}
```

---

## 3. Underlying Systems & VM Mechanics

### Exception Dispatch Flowchart
```text
[ Throw Statement in Try Block ]
               │
               ▼
[ Try Block Exception Cleanup Segment ] (Decrements try-local variables via DEC_REF)
               │
               ▼
   [ Catch Matching Table ]
               │
   ┌───────────┴───────────┐
   ▼                       ▼
Clause 1: INSTANCEOF?    False ──► Clause 2: INSTANCEOF? ──► False ──► OpCode::JMP_TO_OUTER_CLEANUP
   │ (True)                               │ (True)
   ▼                                      ▼
SET_LOCAL e                            SET_LOCAL e
CLEAR_EXCEPTION                        CLEAR_EXCEPTION
Execute Catch Body                     Execute Catch Body
DEC_REF e                              DEC_REF e
   │                                      │
   └───────────────┬──────────────────────┘
                   │
                   ▼
         [ Finally Block ] (If present, executes unconditionally)
                   │
                   ▼
     [ Normal Execution Resumes ]
```

---

## 4. Positive Test Scenarios (Valid Variations)

### Scenario 4.1: Normal Execution (No Exceptions Thrown)
When no exception occurs, the try block executes to completion, catch clauses are entirely skipped, and normal execution resumes.
```solix
int32 step = 0;
try {
    step = 1;
} catch (std.Exception e) {
    step = 99; // Skipped
}
step = 2;
```
*Verification*: `step` equals 2. Catch bytecode is never evaluated.

### Scenario 4.2: Catching and Recovering from Exception
An exception thrown inside the try block is caught, handled, and the program recovers cleanly.
```solix
bool recovered = false;
try {
    throw new std.Exception("Failure");
} catch (std.Exception e) {
    recovered = true;
}
// Execution continues here normally
```
*Verification*: `recovered` is `true`. VM `active_exception` is `null`.

### Scenario 4.3: Specific Catch Ordering (Subclass Before Superclass)
Multiple catch clauses arranged from most specific subclass to most generic base class.
```solix
class CustomA extends std.Exception { CustomA() : super("A") {} }
class CustomB extends std.Exception { CustomB() : super("B") {} }

int32 caught_type = 0;
try {
    throw new CustomB();
} catch (CustomA a) {
    caught_type = 1;
} catch (CustomB b) {
    caught_type = 2; // Must match here
} catch (std.Exception e) {
    caught_type = 3;
}
```
*Verification*: `caught_type` equals 2. `INSTANCEOF` checks for `CustomA` evaluated to false, then `CustomB` evaluated to true.

### Scenario 4.4: Catch Parameter Variable Cleanup
The exception object bound to the catch parameter must have its reference decremented upon exiting the catch clause.
```solix
void test_catch_param_cleanup() {
    try {
        throw new std.Exception("temporary");
    } catch (std.Exception err) {
        Console.println(err.getMessage());
    }
    // 'err' was decremented via DEC_REF upon catch exit
}
```
*Verification*: Zero memory leaks. Exception object is deallocated once the catch block finishes.

### Scenario 4.5: Try-Finally Execution Guarantee
A finally block executes regardless of whether an exception was thrown or caught.
```solix
bool finally_ran = false;
try {
    int32 x = 42;
} finally {
    finally_ran = true;
}
```
*Verification*: `finally_ran` is `true`.

---

## 5. Negative Test Scenarios (Invalid Variations)

### Scenario 5.1: Try Block Without Catch or Finally
A `try` statement cannot exist in isolation without at least one `catch` clause or a `finally` block.
```solix
void test_isolated_try() {
    try {
        int32 x = 1;
    } // Error: expected 'catch' or 'finally'
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Syntax error: 'try' statement must have at least one 'catch' or 'finally' block
```

### Scenario 5.2: Catching Non-Exception Type
Attempting to catch a class that does not inherit from `std.Exception`.
```solix
class CustomWidget {}

void test_catch_non_exception() {
    try {
        do_work();
    } catch (CustomWidget w) { // Error: CustomWidget does not inherit from std.Exception
        Console.println("Error");
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Catch parameter type 'CustomWidget' must inherit from 'std.Exception'
```

### Scenario 5.3: Unreachable Catch Clause (Hiding Subclass Handler)
Declaring a catch clause for a base class *before* a catch clause for a subclass makes the second clause completely unreachable.
```solix
class CustomError extends std.Exception { CustomError() : super("Custom") {} }

void test_unreachable_catch() {
    try {
        do_work();
    } catch (std.Exception e) {
        Console.println("Base catch");
    } catch (CustomError c) { // Error: unreachable catch block
        Console.println("Unreachable");
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Unreachable catch clause: 'CustomError' is already handled by preceding catch for 'std.Exception'
```

### Scenario 5.4: Accessing Catch Parameter Outside Catch Scope
Attempting to reference the exception variable declared in a catch clause outside that clause.
```solix
void test_catch_var_leak() {
    try {
        do_work();
    } catch (std.Exception err) {
        Console.println("Caught");
    }
    Console.println(err.getMessage()); // Error: err is out of scope
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Undefined identifier: err
```
