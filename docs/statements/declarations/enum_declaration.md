# EnumDeclaration

## 1. Overview & Purpose

An `EnumDeclaration` defines a discrete set of named constant values backed by 64-bit integer values (`int64`). Enums are value types: they incur **zero ARC overhead** and require no heap allocations.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### How It Compiles
Members evaluate to constant integer push instructions:
```solix
// Solix Code
enum Status { PENDING, ACTIVE, DONE }
Status s = Status.ACTIVE;
```

```bytecode
// Compiled VM Bytecode
PUSH_CONST_I64 1            // Status.ACTIVE is ordinal 1
SET_LOCAL 1                 // Stored into 's' (value type, 0 ARC)
```

---

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [EnumDeclaration in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#enumdeclaration).
