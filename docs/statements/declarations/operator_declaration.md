# OperatorDeclaration

## 1. Overview & Purpose

An `OperatorDeclaration` allows user-defined classes to overload arithmetic operators (`+`, `-`, `*`, `/`) and assignment (`=`). Binary operator methods take one parameter, with `this` representing the left-hand operand.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### How It Compiles
When `v1 + v2` is evaluated where `v1` is of class type `Vector`:
```bytecode
GET_LOCAL 1                 // Load v1 (LHS)
GET_LOCAL 2                 // Load v2 (RHS parameter)
CALL_VIRTUAL <operator+_slot>
```

---

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [OperatorDeclaration in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#operatordeclaration).
