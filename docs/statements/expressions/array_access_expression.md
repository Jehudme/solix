# ArrayAccessExpression (`NodeType::ARRAY_ACCESS`)

## 1. Description & Purpose

The `array access` expression (`array[index]`) accesses or modifies an element in a contiguous array. It functions both as an rvalue (reading an element value onto the stack) and as an lvalue (acting as the target for an assignment). The VM performs mandatory runtime bounds checking against the array length, throwing an `IndexOutOfBoundsException` if violated.

## 2. Syntax & Grammar

```solix
<array-expr> '[' <index-expr> ']'
```

## 3. Underlying Systems & Mechanics

- Evaluates array reference. If null, VM raises `NullPointer`.
- Evaluates index (must be `int32`).
- VM executes hardware/software bounds check: verifies `0 <= index < length`.
- Emits `ARRAY_LOAD` or prepares destination for `ARRAY_STORE`.

## 4. Positive Test Scenarios (Valid Variations)

1. **1D Array Access**: `int32 val = arr[0]; arr[0] = 5;`
2. **Multidimensional Access**: `int32 cell = grid[row][col];`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Non-Integer Index**:
   - `int32 v = arr["first"];`  
     *Error*: `Array index must be int32`
2. **Indexing Non-Array**:
   - `int32 x = 42; int32 v = x[0];`  
     *Error*: `Cannot index a non-array value`
3. **Out-of-Bounds Subscript (Runtime)**:
   - `int32[] arr = new int32[3]; int32 v = arr[10];`  
     *Runtime Exception*: `IndexOutOfBounds`
