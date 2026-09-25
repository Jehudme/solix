# TryCatchFinallyStatement

## 1. Overview & Purpose

A `TryCatchFinallyStatement` provides structured error recovery (`catch`) and guaranteed finalization (`finally`). Protected code in the `try` block routes exceptions to a typed catch matching table, while `finally` executes unconditionally even on early returns.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
try {
    dangerous();
} catch (std.Exception e) {
    recover();
}
```

```bytecode
// Compiled VM Bytecode
// --- Try Block ---
CALL dangerous()
JUMP <try_end_ip>               // Normal exit jumps over catch table

// --- Catch Matching Table (<catch_start_ip>) ---
GET_EXCEPTION                   // Push active exception
INSTANCEOF <std.Exception_vtable_id>
JUMP_IF_FALSE <unhandled_ip>    // Next catch or outer cleanup
GET_EXCEPTION
SET_LOCAL 1                     // Store into catch parameter 'e'
CLEAR_EXCEPTION                 // Clear active exception register
CALL recover()
GET_LOCAL 1                     // Clean up catch parameter
DEC_REF
JUMP <try_end_ip>

// --- End (<try_end_ip>) ---
```

---

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [TryCatchFinallyStatement in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#trycatchfinallystatement).
