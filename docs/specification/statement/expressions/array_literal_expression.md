# ArrayLiteralExpression

## 1. Overview & Purpose

An `ArrayLiteralExpression` (`[1, 2, 3]` or `{1, 2, 3}`) constructs and populates an array inline. The element type is inferred by unifying the element expressions.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
int32[] primes = [2, 3, 5];
```

```bytecode
// Compiled VM Bytecode
PUSH_CONST_I32 3
ALLOC_STATIC 4              // Allocate array of length 3
DUP
PUSH_CONST_I32 2
PUSH_CONST_I32 0
SET_ARRAY                   // arr[0] = 2
DUP
PUSH_CONST_I32 3
PUSH_CONST_I32 1
SET_ARRAY                   // arr[1] = 3
DUP
PUSH_CONST_I32 5
PUSH_CONST_I32 2
SET_ARRAY                   // arr[2] = 5
SET_LOCAL 1                 // primes = array pointer
```
