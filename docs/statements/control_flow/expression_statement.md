# ExpressionStatement

## 1. Overview & Purpose

An `ExpressionStatement` evaluates an expression purely for its side effects—such as variable assignments, method invocations, or increment/decrement operations.

Because statement execution must keep the Virtual Machine operand stack aligned, any value produced by the expression must be consumed and discarded:
- If the expression produces a **primitive value** (e.g. `x + 1;`), the VM emits `POP` to discard the raw bits.
- If the expression produces an **unassigned reference object** (e.g. `new String("temp");` or calling a function that returns a new object without storing it), the VM emits `DEC_REF` to immediately free the temporary object so memory is not leaked.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
x = 5;                      // Assignment side-effect
Console.println("log");     // Void method invocation
new String("discard_me");   // Temporary reference discard
```

```bytecode
// Compiled VM Bytecode
// 1. x = 5;
PUSH_CONST_I32 5
SET_LOCAL 1                 // Stored into x; operand stack is balanced

// 2. Console.println("log");
PUSH_CONST_STRING 0
CALL_NATIVE Console.println // Returns void; no pop needed

// 3. new String("discard_me");
PUSH_CONST_STRING 1
CALL String.new(String)     // Pushes new heap pointer to stack
DEC_REF                     // Immediately decremented and freed!
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Method Calls and Assignments
```solix
int32 x = 0;
x = 10;
x++;
Console.println(x);
```
*Expected Result*: Evaluates side effects sequentially; stack depth remains 0 at every statement boundary.

### Case 3.2: Immediate Temporary Reclamation
```solix
String generate() { return new String("temporary"); }

void test() {
    generate(); // Call and discard
}
```
*Expected Result*: Returned `String` is decremented via `DEC_REF` immediately; 0 heap leaks.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Missing Semicolon
```solix
void test() {
    x = 10 // Missing ';'
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Syntax error: expected ';' after expression
```

### Case 4.2: Method Call on Null Reference (Runtime Fault)
```solix
void test() {
    String s = null;
    s.length(); // Null pointer dereference
}
```
*Expected Runtime Exception*:
```text
[FATAL VM PANIC] NullReferenceException: Attempted to invoke method on null object reference
```
