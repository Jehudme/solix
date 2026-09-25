# IdentifierNode

## 1. Overview & Purpose

An `IdentifierNode` represents a named reference to a local variable, function parameter, class field, global variable, or type.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
int32 y = x;
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 1                 // Resolves 'x' to slot 1 and loads value
SET_LOCAL 2                 // Stores into 'y'
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Local Resolution Precedence
```solix
int32 val = 100;
{
    int32 val = 200;
    Console.println(val); // Resolves to local slot (200)
}
```
*Expected Result*: Prints 200.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Undefined Identifier
```solix
void test() {
    int32 a = unknown_var;
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Undefined identifier: unknown_var
```
