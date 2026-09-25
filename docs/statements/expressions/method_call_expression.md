# MethodCallExpression

## 1. Overview & Purpose

A `MethodCallExpression` (`receiver.method(args)`) invokes a function or method. It performs overload resolution, prepares arguments on the operand stack, and dispatches statically or dynamically via VTables.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
calc.add(10, 20);
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 1                 // Receiver calc (this)
PUSH_CONST_I32 10           // Arg 1
PUSH_CONST_I32 20           // Arg 2
CALL_VIRTUAL <add_slot>     // Dispatches via VTable
```

---

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [MethodCallExpression in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#methodcallexpression).
