# AliasStatement

## 1. Overview & Purpose

An `AliasStatement` (`alias NewName = TargetType;`) introduces a type synonym or parameterized generic alias into the symbol table.

In Solix:
- Aliases are transparently substituted at compile time during semantic analysis.
- They incur **zero runtime memory or boxing overhead**.
- They support generic parameterization: `alias IntMap<V> = Map<int32, V>;`.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### How It Compiles
1. The alias is recorded in Pass 1a.
2. During Pass 2, whenever the alias is referenced, the binder recursively substitutes it with the target type.
3. The compiler emits bytecode targeting the underlying target type directly.

---

## 3. Valid Test Cases (Positive Scenarios)

### Case 3.1: Primitive Synonym
```solix
alias Byte = uint8;

void test() {
    Byte b = 255;
}
```
*Expected Result*: `Byte` compiles as a raw `uint8` with zero wrapper overhead.

### Case 3.2: Parameterized Generic Alias
```solix
alias StringMap<V> = Map<String, V>;

void test() {
    StringMap<int32> map = new StringMap<int32>();
}
```
*Expected Result*: Substituted to `Map<String, int32>`; compiles cleanly.

---

## 4. Invalid Test Cases & Expected Errors (Negative Scenarios)

### Case 4.1: Circular Alias Definition
```solix
alias A = B;
alias B = A; // Error: circular alias
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Circular alias detected in 'A'
```

### Case 4.2: Generic Parameter Arity Mismatch
```solix
alias Pair<K, V> = Map<K, V>;

void test() {
    Pair<String> bad; // Error: expected 2 parameters
}
```
*Expected Compiler Diagnostic*:
```text
[ERROR] binder.cpp: Alias 'Pair' expects 2 generic type arguments, got 1
```
