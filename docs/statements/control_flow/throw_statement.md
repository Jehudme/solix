# ThrowStatement

## 1. Overview & Purpose

A `ThrowStatement` (`throw expr;`) raises a runtime exception, interrupting normal execution and transitioning the VM into the exception unwinding state.

In Solix:
- The thrown value must evaluate to an instance of `std.Exception` (or a subclass). Throwing primitives is illegal.
- The throw instruction jumps directly into the containing block's **Exception Cleanup Segment**, ensuring local variables in the throwing block are deallocated prior to unwinding.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
String msg = new String("error");
throw new std.Exception("boom");
```

```bytecode
// Compiled VM Bytecode
PUSH_CONST_STRING 0
CALL std.Exception.new(String)  // Pushes Exception pointer to stack top
THROW_EXCEPTION                 // Pops exception and stores in VM active_exception
JUMP <block_cleanup_ip>         // Jump to block cleanup segment to free 'msg'
```

---

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [ThrowStatement in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#throwstatement).
