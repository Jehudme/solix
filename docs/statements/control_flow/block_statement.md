# BlockStatement (`NodeType::BLOCK`)

## 1. Description & Purpose

The `BlockStatement` is the fundamental scoping and structural container in Solix. Enclosed by curly braces (`{ ... }`), it groups an arbitrary sequence of zero or more statements into a single syntactic unit. In Solix's design, a block is not merely syntactic sugar for compound execution—it is an active boundary for variable lifetime, lexical isolation, and deterministic resource reclamation.

### Key Conceptual Roles:
- **Lexical Scope Boundary**: Introduces an isolated lexical scope in the compiler's symbol table. Variables declared within the block are strictly invisible outside it, preventing accidental cross-scope contamination and variable name collisions.
- **Variable Shadowing**: Allows inner scopes to redeclare identifiers present in outer scopes, creating a localized masking effect that resolves back to the outer declaration once the inner block terminates.
- **ARC Resource Reclamation Boundary**: Establishes an automatic reference counting (ARC) lifetime domain. Every reference-counted object allocated or bound to a local variable inside the block is deterministically released upon scope exit.
- **Exception Unwinding Anchor**: Serves as a critical unit of stack unwinding during exception propagation. The compiler generates dedicated exception cleanup segments for every block to guarantee that local objects do not leak if an exception aborts the block midway through execution.

---

## 2. Syntax & Grammar

### Formal EBNF Grammar
```ebnf
BlockStatement   ::= '{' StatementList? '}'
StatementList    ::= Statement (Statement)*
Statement        ::= VariableDeclaration
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
                   | TryCatchFinallyStatement
                   | BlockStatement
```

### Visual Examples
```solix
// 1. Standalone / Anonymous Lexical Block
{
    int32 temp = 100;
    Console.println(temp);
}

// 2. Control-Flow Attached Block
if (isValid) {
    String msg = new String("Valid input");
    process(msg);
}

// 3. Deeply Nested Scoping with Shadowing
int32 counter = 1;
{
    int32 counter = 99; // Shadows outer 'counter'
    {
        int32 inner_val = counter * 2;
        Console.println(inner_val); // Prints 198
    }
    Console.println(counter); // Prints 99
}
Console.println(counter); // Prints 1
```

---

## 3. Underlying Systems & Mechanics

### 1. Semantic Analysis & Binder Mechanics (`binder.cpp`)
- **Symbol Table Stack Management**: When the binder encounters a `BlockStatement`, it invokes `push_scope()`, creating an isolated child `SymbolTable` referencing the parent scope.
- **Local Index Allocation**: Local variable declarations within the block are assigned sequential `memory_index` slot numbers corresponding to stack frame registers in the VM activation record.
- **Scope Teardown**: Upon completing binder traversal of the block's children, `pop_scope()` restores the enclosing lexical environment. Any lookups performed subsequently for identifiers declared within the child scope will fail.

### 2. Dual-Path Bytecode Generation & ARC Cleanup (`assembler.cpp`)
When the compiler emits bytecode for a `BlockStatement`, it generates two distinct execution paths: a **Normal Execution Path** and an **Exception Cleanup Path**.

```
+--------------------------------------------------------+
| 1. Child Statement Execution (Normal Operations)       |
+--------------------------------------------------------+
| 2. Normal Scope Cleanup:                               |
|    - Loop children in REVERSE order (rbegin to rend)   |
|    - For each reference VAR_DECL:                     |
|        OpCode::GET_LOCAL <memory_index>                |
|        OpCode::DEC_REF                                 |
+--------------------------------------------------------+
| 3. OpCode::JUMP <skip_cleanup_target>                  |
|    (Bypasses the exception cleanup segment below)      |
+========================================================+
| 4. Exception Cleanup Segment (cleanup_ip):             |
|    - Target for all inner exception patch jumps        |
|    - Re-emits ARC cleanup for all reference variables: |
|        OpCode::GET_LOCAL <memory_index>                |
|        OpCode::DEC_REF                                 |
|    - Next Hop Dispatch:                                |
|      * Function Body: OpCode::JMP_TO_OUTER_CLEANUP     |
|      * Try Body: OpCode::JUMP -> outer catch handler   |
|      * Nested Block: OpCode::JUMP -> parent cleanup_ip |
+========================================================+
| 5. Normal Continuation Target (<skip_cleanup_target>)  |
+--------------------------------------------------------+
```

