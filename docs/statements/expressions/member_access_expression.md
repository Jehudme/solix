# MemberAccessExpression

## 1. Overview & Purpose

A `MemberAccessExpression` (`receiver.member`) accesses fields or nested properties. Instance field offsets are resolved statically at compile time.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
int32 age = user.age;
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 1                 // user
GET_PROPERTY 0              // Reads field at offset 0
SET_LOCAL 2                 // age
```

---

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [MemberAccessExpression in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#memberaccessexpression).
