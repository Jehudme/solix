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

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [IdentifierNode in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#identifiernode).
