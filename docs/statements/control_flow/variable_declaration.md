# VariableDeclarationStatement (`NodeType::VAR_DECL`)

## 1. Description & Purpose

The `variable declaration` statement introduces one or more named variables into the current lexical scope, specifying their type and optional initial value. In Solix, local variables are allocated stack frame slots (registers/local indices), while global variables are stored in the global data segment. Variables holding reference types (e.g. classes, arrays) participate in ARC tracking, receiving an initial reference increment upon assignment and decrement upon scope termination.

## 2. Syntax & Grammar

```solix
['const'] <type-info> <name> ['=' <initializer-expr>] ';'
```

## 3. Underlying Systems & Mechanics

- Registers variable in active lexical `current_scope`.
- Assigns stack register index `local_variable_index++`.
- Evaluates initializer expression and emits store opcode (`SET_LOCAL`).
- Enforces reference counting rules if type is class or array (`is_reference_type`).
- Enforces variable shadowing checks: scans parent scopes and emits diagnostic warning if inner variable hides an outer name.

## 4. Positive Test Scenarios (Valid Variations)

1. **Uninitialized Primitive**: `int32 x;` (zero-initialized)
2. **Initialized Primitive**: `float64 f = 3.14159;`
3. **Reference Allocation**: `String s = new String("hello");`
4. **Const Immutable Binding**: `const int32 MAX_USERS = 500;`
5. **Reference Parameter Binding**: `int32& ref_target;`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Type Mismatch in Initialization**:
   - `int32 x = "text";`  
     *Error*: `Type mismatch in variable declaration: expected 'int32', got 'char[]'`
2. **Uninitialized Const Variable**:
   - `const int32 LIMIT;`  
     *Error*: `Const variable 'LIMIT' must have an initializer`
3. **Duplicate Variable in Same Scope**:
   - `int32 count = 1; int32 count = 2;`  
     *Error*: `Duplicate local variable 'count'`
4. **Variable Shadowing Warning**:
   - Outer: `int32 temp = 1;` Inner: `int32 temp = 2;`  
     *Warning*: `[warning] Variable 'temp' shadows a variable in an outer scope`
