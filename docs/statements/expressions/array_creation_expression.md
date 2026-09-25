# ArrayCreationExpression (`NodeType::ARRAY_CREATION`)

## 1. Description & Purpose

The `array creation` expression dynamically allocates a contiguous array buffer on the heap (`new Type[size]`). The VM allocates memory for the array header (element count, element size, element type VTable) and zeroes or initializes the elements. Arrays are managed as first-class reference types governed by ARC memory management.

## 2. Syntax & Grammar

```solix
'new' <element-type> '[' <size-expr> ']' ('[' <size-expr> ']')*
```

## 3. Underlying Systems & Mechanics

- Evaluates size expression (must be `int32`).
- Emits `ARRAY_ALLOC` with element count and type metadata.
- Sets array length header.
- Initializes all elements to zero or null.
- Multi-dimensional syntax compiles into nested allocation loops.

## 4. Positive Test Scenarios (Valid Variations)

1. **1D Primitive Array**: `int32[] nums = new int32[10];`
2. **Multidimensional Array**: `int32[][] grid = new int32[5][5];`
3. **Object Reference Array**: `String[] words = new String[16];`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Array Size Not int32**:
   - `int32[] arr = new int32[3.14];`  
     *Error*: `Array size must be int32`
2. **Negative Array Size (Runtime)**:
   - `int32[] arr = new int32[-5];`  
     *Runtime Exception*: `IndexOutOfBounds`
