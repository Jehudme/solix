# CastExpression (`NodeType::CAST_EXPR`)

## 1. Description & Purpose

The `cast` expression explicitly converts a value of one type to another target type using prefix syntax `(TargetType)expr`. Solix supports numeric conversions (e.g. `int32` to `float64`), identity casts, and object hierarchy upcasting/downcasting. Downcasting reference types emits a runtime `CAST_CHECK` opcode that validates class hierarchy conformance, throwing a `TypeCastException` on mismatch.

## 2. Syntax & Grammar

```solix
'(' <target-type> ')' <expression>
```

## 3. Underlying Systems & Mechanics

- Evaluates operand expression.
- Numeric cast emits conversion opcode (`I32_TO_I64`, `F64_TO_I32`, etc.).
- Upcast: zero runtime cost; verified by compiler via inheritance tree.
- Downcast: compiler verifies type hierarchy; runtime checks `vtable_id`.

## 4. Positive Test Scenarios (Valid Variations)

1. **Numeric Truncation & Extension**: `int32 i = (int32)3.99; float64 f = (float64)10;`
2. **Scalar Char Conversion**: `int32 code = (int32)'A'; char c = (char)65;`
3. **Class Upcast**: `Animal a = (Animal)new Dog();`
4. **Class Downcast**: `Dog d = (Dog)animal_ref;`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Cast Between Primitive and Class**:
   - `Dog d = (Dog)42;`  
     *Error*: `Cannot cast between primitive and class types`
2. **Cast Between Unrelated Classes**:
   - `Dog d = new Dog(); Engine e = (Engine)d;`  
     *Error*: `Cannot cast 'Dog' to 'Engine' - no inheritance relationship`
