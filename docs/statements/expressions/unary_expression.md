# UnaryExpression

## 1. Overview & Purpose

A `UnaryExpression` operates on a single operand (`!`, `-`, `+`, `~`, prefix/postfix `++`, `--`). Prefix operations mutate and yield the new value; postfix operations yield the original value before mutating.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
int32 b = a++;
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 1                 // Push original 'a' (to yield as result)
DUP
INC_I64
SET_LOCAL 1                 // 'a' incremented in storage
SET_LOCAL 2                 // 'b' receives original value
```

---

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [UnaryExpression in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#unaryexpression).
