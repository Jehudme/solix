# BlockStatement (`NodeType::BLOCK`)

## 1. Description, Purpose & Architectural Implementation

### Conceptual Overview
The `BlockStatement` is the primary structural and scoping container in the Solix programming language. Syntactically delimited by a pair of matching curly braces (`{ ... }`), it encapsulates an ordered sequence of zero or more statements into a single executable unit.

In Solix, a block statement is not a passive syntactic group. It is an active runtime and compile-time lifecycle boundary. It governs the declaration, accessibility, and destruction of variables, controls memory reclamation under Automatic Reference Counting (ARC), and serves as the essential landing anchor during structured exception unwinding.

---

### Key Conceptual Roles

#### 1. Lexical Scope Isolation
Every block statement defines a distinct lexical environment in Solix. Identifiers declared within a block are strictly confined to that block and any child blocks nested beneath it. When execution exits the block, its variables cease to exist in the compiler's symbol table. This isolation enforces the principle of least privilege in variable visibility, preventing accidental state contamination, reducing coupling, and making programs easier to reason about.

#### 2. Variable Shadowing & Scope Masking
Solix allows an inner block to declare a variable with the exact same identifier as a variable declared in an enclosing outer block. When this occurs:
- The inner variable **masks** (shadows) the outer variable for the entire lifetime of the inner block.
- Any lookup of the identifier within the inner block resolves exclusively to the inner variable's type and memory slot.
- Once the inner block terminates, the mask is removed, and subsequent lookups transparently resolve back to the outer variable.
- Shadowing is strictly prohibited within the *same* immediate block scope (which triggers a duplicate declaration error), but permitted across nested scope boundaries.

#### 3. Deterministic ARC Resource Reclamation
Solix employs Automatic Reference Counting (ARC) rather than a non-deterministic tracing garbage collector. Under ARC, heap-allocated objects (such as class instances, strings, and arrays) have an internal reference counter that tracks active references.
- When an object reference is stored in a local variable inside a block, its reference count is incremented (`INC_REF`).
- The block statement serves as the strict deterministic lifetime boundary for these local variables.
- Upon natural exit of the block, Solix automatically decrements the reference counts (`DEC_REF`) of all reference-typed local variables declared within that block.
- If a reference count reaches zero, the object is immediately deallocated from heap memory via `RELEASE`.
- Decrements are emitted in strict **reverse declaration order** (LIFO: Last-In, First-Out), ensuring that dependent resources allocated later in the block are torn down before the prerequisites allocated earlier.

#### 4. Structured Exception Unwinding & Trampoline Anchoring
When a runtime exception is thrown (via `throw` or a runtime panic), normal sequential execution is interrupted. The call stack must unwind until a matching `try-catch` handler is encountered.
- Without proper cleanup during unwinding, any local reference variables held in active blocks between the throw site and the catch handler would be leaked permanently.
- Solix solves this by treating every `BlockStatement` as an unwinding trampoline. Every block generates an isolated **Exception Cleanup Segment** in its compiled bytecode.
- When an exception occurs within a block, execution jumps directly to that block's cleanup segment, decrements all local reference variables in that block, and then transfers control to the next outer cleanup segment or enclosing catch handler.

---

### How BlockStatement Was Implemented in Solix

The implementation of `BlockStatement` spans the entire Solix compiler and runtime pipeline:

#### 1. Abstract Syntax Tree Representation (`statements.hpp`)
In the AST, a block is represented by the `BlockStatement` struct, inheriting from `Node`:
```cpp
enum class BlockKind {
    NORMAL,         // A standard lexical block or nested block
    FUNCTION_BODY,  // The root body block of a method or constructor
    TRY_BODY        // The body block directly enclosed by a try statement
};

struct BlockStatement : public Node {
    BlockKind block_kind = BlockKind::NORMAL;
    BlockStatement(const Token& t) : Node(NodeType::BLOCK, t) {}
};
```
The `block_kind` property is critical: it informs the code generator what kind of jump instruction must be emitted at the tail of the block's exception cleanup segment.

