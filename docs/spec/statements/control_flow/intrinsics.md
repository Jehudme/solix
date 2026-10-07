# Language Intrinsics (`assert`, `exit`)

## 1. Overview & Purpose

Solix provides two first-class language intrinsics: `assert` and `exit`.

### `assert`
The `assert` statement validates program invariants and preconditions during execution. It supports two syntactic forms:
1. `assert condition;` — Evaluates boolean `condition`. If `false`, throws an unhandled `AssertionError` runtime exception with default message `"Assertion failed"`.
2. `assert condition : message;` — Evaluates boolean `condition`. If `false`, evaluates `message` (which must be a string) and throws `AssertionError` runtime exception with that custom message.

When `condition` evaluates to `true`, the assertion completes silently with zero stack pollution.

### `exit`
The `exit` statement immediately terminates process execution with an integer exit code:
- `exit(code);` — Evaluates integer expression `code` and invokes the operating system process exit routine (`std::exit`), immediately halting the VM.

Both statements are statically type-checked by the compiler binder:
- The assertion condition must be type `bool`.
- The optional assertion message must be type `String` (or string literal `char[]`).
- The exit code expression must be an integer primitive (`int8`, `int16`, `int32`, `int64`, `uint8`, `uint16`, `uint32`, `uint64`).

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### 2.1 `assert` Compilation

When compiling `assert condition;` or `assert condition : message;`:
1. The condition expression is compiled and evaluated onto the operand stack.
2. If a message expression is present, the message is evaluated onto the stack next.
3. The compiler emits `OpCode::ASSERT` (`0x88`) with an immediate byte:
   - `0x00`: No message provided. The VM pops the boolean condition; if 0, throws `AssertionError: Assertion failed`.
   - `0x01`: Custom message provided. The VM pops the string reference and the boolean condition; if 0, throws `AssertionError` with the message. If the condition passed, the message reference is decremented (`DEC_REF`) and discarded.

#### Bytecode Disassembly Example
```solix
// Solix Code
int32 testAssert(bool valid) {
    assert valid : "invalid state";
    return 1;
}
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 0                 // Load 'valid' (bool)
PUSH_CONST_STR "invalid state"
ASSERT 1                    // Checks bool; if false, throws AssertionError with message
PUSH_CONST_I32 1
RETURN
```

### 2.2 `exit` Compilation

When compiling `exit(code);`:
1. The integer expression `code` is compiled onto the operand stack.
2. The compiler emits `OpCode::EXIT` (`0x89`).
3. At runtime, the VM pops the integer value from the stack and calls the platform exit routine `std::exit(code)`.

#### Bytecode Disassembly Example
```solix
// Solix Code
void terminate(int32 status) {
    exit(status);
}
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 0                 // Load 'status' (int32)
EXIT                        // Exits process immediately with status code
```
