# TernaryExpression

## 1. Overview & Purpose

A `TernaryExpression` (`cond ? true_expr : false_expr`) is an inline conditional operator. It is strictly short-circuiting: only the branch matching the evaluated condition is executed.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
int32 val = flag ? 10 : 20;
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 1                 // Load flag
JUMP_IF_FALSE <false_branch>
PUSH_CONST_I32 10           // True branch
JUMP <end>
<false_branch>:
PUSH_CONST_I32 20           // False branch
<end>:
SET_LOCAL 2                 // Stored into val
```

---

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [TernaryExpression in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#ternaryexpression).
