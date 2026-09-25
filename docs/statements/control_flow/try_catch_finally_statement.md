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

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Specific Catch Hierarchy
```solix
class CustomError extends std.Exception { CustomError() : super("Custom") {} }

int32 result = 0;
try {
    throw new CustomError();
} catch (CustomError c) {
    result = 1; // Matches here
} catch (std.Exception e) {
    result = 2;
}
```
*Expected Result*: `result` equals 1.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Unreachable Catch Clause
```solix
class SubErr extends std.Exception {}

void test() {
    try {
        work();
    } catch (std.Exception e) {
    } catch (SubErr s) { // Error: unreachable catch
    }
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Unreachable catch clause: 'SubErr' is already handled by preceding catch for 'std.Exception'
```
