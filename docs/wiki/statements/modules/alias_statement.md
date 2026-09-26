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
