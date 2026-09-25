# InstanceOfExpression (`NodeType::INSTANCEOF_EXPR`)

## 1. Description & Purpose

The `instanceof` expression (`expr instanceof Type`) evaluates whether an object instance conforms at runtime to a specified class or interface type. It inspects the target object's VTable ID and ancestor hierarchy table. The expression yields a boolean `true` if the instance inherits from or implements the target type, and `false` otherwise (or if the evaluated reference is `null`).

## 2. Syntax & Grammar

```solix
<expression> 'instanceof' <class-type>
```

## 3. Underlying Systems & Mechanics

- Evaluates expression. If null, evaluates to `false`.
- Reads `vtable_id` from instance word 0.
- Traverses base vtable pointers in runtime metadata table to check if target type is an ancestor.
- Leaves `bool` on stack.

## 4. Positive Test Scenarios (Valid Variations)

1. **Class Hierarchy Query**:
   ```solix
   if (entity instanceof Enemy) {
       Enemy e = (Enemy)entity;
   }
   ```

## 5. Negative Test Scenarios (Invalid Variations)

1. **Left Hand Side is Primitive**:
   - `if (10 instanceof int32)`  
     *Error*: `instanceof requires an object reference on left-hand side`
2. **Right Hand Side is Not a Class**:
   - `if (obj instanceof int32)`  
     *Error*: `Right-hand side of instanceof must be a class type`
