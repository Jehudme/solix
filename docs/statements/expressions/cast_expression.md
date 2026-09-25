# CastExpression

## 1. Overview & Purpose

A `CastExpression` (`(TargetType)expr`) converts types. Primitive casts emit conversion opcodes (e.g. `CONV_F64`). Reference downcasts emit `CAST_CHECK` to validate the object's VTable at runtime.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
Dog d = (Dog)animal;
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 1                 // Load animal reference
CAST_CHECK <Dog_vtable_id>  // Verifies inheritance; throws if mismatch!
SET_LOCAL 2                 // Stored into d
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Valid Downcast
```solix
Animal a = new Dog();
Dog d = (Dog)a; // Succeeds
```
*Expected Result*: `d` holds a valid reference to `Dog`.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Bad Downcast (Runtime Fault)
```solix
Animal a = new Cat();
Dog d = (Dog)a; // Fails!
```
*Expected Runtime Exception*:
```text
[FATAL VM PANIC] TypeCastException: Cannot cast 'Cat' to 'Dog'
```
