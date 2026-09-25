# §11 BlockStatement

## 1. Overview & Scope

A `BlockStatement` is a fundamental structural construct in Solix that encapsulates an ordered sequence of zero or more statements within a pair of enclosing curly braces (`{ ... }`). It establishes an isolated lexical declaration space, defines the deterministic extent of local variable lifetimes under the Automatic Reference Counting (ARC) memory model, and serves as an unwinding trampoline unit during exceptional control-flow transfers.

### Syntactic Placement
A `BlockStatement` is legally permitted in the following contexts:
1. **Method & Constructor Bodies**: As the top-level execution container of a subroutine (`BlockKind::FUNCTION_BODY`).
2. **Control-Flow Branch Bodies**: As the executable body of `if`, `else`, `while`, `do-while`, `for`, and `switch` statements.
3. **Protected & Recovery Bodies**: As the protected body of a `try` construct (`BlockKind::TRY_BODY`), a `catch` handler, or a `finally` block.
4. **Standalone Lexical Blocks**: As an anonymous nested block situated arbitrarily within any executable statement sequence.

A `BlockStatement` is **strictly prohibited** at the translation-unit (package) level and directly within class or interface declaration bodies outside of member routines.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
BlockStatement       ::= '{' StatementSequence? '}'
StatementSequence    ::= Statement+
Statement            ::= VariableDeclaration
                       | ExpressionStatement
                       | IfStatement
                       | WhileStatement
                       | DoWhileStatement
                       | ForStatement
                       | SwitchStatement
                       | BreakStatement
                       | ContinueStatement
                       | ReturnStatement
                       | ThrowStatement
                       | TryStatement
                       | BlockStatement
```

### Canonical Code Patterns
```solix
// 1. Anonymous Standalone Block
{
    int32 temp = 100;
    Console.println(temp);
}

// 2. Control-Flow Bound Block
if (status == 1) {
    String message = new String("Active");
    send(message);
}

// 3. Nested Hierarchical Blocks with Lexical Shadowing
int32 value = 1;
{
    int32 value = 2; // Shadows outer 'value'
    {
        int32 inner = value * 10;
        Console.println(inner); // Prints 20
    }
    Console.println(value); // Prints 2
}
Console.println(value); // Prints 1
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Visibility Rules
- **Enclosure Boundary**: Every `BlockStatement` introduces a distinct child `SymbolTable` scope. Any identifier introduced via a `VariableDeclaration` within the block is visible only from the point of its declaration downward until the closing brace `}`.
- **Downward Visibility**: Child blocks nested within the block inherit access to all identifiers declared in the parent block.
- **Upward Non-Visibility**: Once execution or compilation exits the block, all identifiers declared within that block become completely unreachable. Any reference to such an identifier from an outer scope results in a compile-time diagnostic.

### 3.2 Shadowing Rules
- An identifier declared within an inner block masks (shadows) any identifier with an identical name declared in an enclosing outer block or outer function scope.
- Lookups for the shadowed identifier within the inner block resolve exclusively to the inner declaration's type, memory slot, and qualifiers.
- Upon termination of the inner block, the masking effect immediately dissolves, restoring visibility to the outer declaration.
- **Duplicate Declaration Invariant**: Shadowing is permitted only across distinct, nested scope boundaries. Declaring two identifiers with the same name within the *same immediate* block scope is illegal.

### 3.3 Lifetime (Static Extent)
- The static extent of a local variable spans from its initialization point to the terminating brace `}` of its enclosing block.
- Local variables are allocated zero-indexed register slots (`memory_index`) within the function's activation frame. Register indices are monotonic and persistent for the duration of the frame, but the *accessibility and ARC ownership* of those slots are bounded strictly by the block's lexical lifetime.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Normal Completion
Execution of a `BlockStatement` proceeds through the following formal operational steps:
1. **Scope Entry**: The VM retains the current activation frame base pointer. No stack adjustment is required for entry.
2. **Sequential Statement Execution**: The statements within `StatementSequence` are executed in strict sequential order from first to last ($S_1, S_2, \dots, S_n$).
3. **Sub-statement Evaluation**: Each statement $S_i$ is executed to completion. If statement $S_i$ completes normally, control proceeds to $S_{i+1}$.
4. **Normal Scope Teardown**: Upon normal completion of the final statement $S_n$ (or immediately if the block is empty), the VM initiates deterministic scope cleanup:
   - The compiler emits cleanup opcodes traversing all child declarations of the block in **reverse declaration order** ($D_m, D_{m-1}, \dots, D_1$).
   - For every declaration where `is_reference_type == true`:
     ```bytecode
     OpCode::GET_LOCAL <memory_index>
     OpCode::DEC_REF
     ```
