# OperatorDeclaration

## 1. Overview & Purpose

An `OperatorDeclaration` allows user-defined classes to overload arithmetic operators (`+`, `-`, `*`, `/`) and assignment (`=`). Binary operator methods take one parameter, with `this` representing the left-hand operand.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### How It Compiles
When `v1 + v2` is evaluated where `v1` is of class type `Vector`:
```bytecode
GET_LOCAL 1                 // Load v1 (LHS)
GET_LOCAL 2                 // Load v2 (RHS parameter)
CALL_VIRTUAL <operator+_slot>
```

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Vector Addition Overload
```solix
class Vector {
    int32 x;
    Vector(int32 x) { this.x = x; }
    Vector operator+(Vector other) {
        return new Vector(this.x + other.x);
    }
}

void test() {
    Vector v1 = new Vector(10);
    Vector v2 = new Vector(20);
    Vector v3 = v1 + v2; // Calls operator+
}
```
*Expected Result*: `v3.x` equals 30.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Unsupported Operator Overload
```solix
class Test {
    bool operator&&(Test other) {} // Error: unsupported
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] parser.cpp: Operator '&&' cannot be overloaded
```
