# ReturnStatement (`NodeType::RETURN_STMT`)

## 1. Description, Purpose & Architectural Implementation

### Conceptual Overview
The `ReturnStatement` terminates execution of the current subroutine (method, function, or constructor), transfers control back to the caller's activation frame, and optionally yields a computed return value.

In Solix's memory-safe architecture, returning from a function is a complex, coordinated operation. It cannot simply pop the call stack pointer. Doing so would leave every active local reference variable, function parameter, and the instance's `this` pointer dangling or permanently leaked. Instead, the Solix compiler orchestrates a systematic lexical scope traversal that decrements all active reference variables across all enclosing blocks, decrements reference parameters and the receiver object, and preserves the return value on the operand stack before emitting the VM `RETURN` opcode.

---

### Key Conceptual Roles

#### 1. Function Execution Termination & Caller Transfer
The primary role of the `ReturnStatement` is to exit the current subroutine. Any statements located after a return statement within the same sequential execution block are unreachable dead code.

#### 2. Return Value Propagation & Type Compatibility
When a function is declared with a non-void return type, the return statement must supply an expression whose evaluated type is assignable to that declared return type:
- If the method return type is `void`, a bare `return;` is required (or returning void). Returning a value from a void method produces a compile-time diagnostic.
- If the method return type is non-void, returning without a value produces a compile-time diagnostic (`"Must return a value from non-void method"`).
- Type compatibility verifies inheritance upcasting, interface conformance, and numeric conversions (`is_assignable`).

#### 3. Comprehensive Lexical Scope Unwinding & ARC Teardown
A return statement may be located deep within nested loops, conditional branches, or anonymous blocks.
- Solix guarantees that returning early from an inner block does not leak resources allocated in outer blocks of that function.
- During compilation, the assembler traces the AST parent hierarchy from the `ReturnStatement` up to the root `MethodDeclaration` or `ConstructorDeclaration`.
- For every intermediate `BlockStatement` encountered, the compiler emits inline `DEC_REF` instructions for all active reference variables in that block in reverse declaration order.

#### 4. Parameter & Receiver (`this`) Lifecycle Teardown
Once all intermediate block-scoped locals have been decremented, the function's own activation frame inputs must be cleaned up:
- If the method is an instance method (non-static member of a class), slot 0 contains the implicit `this` reference. The compiler emits `GET_LOCAL 0` + `DEC_REF`.
- For every formal parameter that is a reference type (e.g. classes, arrays, strings), the compiler emits `GET_LOCAL <param_index>` + `DEC_REF`.

---

### How ReturnStatement Was Implemented in Solix

#### 1. Abstract Syntax Tree Representation (`statements.hpp`)
```cpp
struct ReturnStatement : public Node {
    std::unique_ptr<Node> value;

    ReturnStatement(const Token& t, std::unique_ptr<Node> val = nullptr)
        : Node(NodeType::RETURN_STMT, t), value(std::move(val)) {
        if (value) value->parent = this;
    }
};
```

#### 2. Semantic Analysis & Validation (`binder.cpp`)
During semantic analysis (`BinderPass::BIND_EXECUTION`):
```cpp
void Binder::visit(ReturnStatement &n) {
  if (current_pass == BinderPass::BIND_EXECUTION) {
    if (n.value) {
      TypeInfo return_type = evaluate_expression(n.value.get());
      if (current_method &&
          !is_assignable(current_method->return_type, return_type)) {
        record_error(&n, "Return type mismatch: expected '" +
                             current_method->return_type.name + "', got '" +
                             return_type.name + "'");
      }
    } else if (current_method && current_method->return_type.name != "void") {
      record_error(&n, "Must return a value from non-void method");
    }
  }
}
```

#### 3. Bytecode Emission & Lexical Unwinding (`assembler.cpp`)
The assembler implements early return scope unwinding by walking the AST hierarchy:
```cpp
void Assembler::visit(ReturnStatement &node) {
  // 1. Evaluate and push the return value (if non-void)
  if (node.value) {
    compile_expression(node.value.get());
  }

  // 2. Unwind all enclosing BlockStatements up to the function root
  Node *current = node.parent;
  Node *func_node = nullptr;
  while (current) {
    if (current->node_type == NodeType::BLOCK) {
      emit_cleanup_for_node(current);
    } else if (current->node_type == NodeType::METHOD_DECL ||
               current->node_type == NodeType::CONSTRUCTOR_DECL) {
      func_node = current;
      break;
    }
    current = current->parent;
  }

  // 3. Clean up the function parameters and 'this' pointer
  if (func_node) {
    emit_cleanup_for_function(func_node);
  }

  // 4. Emit the VM RETURN opcode
  emit_byte(static_cast<uint8_t>(OpCode::RETURN));
}
```

