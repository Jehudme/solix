# ReturnStatement (`NodeType::RETURN_STMT`)

## 1. Description & Purpose

The `return` statement terminates execution of the current function or method, optionally yields a return value back to the caller, and unwinds the function activation frame. Before executing the VM `RET` opcode, the compiler emits comprehensive ARC decrements for all active local variables in all enclosing scopes of the function, ensuring that objects do not leak when returning early from deep blocks.

## 2. Syntax & Grammar

```solix
'return' [<expression>] ';'
```

## 3. Underlying Systems & Mechanics

- **Return Value Preservation**:
  - If returning a reference object, the compiler compiles the expression into a designated return register and increments its reference count (`INC_REF`).
- **Activation Frame Unwinding**:
  - Emits `DEC_REF` / `RELEASE` for all local variables across all active lexical blocks up to function root.
- Emits `RET` opcode.

## 4. Positive Test Scenarios (Valid Variations)

1. **Void Function Return**: `void test() { return; }`
2. **Value Return**: `int32 add(int32 a, int32 b) { return a + b; }`
3. **Reference Object Return (ARC Safe)**:
   ```solix
   String create() {
       String s = new String("safe");
       return s; // Safely returned without dangling pointer
   }
   ```

## 5. Negative Test Scenarios (Invalid Variations)

1. **Returning Value from Void Function**:
   - `void test() { return 42; }`  
     *Error*: `Cannot return a value from void function`
2. **Returning Void from Non-Void Function**:
   - `int32 test() { return; }`  
     *Error*: `Expected return value of type 'int32'`
3. **Type Mismatch in Return**:
   - `int32 test() { return "hello"; }`  
     *Error*: `Type mismatch in return statement: expected 'int32', got 'char[]'`
