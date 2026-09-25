# FieldDeclaration (`NodeType::FIELD_DECL`)

## 1. Description & Purpose

The `field` declaration defines state storage either within a class instance (instance fields) or globally across the program / class (static fields or top-level global variables). Fields support access modifiers (`public`, `private`, `protected`), optional initializers, and memory ownership modifiers such as `weak`. Solix provides the `weak` keyword specifically for fields to prevent strong reference cycles in object graphs, automatically managing weak reference pointers in the runtime ARC engine.

## 2. Syntax & Grammar

```solix
[access-modifier] ['static'] ['const'] ['weak'] <type-info> <name> ['=' <initializer-expr>] ';'
```

## 3. Underlying Systems & Mechanics

- **Instance Fields**: Assigned memory index offsets in instance layout (`memory_index = 1 .. N`). Field initializers are compiled into bytecode and injected into constructor preambles before user code runs. Emits `SET_PROPERTY` (or `WEAK_SET_PROPERTY` for weak fields).
- **Static Fields**: Assigned global memory index in `static_variable_index`. Evaluated in global data section; accessed via `GET_GLOBAL` and `SET_GLOBAL`.
- **Top-Level Global Variables**: Can be declared directly outside classes in compilation units; stored in global memory index.
- **ARC Lifecycle**: Reference-typed fields trigger `INC_REF` upon assignment. When the parent object is destroyed (`RELEASE`), the VM iterates reference field offsets and triggers `DEC_REF`.
- **Weak Fields (`weak`)**: Non-owning pointer flag set (`is_weak = true`). Emits `WEAK_SET_PROPERTY`, which skips `INC_REF` to prevent cyclic memory leaks.

## 4. Positive Test Scenarios (Valid Variations)

1. **Instance Field Uninitialized**: `public int32 health;`
2. **Instance Field with Initializer**: `private float64 scale = 1.0;`
3. **Static Class Field**: `public static int32 count = 0;`
4. **Top-Level Global Variable**: `int32 global_counter = 42;`
5. **Const Immutable Field**: `public const int32 BUFFER_SIZE = 1024;`
6. **Weak Reference Pointer**: `protected weak TreeNode parent_node;`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Initializer Type Mismatch**:
   - `int32 score = "top";`  
     *Error*: `Type mismatch in field initialization: expected 'int32', got 'char[]'`
2. **Weak Modifier on Primitive**:
   - `weak int32 count;`  
     *Error*: `'weak' modifier only valid on reference types`
3. **Duplicate Field Name in Class**:
   - `int32 x; float64 x;`  
     *Error*: `Duplicate member 'x' in class`
4. **Accessing Uninstantiated `this` in Field Initializer**:
   - `int32 y = this.compute();`  
     *Error*: `Cannot access 'this' in field initializer`
