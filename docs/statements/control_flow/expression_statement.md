# §13 ExpressionStatement

## 1. Overview & Scope

An `ExpressionStatement` wraps an expression to evaluate it purely for its side effects (such as value assignments, object mutations, method invocations, or increment/decrement operations). Any computed result produced by the expression is systematically consumed and discarded from the Virtual Machine operand stack, preserving strict operand stack alignment across sequential statement execution.

In Solix's Automatic Reference Counting (ARC) architecture, an expression statement carries a crucial memory-safety responsibility: if the underlying expression produces an unassigned temporary reference object (such as calling a method that returns a newly allocated `String` or class instance without storing it in a variable), the runtime must immediately decrement and deallocate the temporary object to prevent silent memory leaks.

### Syntactic Placement
An `ExpressionStatement` is permitted inside any statement sequence within blocks, control-flow branches, and subroutine bodies. It is prohibited at file root scope and class declaration bodies outside methods.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
ExpressionStatement ::= Expression ';'
```

### Canonical Code Patterns
```solix
// 1. Method Invocation for Side Effects
Console.println("Log event");

// 2. Assignment Expression Statement
counter = counter + 1;

// 3. Increment / Decrement Side Effects
index++;
--retry_count;

// 4. Object Instantiation Without Variable Retention
new TemporaryTask().execute();
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Expression Scope
- The contained expression is resolved within the immediate lexical scope in which the statement appears.
- All identifiers referenced must be valid and visible within the active `SymbolTable`.

### 3.2 Result Discard & Void Invariant
- The expression may produce any primitive or reference type, or it may be of type `void`.
- Regardless of the evaluated expression's type, the statement itself produces no value and cannot be embedded as an operand in another expression.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Normal Completion
Execution of an `ExpressionStatement` proceeds through the following steps:
1. **Expression Evaluation**: The inner expression is evaluated to completion. If the expression computes a value, that value is pushed onto the VM operand stack.
2. **Result Discard & Stack Alignment**:
   - The compiler inspects the expression's static type (`expression_type`).
   - **Reference Type Discard**: If `is_reference_type(expression_type) == true`:
     ```bytecode
     OpCode::DEC_REF
     ```
     The reference pointer on top of the stack is popped and its reference count is decremented. If `ref_count == 0`, the temporary object is immediately freed via `RELEASE`.
   - **Value Type Discard**: If the type is a primitive scalar (non-reference):
     ```bytecode
     OpCode::POP
     ```
     The 8-byte scalar value is popped from the operand stack, restoring the stack top to its pre-statement baseline.
   - **Void Discard**: If the expression is of type `void` (e.g. `Console.println`), no value was pushed, so neither `POP` nor `DEC_REF` is emitted.
3. Execution proceeds sequentially to the next statement.

### 4.2 Abrupt Completion
An `ExpressionStatement` completes abruptly if the evaluation of its inner expression completes abruptly (such as throwing an exception or triggering a division by zero). The operand stack discard is bypassed, and control transfers to the enclosing block's exception cleanup segment.

### 4.3 Exception Unwinding & Trampolines
If an expression statement throws an exception midway through evaluation (e.g. `obj.riskyMethod()`), the partially evaluated expression unwinds into the block's exception cleanup segment.

---

## 5. Memory Model & ARC Invariants

### 5.1 Temporary Reference Object Destruction Invariant
- Every expression producing a reference type increments that object's reference counter upon creation.
- If that reference is not captured by an lvalue (such as a local variable or field assignment), the `ExpressionStatement` is obligated to emit `OpCode::DEC_REF`.
- This guarantees that statements such as `new String("temp").trim();` do not leak temporary heap objects.

### 5.2 Compiler Lowering Logic (`assembler.cpp`)
```cpp
void Assembler::visit(ExpressionStatement &node) {
  compile_expression(node.expression.get());
  bool is_ref = is_reference_type(node.expression->expression_type);
  if (is_ref) {
    emit_byte(static_cast<uint8_t>(OpCode::DEC_REF));
  } else if (node.expression->expression_type.name != "void") {
    emit_byte(static_cast<uint8_t>(OpCode::POP));
  }
}
```

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Missing Terminating Semicolon
Every expression statement must terminate with a semicolon.
```solix
void test_missing_semi() {
    Console.println("error") // Missing ';'
}
```
*Diagnostic Message*:
```text
[ERROR] parser.cpp: Syntax error: expected ';' after expression
```

### Rule 6.2: Undeclared Variable Assignment
Attempting to assign to an undeclared identifier in an expression statement is illegal.
```solix
void test_undeclared() {
    unknown_var = 10;
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Undefined identifier: unknown_var
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Method Call on Null Receiver
Invoking an instance method on an expression that evaluates to `null` triggers an immediate runtime panic.
```solix
void test_null_call() {
    String str = null;
    str.length(); // Null dereference
}
```
*Runtime Fault*:
```text
[FATAL VM PANIC] NullReferenceException: Attempted to invoke method on null object reference
```

---

## 8. Conformance & Verification Examples

### Example 8.1: Discarding Temporary Heap Reference
```solix
// Conformance Test: Unstored return reference must be reclaimed immediately
String generate_text() {
    return new String("temporary_payload");
}

void verify_temporary_reclamation() {
    generate_text(); // Result is ignored
    // Bytecode must emit DEC_REF, releasing 'temporary_payload'
}
```
*Verification Invariant*: Disassembly confirms `OpCode::DEC_REF` is emitted after calling `generate_text()`. Net heap allocation count is 0.

### Example 8.2: Stack Invariance Across Primitive Expressions
```solix
void verify_stack_invariance() {
    int32 x = 10;
    x + 5; // Primitive expression evaluated and popped
    // Operand stack depth remains exactly 0
}
```
*Verification Invariant*: Disassembly confirms `OpCode::POP` is emitted following evaluation of `x + 5`.
