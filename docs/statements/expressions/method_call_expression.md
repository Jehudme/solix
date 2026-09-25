# §35 MethodCallExpression

## 1. Overview & Scope

A `MethodCallExpression` (`target.method(args)`) invokes a function or method. The compiler performs overload resolution, prepares arguments on the operand stack, and emits either direct call opcodes (for static/final methods) or virtual dynamic dispatch via VTable lookup.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
MethodCallExpression ::= (Expression '.')? Identifier '(' ArgumentList? ')'
ArgumentList         ::= Expression (',' Expression)*
```

---

## 3. Scope & Declaration Space (Static Semantics)

Overload resolution selects the most specific method matching the argument types.

---

## 4. Operational Semantics (Dynamic Execution)

1. Evaluates receiver (for instance methods). Checks for null.
2. Evaluates arguments in left-to-right order.
3. Dispatches via direct call or VTable slot lookup.
4. Pops arguments, allocates callee frame, executes method body.
5. Pushes return value (if non-void) to stack top.

---

## 5. Memory Model & ARC Invariants

Arguments are decremented upon callee return. Return value is preserved on stack.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: No Matching Overload
```solix
void print(int32 x) {}
print("hello"); // Error: no matching overload
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: No matching overload for method 'print' with arguments (String)
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Method Call on Null
```solix
String s = null;
s.length(); // Throws NullReferenceException
```
*Runtime Fault*:
```text
[FATAL VM PANIC] NullReferenceException: Attempted to invoke method on null object reference
```

---

## 8. Conformance & Verification Examples

### Example 8.1: Virtual Dynamic Dispatch
```solix
Animal a = new Dog();
a.speak(); // Dispatches dynamically to Dog.speak
```
*Verification Invariant*: Invokes `Dog.speak` via VTable slot.
