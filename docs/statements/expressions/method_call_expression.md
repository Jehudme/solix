# MethodCallExpression (`NodeType::METHOD_CALL`)

## 1. Description & Purpose

The `method call` expression (`target.method(arg1, arg2)`) invokes a function or method. The compiler resolves the target overload based on argument types, sets up arguments on the operand stack, and emits either a direct call (for static functions / final methods) or an indirect virtual call via VTable lookup for polymorphic instance methods.

## 2. Syntax & Grammar

```solix
<callee-expr> ['<' <type-args...> '>'] '(' <arguments...> ')'
```

## 3. Underlying Systems & Mechanics

- Resolves callee signature matching argument count and types.
- If virtual, dispatches dynamically via vtable (`INVOKE_VIRTUAL`).
- If static, dispatches via `INVOKE_STATIC`.
- Template calls trigger monomorphization if concrete instantiation is not yet compiled.
- Return value pushed to operand stack.

## 4. Positive Test Scenarios (Valid Variations)

1. **Instance Method Call**: `player.take_damage(25);`
2. **Static Method Call**: `Math.max(10, 20);`
3. **Generic Method Call with Explicit Type**: `Arrays.swap<int32>(arr, 0, 1);`
4. **Generic Method Call with Implicit Deduction**: `min(10, 20);`

## 5. Negative Test Scenarios (Invalid Variations)

1. **No Matching Method Signature**:
   - `player.take_damage("twenty");`  
     *Error*: `No matching method: Player.take_damage(char[])`
2. **Calling Method on Null Reference (Runtime)**:
   - `String s = (String)null; s.size();`  
     *Runtime Exception*: `NullPointer`