#### 2. Semantic Analysis & Symbol Binding (`binder.cpp`)
During semantic analysis (`BinderPass::BIND_EXECUTION`):
- The binder calls `push_scope()`, pushing a fresh `SymbolTable` onto the compiler's lexical scope stack.
- The binder iterates through the block's `children` statements. When child `VariableDeclaration` nodes are encountered, their types are resolved, and each is assigned a contiguous `memory_index` slot corresponding to a local variable register in the VM activation frame.
- When all child nodes have been visited, the binder calls `pop_scope()`, popping the `SymbolTable` and restoring the parent lexical environment.

#### 3. Bytecode Generation & Cleanup Patching (`assembler.cpp`)
The code generator (`Assembler::visit(BlockStatement &node)`) implements a sophisticated dual-path layout using forward jump patching:
1. **Push Exception Patch Table**: The compiler pushes a new patch list onto `exception_cleanup_patches`.
2. **Compile Children**: All child statements in the block are compiled sequentially.
3. **Emit Normal Execution Cleanup**: The compiler calls `emit_cleanup_for_node(&node)`, which iterates through the block's children in **reverse order** (`rbegin()` to `rend()`). For every `VariableDeclaration` where `is_reference_type == true`, it emits:
   ```bytecode
   GET_LOCAL <memory_index>
   DEC_REF
   ```
4. **Emit Jump Over Cleanup**: Normal execution must not fall into the exception handler. The compiler emits an unconditional `JUMP` with a 4-byte placeholder target offset (`skip_cleanup_idx`).
5. **Compile Exception Cleanup Segment**:
   - The current bytecode offset is recorded as `cleanup_ip`.
   - All inner exceptions that occurred within this block (recorded in `exception_cleanup_patches.back()`) have their jump offsets patched to point directly to `cleanup_ip`.
   - The compiler pops the patch list from `exception_cleanup_patches`.
   - It re-emits the exact reverse-order cleanup (`emit_cleanup_for_node(&node)`) to decrement local references during unwinding.
6. **Next Hop Dispatch**:
   - If `block_kind == BlockKind::FUNCTION_BODY`: Emits `OpCode::JMP_TO_OUTER_CLEANUP`.
   - If `block_kind == BlockKind::TRY_BODY`: Pushes a jump patch to the outer block's list so `TryStatement` can route the unwound exception to matching catch clauses.
   - If `block_kind == BlockKind::NORMAL`: Emits an unconditional `JUMP` to the outer block's exception cleanup segment.
7. **Patch Normal Skip Jump**: Finally, the placeholder at `skip_cleanup_idx` is patched to point to the instruction immediately following the exception cleanup segment.

---

## 2. Syntax & Grammar

### Syntax Forms
A block statement begins with an opening curly brace `{`, contains zero or more executable statements, and concludes with a matching closing curly brace `}`:

```solix
// Empty Block
{
}

// Block with Variable Declarations and Statements
{
    <Statement_1>;
    <Statement_2>;
    ...
    <Statement_N>;
}
```

### Contextual Usage in Solix
Block statements appear in multiple distinct structural contexts:
1. **Standalone / Local Blocks**: Placed arbitrarily inside functions to limit the scope and lifetime of temporary variables.
2. **Function / Method Bodies**: Forms the root execution container of functions, methods, and constructors (`BlockKind::FUNCTION_BODY`).
3. **Control Flow Bodies**: Forms the execution branches of `if`, `else`, `while`, `do-while`, `for`, and `switch` statements.
4. **Exception Handling Bodies**: Forms the protected `try` block (`BlockKind::TRY_BODY`), `catch` handlers, and `finally` blocks.

```solix
void example_contexts() {
    // 1. Standalone local block
    {
        int32 temp = 42;
        Console.println(temp);
    } // temp is destroyed here

    // 2. Control-flow attached blocks
    if (true) {
        String msg = new String("Hello");
        Console.println(msg);
    } // msg is decremented and released here
}
```

---

