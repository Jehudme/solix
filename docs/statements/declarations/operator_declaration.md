# §10 OperatorDeclaration

## 1. Overview & Scope

An `OperatorDeclaration` enables operator overloading for user-defined classes, allowing instances to be used with natural mathematical and assignment syntax. Solix strictly regulates operator overloading, permitting only the canonical arithmetic operators (`+`, `-`, `*`, `/`) and the assignment operator (`=`).

Operator declarations are compiled as specialized member functions and invoked dynamically when binary expressions evaluate operands of the declaring class type.

### Syntactic Placement
An `OperatorDeclaration` is permitted only directly within class declaration bodies.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
OperatorDeclaration ::= TypeSpecifier 'operator' OperatorSymbol '(' ParameterList ')' BlockStatement
OperatorSymbol      ::= '+' | '-' | '*' | '/' | '='
```

### Canonical Code Patterns
```solix
class Complex {
    float64 re;
    float64 im;

    Complex(float64 r, float64 i) { this.re = r; this.im = i; }

    Complex operator+(Complex other) {
        return new Complex(this.re + other.re, this.im + other.im);
    }
}
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Operator Arity Invariants
- Binary arithmetic operators (`+`, `-`, `*`, `/`) must accept exactly one parameter (with the receiver `this` acting as the left-hand operand).
- The assignment operator (`=`) must accept exactly one parameter.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Expression Lowering
When the compiler encounters `a + b` where `a` is of type `Complex`:
1. Evaluates `a` onto stack.
2. Evaluates `b` onto stack.
3. Emits virtual or direct call to `Complex.operator+(Complex)`.

---

## 5. Memory Model & ARC Invariants

### 5.1 Assignment Operator ARC Balance
Overloading the assignment operator (`=`) must balance reference counts of overwritten fields within the receiver instance.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Unsupported Operator Overload
Attempting to overload unsupported operators (such as `&&`, `||`, or `[]`) is rejected.
```solix
class Test {
    bool operator&&(Test other) {} // Error: unsupported operator
}
```
*Diagnostic Message*:
```text
[ERROR] parser.cpp: Operator '&&' cannot be overloaded
```

---

## 7. Runtime Fault Conditions

### Fault 7.1: Operator Call on Null Receiver
Executing `a + b` when `a` is `null` triggers a `NullReferenceException`.

---

## 8. Conformance & Verification Examples

### Example 8.1: Vector Arithmetic Overload
```solix
Complex c1 = new Complex(1.0, 2.0);
Complex c2 = new Complex(3.0, 4.0);
Complex c3 = c1 + c2; // Invokes operator+
```
*Verification Invariant*: `c3.re` is 4.0 and `c3.im` is 6.0.
