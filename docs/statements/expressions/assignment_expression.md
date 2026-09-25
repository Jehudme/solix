# AssignmentExpression (`NodeType::ASSIGNMENT_EXPR`)

## 1. Description & Purpose

The `assignment` expression evaluates a right-hand side (RHS) expression and stores the computed value into an lvalue storage location (local variable, class field, or array element). Solix supports standard assignment (`=`) and compound assignments (`+=`, `-=`, `*=`, `/=`). For reference types, assignment updates object ownership in ARC by incrementing the new object's reference count and decrementing the displaced object's count.

## 2. Syntax & Grammar

```solix
<lvalue> ('=' | '+=' | '-=' | '*=' | '/=' | '%=') <expression>
```

## 3. Underlying Systems & Mechanics

- Evaluates RHS expression.
- If LHS is a reference variable, emits `INC_REF` for new value and `DEC_REF` for previous occupant.
- If LHS is a static field on a qualified path (`Config.timeout = 5`), emits `SET_GLOBAL` directly without compiling the class receiver as a runtime instance value.
- If class has overloaded `operator=`, transforms assignment into method call.

## 4. Positive Test Scenarios (Valid Variations)

1. **Local Variable Assignment**: `x = 10; s = new String("new");`
2. **Compound Numeric Assignment**: `x += 5; x *= 2;`
3. **Static Class Field Assignment**: `solix.core.Config.debug = true;`
4. **Array Element Assignment**: `arr[0] = 42; matrix[1][2] = 99;`
5. **Instance Member Assignment**: `player.hp = 100;`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Assigning to Non-LValue (Literal, Binary, or Unary Expression)**:
   - `10 = x;` or `(a + b) = 5;` or `-x = 2;`  
     *Error*: Parse error: `Invalid assignment target`
2. **Assigning to Const Variable**:
   - `const int32 MAX = 10; MAX = 20;`  
     *Error*: `Cannot assign to const variable 'MAX'`
3. **Assignment Type Mismatch**:
   - `int32 x = 0; x = "str";`  
     *Error*: `Assignment type mismatch: 'int32' vs 'char[]'`