## 3. Underlying Systems & VM Mechanics

### Dual-Path Bytecode Layout
The following diagram illustrates the binary structure generated by the Solix compiler for every `BlockStatement`:

```text
+-------------------------------------------------------------------+
|               NORMAL EXECUTION PATH (Fall-through)                |
+-------------------------------------------------------------------+
| 1. Child Statement Bytecode (Expressions, Assignments, etc.)      |
+-------------------------------------------------------------------+
| 2. Normal Scope Cleanup (emit_cleanup_for_node):                  |
|    For each local reference in reverse order:                     |
|        OpCode::GET_LOCAL <slot_index>                             |
|        OpCode::DEC_REF                                            |
+-------------------------------------------------------------------+
| 3. OpCode::JUMP <skip_ip>                                         |
|    (Bypasses the exception cleanup segment below)                 |
+===================================================================+
|             EXCEPTION UNWINDING SEGMENT (cleanup_ip)              |
+===================================================================+
| 4. Exception Target Landing:                                      |
|    - All inner throws/panics in this block jump directly here.    |
|    - Unwinding ARC Cleanup (emit_cleanup_for_node):               |
|        OpCode::GET_LOCAL <slot_index>                             |
|        OpCode::DEC_REF                                            |
+-------------------------------------------------------------------+
| 5. Next Hop Unwinding Dispatch:                                   |
|    - If FUNCTION_BODY: OpCode::JMP_TO_OUTER_CLEANUP               |
|    - If TRY_BODY:      OpCode::JUMP -> Catch Clause Dispatch Table |
|    - If NORMAL:        OpCode::JUMP -> Enclosing Block cleanup_ip |
+===================================================================+
| 6. Continuation Target (<skip_ip>)                                |
|    Normal execution resumes here after bypassing the cleanup.     |
+-------------------------------------------------------------------+
```

### Early Jump Handling (`return`, `break`, `continue`)
When a statement within a block transfers control out of the block non-sequentially, normal fall-through is bypassed. The compiler handles this during AST traversal:
- **`return` Statements**: When `Assembler::visit(ReturnStatement &node)` executes, it walks up the AST parent chain from the return node to the enclosing function declaration. For every `BlockStatement` encountered, it immediately emits `emit_cleanup_for_node(current)`. Then it emits `emit_cleanup_for_function(func_node)` to decrement the `this` pointer (for instance methods) and all reference parameters, before finally emitting `OpCode::RETURN`.
- **`break` Statements**: When `Assembler::visit(BreakStatement &node)` executes, it walks up the AST parent chain until finding the target loop or `switch` statement. For every intermediate `BlockStatement` crossed, it emits `emit_cleanup_for_node(current)`. It then emits `OpCode::JUMP` to the loop's break patch address.
- **`continue` Statements**: Similarly walks up to the target loop, emits `emit_cleanup_for_node(current)` for all intermediate blocks, and emits `OpCode::JUMP` to the loop's continuation address.

---

## 4. Positive Test Scenarios (Valid Variations)

### Scenario 4.1: Empty and Nested Empty Blocks
An empty block contains zero statements and must compile cleanly with no stack delta and zero opcode side effects.
```solix
void test_empty_blocks() {
    {}
    {
        {}
        {
            {}
        }
    }
}
```
*Verification*: Compiles without warnings or errors. VM execution produces no stack growth or leaks.

### Scenario 4.2: Lexical Variable Shadowing
An inner block shadows an outer variable of the same name with a different type and value.
```solix
int32 value = 10;
{
    String value = new String("inner_shadow");
    Console.println(value); // Must resolve to String
}
Console.println(value); // Must resolve to int32 (10)
```
*Verification*: Output must be `"inner_shadow"` followed by `10`. Memory inspection confirms `String` was deallocated at the closing brace of the inner block.

