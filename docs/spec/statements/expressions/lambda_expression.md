# Lambda Expressions & Closures

## 1. Overview & Purpose

A `LambdaExpression` (`(params) => expr` or `(params) => { ... }`) defines an anonymous function. If the lambda references local variables from its enclosing scope, it automatically creates a closure, capturing those variables into an ARC-managed heap environment.

### Syntax
- Expression body:
  ```solix
  int32(*)(int32) sq = (x) => x * x;
  ```
- Block body:
  ```solix
  int32(*)(int32, int32) add = (a, b) => {
      return a + b;
  };
  ```
- Capturing closure:
  ```solix
  int32 factor = 5;
  int32(*)(int32) multiplier = (x) => x * factor;
  ```

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Stateless Lambdas vs Closures
1. **Stateless Lambdas (Zero Captures)**:
   - Compiled as synthetic static methods (`__lambda_N`).
   - Coerced directly into function pointers (`<ret>(*)(<params>)`).
   - No heap allocation; executes identically to static functions.

2. **Closures (Stateful with Captures)**:
   - The compiler synthesizes an environment struct or captures array allocated via `ALLOC_DYNAMIC`.
   - Captured variables are pushed onto the operand stack and packed into the environment using `PACK_CLOSURE`.
   - Captured reference types have their reference count incremented (`INC_REF`).
   - When the closure executes, `UNPACK_CAPTURES` restores the captured values into the callee frame.
   - When the closure object is released, `DEC_REF_CALLABLE` recursively decrements captured reference variables.

### Bytecode Disassembly Example
```solix
// Solix Code
int32 offset = 100;
int32(*)(int32) adder = (n) => n + offset;
int32 result = adder(23);
```

```bytecode
// Compiled VM Bytecode
PUSH_CONST_I32 100                      // Local variable offset = 100
SET_LOCAL 0

// Pack closure environment
GET_LOCAL 0                             // Load offset to capture
PUSH_CONST_I32 1                        // 1 capture variable
PUSH_CONST_I32 <__lambda_0_ip>          // Synthetic lambda code address
PACK_CLOSURE                            // Allocates heap closure object with captures
SET_LOCAL 1                             // Store closure handle into 'adder'

// Invocation
PUSH_CONST_I32 23                       // Push argument 'n'
GET_LOCAL 1                             // Load callable closure handle
PUSH_CONST_I32 1                        // Argument count
CALL                                    // Dispatches callable; unpacks environment into frame
```

---

## 3. ARC Memory Semantics for Closures
- Closures are first-class heap objects managed by Automatic Reference Counting (ARC).
- Copying a closure variable emits `INC_REF_CALLABLE`.
- Exiting scope or reassigning a closure variable emits `DEC_REF_CALLABLE`.
- Destructors recursively clean up captured objects, preventing leaks while maintaining predictable destruction order.
