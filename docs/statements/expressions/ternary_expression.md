# TernaryExpression (`NodeType::TERNARY_EXPR`)

## 1. Description & Purpose

The `ternary` expression (`condition ? true_expr : false_expr`) is an inline conditional operator yielding one of two expressions based on the boolean outcome of a condition predicate. It evaluates lazily: only the branch corresponding to the evaluated condition is executed, with short-circuiting preventing evaluation or side effects of the unused branch.

## 2. Syntax & Grammar

```solix
<condition-expr> '?' <true-expr> ':' <false-expr>
```

## 3. Underlying Systems & Mechanics

- Evaluates condition: must evaluate strictly to `bool`.
- Emits conditional jump past true branch.
- Validates that `true-expr` and `false-expr` yield matching types.
- Evaluates only the chosen branch at runtime.

## 4. Positive Test Scenarios (Valid Variations)

1. **Primitive Numeric Ternary**: `int32 max = (a > b) ? a : b;`
2. **Reference Ternary**: `String s = (p != null) ? p.name : "Default";`
3. **Nested Ternary**: `int32 sign = (x > 0) ? 1 : ((x < 0) ? -1 : 0);`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Condition Is Not Bool**:
   - `int32 v = 1 ? 10 : 20;`  
     *Error*: `Ternary condition must be bool`
2. **Mismatched Branch Types**:
   - `auto v = (flag) ? 10 : "text";`  
     *Error*: `Ternary branches must have the same type`
3. **Missing Colon**:
   - `int32 v = (flag) ? 10;`  
     *Error*: `Expected ':' in ternary expression`
