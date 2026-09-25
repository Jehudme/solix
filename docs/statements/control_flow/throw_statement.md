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

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Throw Handled by Catch
```solix
bool caught = false;
try {
    throw new std.Exception("test");
} catch (std.Exception e) {
    caught = true;
}
```
*Expected Result*: `caught` is `true`.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Throwing Primitive Value
```solix
void test() {
    throw 404; // Error: cannot throw primitive
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Cannot throw type 'int32': must inherit from 'std.Exception'
```

### Case 4.2: Throwing Null Reference at Runtime
```solix
void test() {
    std.Exception e = null;
    throw e; // Throws NullReferenceException
}
```
*Expected Runtime Exception*:
```text
[FATAL VM PANIC] NullReferenceException: Attempted to throw null exception reference
```