### Scenario 4.3: Strict LIFO Destruction of Multiple Reference Objects
Multiple reference-counted objects allocated in a single block must be cleaned up in exact reverse order of declaration upon exit.
```solix
class Tracker {
    int32 id;
    Tracker(int32 id) { this.id = id; }
}

void test_lifo_order() {
    {
        Tracker t1 = new Tracker(1);
        Tracker t2 = new Tracker(2);
        Tracker t3 = new Tracker(3);
    }
    // Expected bytecode order: DEC_REF t3, DEC_REF t2, DEC_REF t1
}
```
*Verification*: Bytecode inspection confirms `t3` (slot 2) is decremented first, then `t2` (slot 1), then `t1` (slot 0).

### Scenario 4.4: Early Return from Nested Blocks
A return statement nested deep inside several block scopes must correctly decrement all active reference variables across all intermediate blocks.
```solix
int32 compute_nested(bool early_exit) {
    String outer = new String("outer");
    {
        String middle = new String("middle");
        {
            String inner = new String("inner");
            if (early_exit) {
                return 42; // Must emit DEC_REF for inner, middle, and outer
            }
        }
    }
    return 0;
}
```
*Verification*: Calling `compute_nested(true)` decrements all three strings before returning. Zero heap objects remain allocated.

### Scenario 4.5: Loop Break Traversal Across Multiple Blocks
A `break` statement escaping from a block nested inside a loop must clean up all block-scoped objects without corrupting the loop exit.
```solix
void test_break_cleanup() {
    while (true) {
        String loop_scoped = new String("loop");
        {
            String block_scoped = new String("block");
            break; // Must clean up 'block_scoped' and 'loop_scoped'
        }
    }
}
```
*Verification*: VM heap object count drops back to zero immediately upon loop termination.

### Scenario 4.6: Exception Unwinding Through Multiple Lexical Blocks
An exception thrown inside an inner block unwinds through multiple enclosing block cleanup segments until caught by a `try-catch`.
```solix
void test_unwinding_cleanup() {
    try {
        String s1 = new String("s1");
        {
            String s2 = new String("s2");
            {
                String s3 = new String("s3");
                throw new std.Exception("abort");
            }
        }
    } catch (std.Exception e) {
        // At this point, s3, s2, and s1 must all have been decremented
    }
}
```
*Verification*: Both `s3`, `s2`, and `s1` have their reference counts decremented to 0 by the unwinding trampolines before the catch block executes.

---

## 5. Negative Test Scenarios (Invalid Variations)

### Scenario 5.1: Referencing Out-of-Scope Variable Outside Block
Variables declared inside a block cannot be accessed once that block has closed.
```solix
void test_scope_leakage() {
    {
        int32 block_var = 123;
    }
    int32 leak = block_var; // Illegal: block_var does not exist here
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Undefined identifier: block_var
```

### Scenario 5.2: Missing Closing Brace (Unterminated Block)
Failing to close a block before reaching the end of a file or enclosing construct must trigger a parser error.
```solix
void test_unterminated_block() {
    int32 x = 10;
    {
        int32 y = 20;
    // Missing '}'
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Syntax error: expected '}' before end of file
```

### Scenario 5.3: Unexpected Extra Closing Brace
An extra closing brace without a matching opening brace must immediately halt parsing.
```solix
void test_stray_brace() {
    {
        int32 x = 10;
    }
    } // Stray extra closing brace
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Syntax error: unexpected token '}'
```

### Scenario 5.4: Duplicate Variable Declaration Within Same Immediate Block
While shadowing across parent-child blocks is valid, declaring two variables with identical names in the exact same block scope is illegal.
```solix
void test_duplicate_declaration() {
    {
        int32 item = 1;
        float64 item = 2.0; // Illegal: duplicate in same scope
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Variable 'item' is already defined in the current scope
```

### Scenario 5.5: Control Flow Jump into Uninitialized Block
Attempting to jump into the middle of a block from outside is grammatically and semantically impossible in Solix.
```solix
void test_illegal_jump() {
    goto inner_label; // Solix has no goto, labels are prohibited outside switch/loops
    {
        inner_label:
        int32 x = 10;
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Syntax error: unexpected token 'goto'
```
