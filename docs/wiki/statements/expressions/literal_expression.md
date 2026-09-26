# LiteralNode

## 1. Overview & Purpose

A `LiteralNode` represents a constant scalar or string literal embedded directly in source code (`42`, `3.14`, `'c'`, `"string"`, `true`, `false`, `null`).

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
int32 i = 42;
float64 f = 3.14;
bool b = true;
String s = "text";
Object o = null;
```

```bytecode
// Compiled VM Bytecode
PUSH_CONST_I32 42
SET_LOCAL 1
PUSH_CONST_F64 3.14
SET_LOCAL 2
PUSH_TRUE
SET_LOCAL 3
PUSH_CONST_STRING 0         // Static string pool index
SET_LOCAL 4
PUSH_NULL
SET_LOCAL 5
```