### 3. Early Control Transfer Coordination
When control exits a block non-sequentially (`return`, `break`, `continue`), the compiler walks the AST ancestor hierarchy and emits inline cleanup opcodes:
- **`return`**: Traverses up from the current block to the enclosing `MethodDeclaration` or `ConstructorDeclaration`, emitting `GET_LOCAL` + `DEC_REF` for every reference variable in every block along the path, followed by cleanup for `this` (local 0) and parameters.
- **`break` / `continue`**: Traverses up to the target loop or switch statement, emitting reverse-order `DEC_REF` instructions for all intermediate blocks crossed before emitting the jump.

---

## 4. Positive Test Scenarios (Valid Variations)

### Scenario 4.1: Empty Block Execution
An empty block must compile to a no-op without affecting the operand stack or local register allocations.
```solix
void test_empty_block() {
    {}
    {
        {}
    }
}
```
*Expected Result*: Compiles cleanly; VM stack delta is exactly zero.

### Scenario 4.2: Scope Shadowing of Identifiers
Inner blocks may declare variables with names identical to those in outer scopes.
```solix
int32 x = 42;
{
    String x = new String("shadowed");
    Console.println(x);
}
Console.println(x);
```
*Expected Result*: Outputs `"shadowed"` then `42`. ARC releases `"shadowed"` at inner block exit without altering the outer `x`.

### Scenario 4.3: Multiple Reference Variables Reverse ARC Destruction
Reference variables must be destroyed in strict LIFO (reverse declaration) order on block exit.
```solix
{
    Resource first = new Resource(1);
    Resource second = new Resource(2);
    Resource third = new Resource(3);
}
// Bytecode must emit DEC_REF for third, then second, then first.
```
*Expected Result*: Refcount decrements happen in exact order: `third` -> `second` -> `first`.

### Scenario 4.4: Early `return` Across Multiple Nested Blocks
Returning from within deep blocks must unwind all intermediate lexical scopes.
```solix
int32 find_val(bool cond) {
    Resource r1 = new Resource(1);
    {
        Resource r2 = new Resource(2);
        {
            Resource r3 = new Resource(3);
            if (cond) {
                return 42; // Must emit DEC_REF for r3, r2, r1 before RETURN
            }
        }
    }
    return 0;
}
```
*Expected Result*: Both `find_val(true)` and `find_val(false)` produce zero memory leaks.

### Scenario 4.5: `break` and `continue` Across Intermediate Blocks
Breaking or continuing out of nested blocks inside loops must clean up all block-scoped resources up to the loop boundary.
```solix
while (true) {
    Resource outer = new Resource(10);
    {
        Resource inner = new Resource(20);
        if (condition) {
            break; // Must clean up 'inner' and 'outer'
        }
    }
}
```
*Expected Result*: Clean escape to loop exit with all allocated resources freed.

### Scenario 4.6: Exception Propagation Through Nested Blocks
When an exception is thrown inside a deeply nested block, the runtime trampoline unwinds through the exception cleanup segments.
```solix
try {
    Resource r1 = new Resource(1);
    {
        Resource r2 = new Resource(2);
        throw new std.Exception("boom");
    }
} catch (std.Exception e) {
    // Both r2 and r1 must be decremented before catch block begins
}
```
*Expected Result*: `r2` and `r1` are freed by the unwinding trampoline; catch block executes cleanly.

---

## 5. Negative Test Scenarios (Invalid Variations)

### Scenario 5.1: Accessing Block-Scoped Variable After Block Termination
Attempting to reference a variable outside the block where it was declared must fail during symbol binding.
```solix
void test_scope_leak() {
    {
        int32 secret = 100;
    }
    int32 leak = secret;
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Undefined identifier: secret
```

### Scenario 5.2: Unbalanced Curly Braces (Syntax Errors)
Mismatched or missing closing braces must trigger parser syntax errors and halt compilation.
```solix
void test_unmatched_brace() {
    int32 a = 1;
    {
        int32 b = 2;
    // Missing closing brace
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Syntax error: expected '}' before end of file
```

### Scenario 5.3: Superfluous Stray Closing Brace
An extra closing brace outside of any open block must fail parsing immediately.
```solix
void test_stray_brace() {
    int32 a = 1;
    {
        int32 b = 2;
    }
    } // Stray extra brace
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Syntax error: unexpected token '}'
```

### Scenario 5.4: Redefining Variable in the Same Immediate Scope
Redeclaring a variable with the exact same identifier in the same immediate block scope is illegal (distinct from shadowing in child scopes).
```solix
void test_duplicate_var() {
    {
        int32 duplicate = 10;
        float32 duplicate = 20.0;
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Variable 'duplicate' is already defined in the current scope
```
