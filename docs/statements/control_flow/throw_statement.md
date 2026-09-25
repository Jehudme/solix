# ThrowStatement (`NodeType::THROW_STMT`)

## 1. Description & Purpose

The `throw` statement raises an exception, disrupting normal sequential execution and initiating exception propagation. The thrown value must be an instance of a class that inherits from `std.Exception` (or a subclass thereof). The runtime VM searches active exception handler tables across the call stack to find a matching `catch` block, unwinding activation frames and invoking ARC cleanups.

## 2. Syntax & Grammar

```solix
'throw' <expression> ';'
```

## 3. Underlying Systems & Mechanics

- Evaluates expression (must inherit from `Exception`).
- Emits `THROW` opcode.
- VM intercepts thrown object, records active exception, and begins unwinding activation frames.
- Local variables in intermediate activation frames have their ARC counts decremented during stack unwinding until a matching `catch` handler or top-level crash is reached.

## 4. Positive Test Scenarios (Valid Variations)

1. **Throwing Instantiated Exception**: `throw new NullPointerException("Null ref");`
2. **Rethrowing Caught Exception**: `catch (Exception e) { throw e; }`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Throwing Primitive Integer / Float**:
   - `throw 404;`  
     *Error*: `Can only throw instances of Exception or its subclasses`
2. **Throwing String Literal**:
   - `throw "Fatal error";`  
     *Error*: `Can only throw instances of Exception or its subclasses`
3. **Throwing Unrelated Class**:
   - `class Node {} throw new Node();`  
     *Error*: `Can only throw instances of Exception or its subclasses`
