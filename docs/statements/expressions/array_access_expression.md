# ArrayAccessExpression

## 1. Overview & Purpose

An `ArrayAccessExpression` (`arr[index]`) accesses or modifies an element in an array. The VM enforces runtime bounds checking, raising `IndexOutOfBoundsException` on violation.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
int32 val = arr[2];
arr[2] = 50;
```

```bytecode
// Compiled VM Bytecode
// Read
GET_LOCAL 1                 // arr
PUSH_CONST_I32 2            // index
GET_ARRAY                   // Bounds checks & reads element
SET_LOCAL 2                 // val

// Write
PUSH_CONST_I32 50           // value
GET_LOCAL 1                 // arr
PUSH_CONST_I32 2            // index
SET_ARRAY                   // Bounds checks & writes element
```

---

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [ArrayAccessExpression in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#arrayaccessexpression).
