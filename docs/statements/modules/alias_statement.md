# §3 AliasStatement

## 1. Overview & Scope

An `AliasStatement` introduces a type synonym or a parameterized generic alias into the symbol table. It allows programmers to define concise, meaningful aliases for complex types, long generic specializations, or platform-specific primitive types.

In Solix's compiler pipeline, aliases are transparently substituted during semantic analysis (Pass 2: Type Checking and Resolution). Aliases are not new types; they share identical type identities and memory layouts with their underlying target types, incurring zero runtime performance overhead and requiring no boxing or wrapper objects.

### Syntactic Placement
An `AliasStatement` is legally permitted at top-level translation-unit scope (global alias) or within class and interface declaration bodies (member alias).

---

## 2. Syntax & Production Rules

### Production Rules
```solix
AliasStatement   ::= 'alias' Identifier ('<' TypeParameterList '>')? '=' TypeSpecifier ';'
TypeParameterList::= Identifier (',' Identifier)*
```

### Canonical Code Patterns
```solix
// 1. Primitive Type Synonym
alias Byte = uint8;
alias Real = float64;

// 2. Concrete Generic Specialization
alias StringList = List<String>;
alias CoordinateMap = Map<int32, int32>;

// 3. Parameterized Generic Alias
alias IntMap<V> = Map<int32, V>;
alias Callback<T> = Function<T, void>;
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Symbol Table Binding & Substitution
- The alias identifier is registered in the symbol table during Pass 1a (`REGISTER_GLOBALS`).
- When the binder encounters an alias during type resolution (`resolve_type`), it recursively substitutes the alias with its underlying `TypeSpecifier`.
- For parameterized aliases (`alias IntMap<V> = Map<int32, V>;`), the binder verifies that type arguments provided at the usage site match the declared generic parameter count and substitutes generic parameters accordingly.

### 3.2 Circular Alias Detection
- Aliases cannot be recursively or cyclically defined (e.g. `alias A = B; alias B = A;`). The compiler maintains a cycle-detection set during alias resolution and reports a diagnostic if a cycle is detected.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Normal Completion
- An `AliasStatement` is a purely static compile-time declaration.
- It produces **zero bytecode opcodes** and emits no runtime dispatch or metadata.
- After type substitution in Pass 2, the compiler treats all usages of the alias identically to the target type.

---

## 5. Memory Model & ARC Invariants

### 5.1 Inherited Target Memory Layout
- An alias possesses the exact memory layout, alignment, and ARC reference-counting rules of its resolved target type:
  - If target is primitive (e.g. `alias Byte = uint8;`), instances are stored inline as value types with 0 ARC overhead.
  - If target is a reference type (e.g. `alias UserList = List<User>;`), instances are tracked via ARC (`INC_REF` / `DEC_REF`) identically to `List<User>`.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Circular Alias Definition
Defining an alias that directly or indirectly references itself is illegal.
```solix
alias Alpha = Beta;
alias Beta = Alpha; // Error: circular alias
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Circular alias detected in 'Alpha'
```

### Rule 6.2: Generic Parameter Count Mismatch
Using a generic alias with an incorrect number of type arguments is rejected.
```solix
alias Pair<K, V> = Map<K, V>;

void test() {
    Pair<String> invalid; // Error: expected 2 type arguments
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Alias 'Pair' expects 2 generic type arguments, got 1
```

---

## 7. Runtime Fault Conditions

An `AliasStatement` generates no dynamic runtime faults; substitution is performed entirely at compile time.

---

## 8. Conformance & Verification Examples

### Example 8.1: Generic Parameterized Alias Substitution
```solix
alias IntMap<V> = Map<int32, V>;

void verify_alias() {
    IntMap<String> cache = new IntMap<String>();
    cache.put(1, "one");
    String val = cache.get(1);
}
```
*Verification Invariant*: The compiler substitutes `IntMap<String>` with `Map<int32, String>`. Bytecode instantiation and method invocations target `Map` directly.
