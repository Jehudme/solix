# InstanceOfExpression

## 1. Overview & Purpose

An `InstanceOfExpression` (`obj instanceof TargetType`) queries whether an instance inherits from or implements a type. Returns `true` on match, and `false` otherwise (or if `obj == null`).

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
bool is_dog = a instanceof Dog;
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 1
INSTANCEOF <Dog_vtable_id>  // Pushes bool to stack
SET_LOCAL 2
```

---

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [InstanceOfExpression in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#instanceofexpression).