5. **Completion Transition**: Control transfers unconditionally past the block's exception cleanup segment to the subsequent instruction in the program.

### 4.2 Abrupt Completion
A `BlockStatement` completes abruptly if the evaluation of any contained statement completes abruptly.
- **Abrupt Completion via `return`**:
  1. The return value expression (if present) is evaluated and pushed to the operand stack.
  2. The AST parent hierarchy is traversed from the return node up to the containing function root.
  3. Reverse-order `DEC_REF` cleanup is emitted for every intermediate block crossed.
  4. Function-level cleanup decrements receiver `this` and reference arguments.
  5. The VM executes `OpCode::RETURN`, restoring caller state with the return value at the stack top.
- **Abrupt Completion via `break`**:
  1. The AST parent hierarchy is traversed up to the enclosing loop or `switch` construct.
  2. Reverse-order `DEC_REF` cleanup is emitted for all intermediate blocks.
  3. The VM executes an unconditional `OpCode::JUMP` to the construct's break exit target.
- **Abrupt Completion via `continue`**:
  1. The AST parent hierarchy is traversed up to the enclosing loop construct.
  2. Reverse-order `DEC_REF` cleanup is emitted for all intermediate blocks.
  3. The VM executes an unconditional `OpCode::JUMP` to the loop's continuation (step) target.

### 4.3 Exception Unwinding & Trampolines
When an exception is thrown abruptly (via `throw` or runtime fault):
1. **Throw Interception**: The throwing instruction jumps directly to the current block's **Exception Cleanup Segment** (`cleanup_ip`).
2. **Local Frame Cleanup**: The cleanup segment executes reverse-order `DEC_REF` deallocations for all reference variables declared in the block, reclaiming memory allocated prior to the fault.
3. **Next-Hop Dispatch**:
   - If `block_kind == BlockKind::TRY_BODY`: Control jumps directly into the enclosing `TryStatement`'s catch matching table.
   - If `block_kind == BlockKind::NORMAL`: Control jumps to the enclosing parent block's exception cleanup segment.
   - If `block_kind == BlockKind::FUNCTION_BODY`: The segment emits `OpCode::JMP_TO_OUTER_CLEANUP`, unwinding the current activation record and propagating the exception to the caller frame.

---

## 5. Memory Model & ARC Invariants

### 5.1 ARC Ownership & Reference Counter Invariants
- **Invariant 1 (Positive Ownership)**: Any heap object reference stored in a local variable slot must have a `ref_count >= 1`.
- **Invariant 2 (Strict LIFO Teardown)**: In a block declaring reference variables $V_1, V_2, \dots, V_k$, their reference counts must be decremented in reverse order ($V_k$ first, $V_1$ last). This ensures that if $V_k$ holds an internal reference to $V_1$, $V_k$ is released before its prerequisite $V_1$.
- **Invariant 3 (Deallocation on Zero)**: When `DEC_REF` transitions an object's `ref_count` to 0, the runtime immediately invokes the object's destructor and frees the heap allocation via `RELEASE`.

### 5.2 Bytecode Lowering & Trampoline Architecture
The compiler generates a dual-path binary layout for every `BlockStatement`:

```text
+-----------------------------------------------------------------------+
|                       NORMAL EXECUTION PATH                           |
+-----------------------------------------------------------------------+
| [Child Statement Bytecode Sequence]                                   |
+-----------------------------------------------------------------------+
| [Normal Scope Cleanup]                                                |
| For each reference local in reverse declaration order:                |
|     OpCode::GET_LOCAL <memory_index>                                  |
|     OpCode::DEC_REF                                                   |
+-----------------------------------------------------------------------+
| OpCode::JUMP <skip_cleanup_ip>                                        |
+=======================================================================+
|                 EXCEPTION CLEANUP SEGMENT (cleanup_ip)                |
+=======================================================================+
| [Target of all inner exception throw jumps]                           |
| For each reference local in reverse declaration order:                |
|     OpCode::GET_LOCAL <memory_index>                                  |
|     OpCode::DEC_REF                                                   |
+-----------------------------------------------------------------------+
| Next-Hop Dispatch:                                                    |
|   - If FUNCTION_BODY: OpCode::JMP_TO_OUTER_CLEANUP                    |
|   - If TRY_BODY:      OpCode::JUMP -> Catch Matching Table            |
|   - If NORMAL:        OpCode::JUMP -> Parent Block cleanup_ip         |
+=======================================================================+
| [Continuation Point (<skip_cleanup_ip>)]                              |
| Normal sequential execution resumes here.                             |
+-----------------------------------------------------------------------+
```

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Prohibition of Out-of-Scope Variable Access
An identifier declared within a block cannot be accessed outside the lexical boundary of that block.
```solix
void test_out_of_scope() {
    {
        int32 scoped_val = 42;
    }
    int32 leak = scoped_val;
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Undefined identifier: scoped_val
```