#### 4. Function Frame Cleanup Details (`emit_cleanup_for_function`)
```cpp
void Assembler::emit_cleanup_for_function(Node *func_node) {
  if (!func_node) return;
  if (func_node->node_type == NodeType::METHOD_DECL) {
    auto *m = static_cast<MethodDeclaration *>(func_node);
    bool is_instance_method = !m->is_static && m->parent != nullptr &&
                              m->parent->node_type == NodeType::CLASS_DECL;
    if (is_instance_method) {
      emit_byte(static_cast<uint8_t>(OpCode::GET_LOCAL));
      emit_int32(0);
      emit_byte(static_cast<uint8_t>(OpCode::DEC_REF));
    }
    for (const auto &param : m->parameters) {
      if (param && param->is_reference_type) {
        emit_byte(static_cast<uint8_t>(OpCode::GET_LOCAL));
        emit_int32(param->memory_index);
        emit_byte(static_cast<uint8_t>(OpCode::DEC_REF));
      }
    }
  }
}
```

---

## 2. Syntax & Grammar

### Syntax Forms
A return statement may either return a value or exit a void function:

```solix
// 1. Returning a value from non-void function
return <expression>;

// 2. Returning from void function
return;
```

### Contextual Examples
```solix
int32 compute_sum(int32 a, int32 b) {
    return a + b;
}

void process_data(bool should_abort) {
    if (should_abort) {
        return; // Early exit from void function
    }
    perform_heavy_task();
}
```

---

## 3. Underlying Systems & VM Mechanics

### Return Execution Flow
```text
[ Evaluate Return Expression ] ──► Pushes Return Value onto Stack (Top of Stack)
              │
              ▼
[ AST Parent Traversal ]       ──► For each enclosing BlockStatement:
                                   Emits GET_LOCAL + DEC_REF for all reference locals
              │
              ▼
[ Function Cleanup ]           ──► Emits GET_LOCAL 0 + DEC_REF (this pointer)
                                   Emits GET_LOCAL <slot> + DEC_REF for reference params
              │
              ▼
[ OpCode::RETURN ]             ──► Pops activation frame from call stack
                                   Restores caller's instruction pointer (ip)
                                   Caller reads return value from stack top
```

---

## 4. Positive Test Scenarios (Valid Variations)

### Scenario 4.1: Returning Primitive Values
Returning integer, floating-point, or boolean scalar values from functions.
```solix
int32 get_magic_number() {
    return 42;
}

bool is_positive(int32 x) {
    if (x > 0) {
        return true;
    }
    return false;
}
```
*Verification*: Returned values match expected scalars. Stack frame pops cleanly.

### Scenario 4.2: Returning Reference Objects
Returning a newly created heap instance or existing reference variable.
```solix
String create_greeting(String name) {
    String greeting = new String("Hello, ");
    return greeting; // Returning reference
}
```
*Verification*: Returned `String` retains an active reference count of 1. Temporary parameters are decremented without destroying the return value.

### Scenario 4.3: Early Return from Deeply Nested Blocks
Returning from inside multiple nested loops and blocks without leaking any intermediate local objects.
```solix
int32 search_matrix(int32[][] matrix, int32 target) {
    for (int32 r = 0; r < 10; r++) {
        String row_info = new String("Scanning row");
        for (int32 c = 0; c < 10; c++) {
            String cell_info = new String("Scanning cell");
            if (matrix[r][c] == target) {
                return target; // Must decrement cell_info and row_info!
            }
        }
    }
    return -1;
}
```
*Verification*: Heap verification confirms `cell_info` and `row_info` are decremented and reclaimed when the early return triggers.

### Scenario 4.4: Early Return from Void Function
Exiting early from a void function without returning a value.
```solix
void validate_and_run(bool is_ready) {
    if (!is_ready) {
        return; // Valid early return
    }
    Console.println("Running task");
}
```
*Verification*: Function exits immediately when `is_ready` is false; `"Running task"` is not printed.

---

## 5. Negative Test Scenarios (Invalid Variations)

### Scenario 5.1: Missing Return Value in Non-Void Method
Attempting to use a bare `return;` inside a method with a non-void return type.
```solix
int32 calculate() {
    return; // Error: must return a value
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Must return a value from non-void method
```

### Scenario 5.2: Incompatible Return Value Type
Returning an expression whose type is not assignable to the declared return type.
```solix
int32 get_number() {
    return "forty-two"; // Error: String cannot be converted to int32
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Return type mismatch: expected 'int32', got 'String'
```

### Scenario 5.3: Returning Value from Void Method
Attempting to return an expression from a method declared with return type `void`.
```solix
void log_status() {
    return 100; // Error: cannot return value from void method
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Return type mismatch: expected 'void', got 'int32'
```

### Scenario 5.4: Return Statement at Global File Scope
Using a `return` statement outside of any function, method, or constructor body.
```solix
package test;

int32 x = 10;
return; // Error: return outside of function body
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Return statement not allowed outside of function or method body
```
