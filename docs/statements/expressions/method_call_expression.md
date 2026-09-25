# MethodCallExpression

## 1. Overview & Purpose

A `MethodCallExpression` (`receiver.method(args)`) invokes a function or method. It performs overload resolution, prepares arguments on the operand stack, and dispatches statically or dynamically via VTables.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
calc.add(10, 20);
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 1                 // Receiver calc (this)
PUSH_CONST_I32 10           // Arg 1
PUSH_CONST_I32 20           // Arg 2
CALL_VIRTUAL <add_slot>     // Dispatches via VTable
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Overloaded Method Selection
```solix
class Printer {
    void print(int32 x) { Console.println("int"); }
    void print(String s) { Console.println("str"); }
}

void test() {
    Printer p = new Printer();
    p.print(42);       // Calls print(int32)
    p.print("hello");  // Calls print(String)
}
```
*Expected Result*: First prints `"int"`, second prints `"str"`.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: No Matching Overload
```solix
void test(Printer p) {
    p.print(true); // Error: no boolean overload
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: No matching overload for method 'print' with arguments (bool)
```