### Rule 6.2: Prohibition of Duplicate Identifiers in Immediate Scope
Declaring two variables with identical identifiers in the same immediate block is illegal.
```solix
void test_duplicate() {
    {
        int32 count = 1;
        float64 count = 2.0;
    }
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Variable 'count' is already defined in the current scope
```

### Rule 6.3: Unterminated Block Boundary
Failing to supply a terminating closing brace `}` before reaching EOF or an outer construct boundary triggers a parser syntax error.
```solix
void test_unclosed() {
    {
        int32 x = 10;
// Missing '}'
```
*Diagnostic Message*:
```text
[ERROR] parser.cpp: Syntax error: expected '}' before end of file
```

### Rule 6.4: Extraneous Closing Brace
Encountering an unmatched closing brace `}` outside any active block terminates compilation immediately.
```solix
void test_stray() {
    {
        int32 x = 10;
    }
    }
}
```
*Diagnostic Message*:
```text
[ERROR] parser.cpp: Syntax error: unexpected token '}'
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Deep Recursion Stack Frame Exhaustion
Creating deeply nested block activations via recursion that exceed the VM's configured activation frame depth limits triggers an unrecoverable stack overflow.
```solix
void infinite_block_recursion() {
    {
        infinite_block_recursion();
    }
}
```
*Runtime Fault*:
```text
[FATAL VM PANIC] StackOverflowException: Call stack depth exceeded limit (1024 frames)
```

### Fault 7.2: Uncaught Exception Propagation Past Root Block
When an exception unwinds through the root `FUNCTION_BODY` block of `main` without encountering an enclosing `try-catch`, the VM terminates the process.
```solix
void main() {
    {
        throw new std.Exception("Fatal unhandled error");
    }
}
```
*Runtime Fault*:
```text
[FATAL VM PANIC] Unhandled Exception: 'Fatal unhandled error'
    at main() in test.slx:line 3
Process terminated with exit code 1
```

---

## 8. Conformance & Verification Examples

### Example 8.1: Lexical Shadowing with Differing Types
```solix
// Conformance Test: Scope masking must isolate variable types and slots
int32 target = 10;
{
    String target = new String("shadow_str");
    Console.println(target); // Must output: "shadow_str"
}
Console.println(target);     // Must output: 10
```
*Verification Invariant*: Two distinct frame indices are allocated. The `String` object is deallocated upon exiting the inner block; `target` remains 10.

### Example 8.2: Guaranteed LIFO Destruction Under Normal Execution
```solix
class Tracker {
    int32 id;
    Tracker(int32 id) { this.id = id; }
}

void verify_lifo() {
    {
        Tracker first = new Tracker(1);
        Tracker second = new Tracker(2);
        Tracker third = new Tracker(3);
    }
    // Execution must decrement: third, then second, then first.
}
```
*Verification Invariant*: Bytecode disassembly confirms `DEC_REF` sequence matches exact reverse order: `third` -> `second` -> `first`.

### Example 8.3: Non-Local Early Return Across Nested Lexical Blocks
```solix
int32 verify_early_return(bool condition) {
    String res1 = new String("res1");
    {
        String res2 = new String("res2");
        {
            String res3 = new String("res3");
            if (condition) {
                return 42; // Must emit cleanups for res3, res2, and res1 before RETURN
            }
        }
    }
    return 0;
}
```
*Verification Invariant*: Both `verify_early_return(true)` and `verify_early_return(false)` result in 0 net heap allocations remaining.

### Example 8.4: Exception Unwinding Through Multiple Enclosed Blocks
```solix
void verify_unwinding_integrity() {
    try {
        String outer = new String("outer");
        {
            String middle = new String("middle");
            {
                String inner = new String("inner");
                throw new std.Exception("abort");
            }
        }
    } catch (std.Exception e) {
        // inner, middle, and outer must be reclaimed before entering catch body
    }
}
```
*Verification Invariant*: Heap object allocation counter confirms `inner`, `middle`, and `outer` are all reclaimed by unwinding trampolines prior to entering the catch block.
