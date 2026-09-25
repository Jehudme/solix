# §36 IdentifierNode

## 1. Overview & Scope

An `IdentifierNode` represents a symbolic name reference within source code, addressing a local variable, parameter, class field, global variable, function, or type name.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
Identifier ::= [a-zA-Z_][a-zA-Z0-9_]*
```

---

## 3. Scope & Declaration Space (Static Semantics)

Resolved against the active lexical scope hierarchy:
1. Local variables and parameters.
2. Instance/static class fields.
3. Global package variables.

---

## 4. Operational Semantics (Dynamic Execution)

- Local variable/parameter: Emits `OpCode::GET_LOCAL <memory_index>`.
- Global variable: Emits `OpCode::GET_GLOBAL <global_index>`.
- Instance field: Emits `OpCode::GET_PROPERTY <offset>`.

---

## 5. Memory Model & ARC Invariants

Pushes value or reference pointer to operand stack.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Undefined Identifier
```solix
int32 x = undeclared_variable; // Error
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Undefined identifier: undeclared_variable
```

---

## 7. Runtime Fault Conditions

None under normal operation.

---

## 8. Conformance & Verification Examples

### Example 8.1: Local Scope Precedence Over Field
```solix
class Test {
    int32 x = 10;
    void run() {
        int32 x = 20;
        Console.println(x); // Resolves to local x (20)
    }
}
```
*Verification Invariant*: Outputs 20; resolves to local slot.
