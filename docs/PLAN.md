# Solix Language — Improvement & Polish Plan

> Synthesized from two independent code reviews (scores: 72/100 and 92/100).  
> All issues, bugs, missing features, and documentation gaps are tracked here.

---

## Workflow Rules (MANDATORY for every phase)

Each phase **must** follow this exact loop, in order. Do **not** skip steps.

1. **Create a dedicated Git branch** for the phase: `git checkout -b phase-N-short-name`
2. **Implement** the feature or fix.
3. **Commit** the implementation: `git commit -m "feat/fix: ..."`
4. **Add or update unit tests** (Catch2, in `tests/src/`).
5. **Commit** the tests: `git commit -m "test: ..."`
6. **Add or update `tests/resources/test.slx`** — always **append** new Solix code on top of existing content. **Never remove, simplify, or reorganize existing code.**
7. **Commit** the test.slx update: `git commit -m "test(integration): ..."`
8. **Compile and run** `test.slx` end-to-end:
   ```bash
   ./launcher/solix_launcher compile ../tests/resources/test.slx -o compiled.slxb && \
   ./launcher/solix_launcher run compiled.slxb HELLO_SOLIX ARG2
   ```
9. **Commit** the verified result: `git commit -m "chore: verify phase N integration test passes"`
10. **Merge** the branch into `master` via `git merge --no-ff`.

> **Rule:** `test.slx` is a living document. Every phase adds to it. It must always compile and run successfully at the end of every phase.

---

## Phase Overview (ordered: most critical → least critical, easiest → hardest)

| # | Name | Branch | Criticality | Difficulty |
|---|---|---|---|---|
| 1 | Hotfix: Code Hygiene & Undefined Behaviour | `phase-1-hotfix` | 🔴 Critical | ⭐ Easy |
| 2 | VM Safety: Bounds Checking | `phase-2-vm-safety` | 🔴 Critical | ⭐⭐ Medium |
| 3 | ARC: Wire Emit in Assembler | `phase-3-arc-emit` | 🔴 Critical | ⭐⭐ Medium |
| 4 | Cast Safety: Class Type Validation | `phase-4-cast-safety` | 🟠 High | ⭐⭐ Medium |
| 5 | Access Modifier Enforcement | `phase-5-access-control` | 🟠 High | ⭐⭐ Medium |
| 6 | Error Recovery: Multi-Diagnostic Parser | `phase-6-error-recovery` | 🟠 High | ⭐⭐⭐ Hard |
| 7 | ARC: Cycle Detection / Weak References | `phase-7-arc-cycles` | 🟠 High | ⭐⭐⭐ Hard |
| 8 | Static Dispatch `::` Fully Wired | `phase-8-scope-resolution` | 🟡 Medium | ⭐⭐ Medium |
| 9 | Abstract Methods & Interfaces | `phase-9-interfaces` | 🟡 Medium | ⭐⭐⭐ Hard |
| 10 | RTTI & Safe Downcast | `phase-10-rtti` | 🟡 Medium | ⭐⭐⭐ Hard |
| 11 | AST: Visitor Pattern Refactor | `phase-11-visitor` | 🟡 Medium | ⭐⭐⭐⭐ Very Hard |
| 12 | README & Full Documentation | `phase-12-docs` | 🟢 Polish | ⭐ Easy |
| 13 | Test Coverage Expansion | `phase-13-tests` | 🟢 Polish | ⭐⭐ Medium |
| 32 | Runtime VM & Memory Safety | `phase-32-runtime-vm-memory` | 🔴 Critical | ⭐⭐ Medium |
| 33 | Assembler CodeGen & ARC Safety | `phase-33-assembler-codegen-arc` | 🔴 Critical | ⭐⭐⭐ Hard |
| 34 | Semantic Analysis & Type System Binding | `phase-34-binder-types-vtables` | 🔴 Critical | ⭐⭐⭐ Hard |

---

## Phase 1 — Hotfix: Code Hygiene & Undefined Behaviour

**Branch:** `phase-1-hotfix`
**Criticality:** 🔴 Critical
**Difficulty:** ⭐ Easy

### 1.1 — Duplicate `#include <iostream>` in `runtime.cpp`
- **File:** `language/src/processes/runtime.cpp`
- **Problem:** `#include <iostream>` appears twice at the top (added during debug sessions).
- **Fix:** Remove the duplicate.

### 1.2 — Dead debug comments left in `runtime.cpp`
- **File:** `language/src/processes/runtime.cpp`
- **Problem:** Lines like `// std::cout << "SET_VTABLE obj=" ...` are commented-out debug prints that should not be committed.
- **Fix:** Delete all commented-out debug `std::cout` lines.

### 1.3 — `VariableDeclaration` has `is_virtual` and `vtable_index`
- **File:** `language/include/solix/statements.hpp`
- **Problem:** `VariableDeclaration` contains `is_virtual`, `is_override`, and `vtable_index` — copy-pasted from `MethodDeclaration` by mistake. These fields are semantically meaningless on a variable.
- **Fix:** Remove those three fields from `VariableDeclaration` entirely.

### 1.4 — Replace `std::memcpy` bit-casts with `std::bit_cast` (C++20)
- **Files:** `language/include/solix/runtime.hpp`, `language/src/processes/runtime.cpp`
- **Problem:** `std::memcpy(&val, &value, 8)` is used to reinterpret `double` as `uint64_t`. C++20 provides `std::bit_cast<uint64_t>(value)` which is type-safe and constexpr.
- **Fix:** Replace all `memcpy`-based bit casts with `std::bit_cast`. Requires `#include <bit>` and verifying CMake targets C++20.

### 1.5 — `global_native_registry` is a file-level global
- **File:** `language/src/processes/runtime.cpp`
- **Problem:** `static std::unordered_map<std::string, NativeFunction> global_native_registry` is shared across all `RuntimeContext` instances. This makes unit testing impossible (state leaks between tests) and is a known global-state anti-pattern.
- **Fix:** Move the registry into `RuntimeContext` itself, or thread it through `RuntimeOptions`. The `register_native_function()` free function should register into the options struct, not a file-scope static.

### 1.6 — CALL_VIRTUAL frame setup bypasses the Frame stack
- **File:** `language/src/processes/runtime.cpp`
- **Problem:** The `CALL_VIRTUAL` handler directly manipulates `memory.stack_pointer` with bare arithmetic instead of using the same `Frame` push mechanism as `CALL`. When the virtual method executes `RETURN`, it restores the wrong frame pointer, corrupting caller locals. This silently produces wrong values or crashes on nested virtual calls.
- **Fix:** Unify `CALL_VIRTUAL` with `CALL`. The only difference should be how the target IP is resolved (from the vtable rather than the stack). Frame push/pop must be identical.

---

## Phase 2 — VM Safety: Bounds Checking

**Branch:** `phase-2-vm-safety`
**Criticality:** 🔴 Critical
**Difficulty:** ⭐⭐ Medium

### Context
The VM assumes bytecode is perfectly valid. A corrupted `.slxb` file, an assembler bug, or malformed input causes silent segfaults instead of clean errors.

### 2.1 — Stack underflow and overflow checks
- **Fix:** In `push()`, assert `stack_pointer < stack.size()`. In `pop()`, assert `stack_pointer > 0`. Throw `std::runtime_error` with a clear message on violation.

### 2.2 — Heap bounds on `GET_PROPERTY` / `SET_PROPERTY`
- **Fix:** Before `heap_data[obj + offset]`, verify `(obj + offset) < heap.size()`. Throw on violation.

### 2.3 — Frame bounds on `GET_LOCAL` / `SET_LOCAL`
- **Fix:** Before accessing `stack[frame_pointer + index]`, verify the index is within the current frame's allocated `frame_size`. Throw on violation.

### 2.4 — Unknown opcode guard
- **Fix:** Add a `default:` case to the main dispatch switch that throws `std::runtime_error("Unknown opcode: " + std::to_string(op))` instead of silently falling through.

### 2.5 — VTable ID validation in `CALL_VIRTUAL`
- **Fix:** Before `vtables[vtable_id][vtable_index]`, verify the ID exists in the map and the index is in range. Throw a clear error if not.

### Test additions
- Unit test: push to capacity then push once more → expect exception.
- Unit test: pop from empty stack → expect exception.
- Unit test: `GET_PROPERTY` with out-of-bounds offset → expect exception.

---

## Phase 3 — ARC: Wire Emit in Assembler

**Branch:** `phase-3-arc-emit`
**Criticality:** 🔴 Critical
**Difficulty:** ⭐⭐ Medium

### Context
The VM has `INC_REF` and `DEC_REF` opcodes and `Memory::increase_reference` / `decrease_reference` work correctly — but the assembler **never emits these opcodes**. Every `new Class()` permanently leaks its allocation. The ARC system is completely inert.

### 3.1 — Emit `INC_REF` when storing a reference-type value
- **When:** Any `SET_LOCAL`, `SET_GLOBAL`, or `SET_PROPERTY` that stores a reference-type value (class instance or array) must emit `INC_REF` on the incoming address before the store.
- **How:** The binder already sets `TypeInfo::is_reference_type`. Use that flag in the assembler.

### 3.2 — Emit `DEC_REF` when a local variable goes out of scope
- **When:** At the end of a `BlockStatement`, for every local variable in that block that is a reference type, emit `GET_LOCAL` + `DEC_REF`.
- **How:** Track which locals in the current block are reference types during `compile_block`. At block exit, iterate in reverse and emit the pair.

### 3.3 — Emit `DEC_REF` on field overwrite
- **When:** Before overwriting a reference-type field, load the old value and emit `DEC_REF`, then store the new value and emit `INC_REF`.

### Test additions
- Unit test (assembler): Verify `INC_REF`/`DEC_REF` opcodes appear in output for reference-type assignments.
- Unit test (runtime): Allocate an object, let it leave scope, verify `currently_used_words` decrements.
- `test.slx`: Add a scope block that allocates objects and verify the program completes without heap overflow.

---

## Phase 4 — Cast Safety: Class Type Validation

**Branch:** `phase-4-cast-safety`
**Criticality:** 🟠 High
**Difficulty:** ⭐⭐ Medium

### Context
`(Dog)animal` compiles without error and emits zero bytecode. No compile-time or runtime check exists for class-type casts.

### 4.1 — Binder: validate class casts at compile time
- **Rules:**
  - Primitive ↔ class cast → compile error.
  - **Upcast** (child → parent): always allowed, no warning.
  - **Downcast** (parent → child): allowed, emit a NOTE that it is unchecked until Phase 10.
  - **Unrelated classes**: compile error — `"Cannot cast 'Dog' to 'Engine': no inheritance relationship"`.
- **How:** Reuse the `is_assignable` inheritance walk added in Phase 4B.

### 4.2 — Assembler: correct handling for valid class casts
- For a valid class-type cast, emit nothing (the pointer is unchanged). Ensure the `else if` chain still reaches `break` so the stack is not corrupted.

### 4.3 — Add `CAST_CHECK` opcode (stub for Phase 10)
- Add `CAST_CHECK` to `optcodes.hpp` now as a no-op. Phase 10 will fill it in.

### Test additions
- Unit test (binder): `(Engine)dog` → compile error.
- Unit test (binder): `(Animal)dog` → success.
- Unit test (binder): `(Dog)animal` → success with downcast note.
- `test.slx`: Add `Animal a2 = (Animal)dog; a2.speak();` — must dispatch to `Dog::speak()` via vtable.

---

## Phase 5 — Access Modifier Enforcement

**Branch:** `phase-5-access-control`
**Criticality:** 🟠 High
**Difficulty:** ⭐⭐ Medium

### Context
`public`, `private`, `protected`, `internal` are parsed and stored but never checked. Any code accesses any member freely.

### Rules to implement in the binder

| Modifier | Accessible from |
|---|---|
| `public` | Everywhere |
| `internal` | Same package only |
| `protected` | Same class + subclasses |
| `private` | Same class only |

### 5.1 — Enforce field access modifiers
- **File:** `language/src/processes/binder.cpp`, `MEMBER_ACCESS` handler
- Check `access_modifier` against `current_class` and `current_package` when resolving `obj.field`.

### 5.2 — Enforce method access modifiers
- Same logic as 5.1 but in the `METHOD_CALL` + `MEMBER_ACCESS` handler.

### 5.3 — `protected` requires inheritance relationship
- If `access_modifier == KEYWORD_PROTECTED`, verify `current_class` is the same as or a subclass of the owning class.

### Test additions
- Unit test: access `private` field from outside class → compile error.
- Unit test: access `protected` field from subclass → success.
- Unit test: access `protected` field from unrelated class → compile error.
- `test.slx`: Move any direct access to `Engine.engineName` (declared `private const`) inside `Engine` methods.

---

## Phase 6 — Error Recovery: Multi-Diagnostic Parser

**Branch:** `phase-6-error-recovery`
**Criticality:** 🟠 High
**Difficulty:** ⭐⭐⭐ Hard

### Context
Every pass throws on the first error. A professional compiler reports all errors it can find in a single pass.

### 6.1 — Binder: collect all errors, report at end
- Replace `throw_error(...)` with `record_error(...)` that pushes to a `std::vector<Diagnostic>`. After the full tree walk, if the vector is non-empty, throw a summary or return a Result type.

### 6.2 — Parser: panic-mode synchronization
- On a parse error, record the diagnostic, then advance tokens until a synchronization point (`;`, `}`, or a top-level keyword like `public`, `class`, `void`). Resume parsing from there.

### 6.3 — Diagnostic formatting improvement
- Add error codes (`E001 — Type mismatch`, `E002 — Unknown identifier`), severity levels (`ERROR`, `WARNING`, `NOTE`), and an optional `--no-color` CLI flag.

### Test additions
- Unit test: file with 3 syntax errors → all 3 reported.
- Unit test: binder with 2 type errors → both appear in output.

---

## Phase 7 — ARC: Cycle Detection / Weak References

**Branch:** `phase-7-arc-cycles`
**Criticality:** 🟠 High
**Difficulty:** ⭐⭐⭐ Hard

### Context
Pure ARC cannot collect reference cycles. If A holds a reference to B and B holds one back to A, both retain counts ≥ 1 forever — permanent memory leak.

### Option A — Weak references (recommended)
- **Add `weak` keyword** to field declarations: `weak public Dog partner;`
- A `weak` reference does **not** call `INC_REF` on assignment and does **not** prevent collection. If the referenced object is freed, the weak slot is zeroed.
- **Assembler:** Skip `INC_REF` for `weak` field writes.
- **Runtime:** When `DEC_REF` drops an object to zero refs, zero all incoming weak slots pointing to it.

### Option B — Mark-and-sweep backup
- Keep ARC as primary collector.
- Add a secondary mark-and-sweep pass triggered when heap usage exceeds a threshold (e.g. 75%).
- Sweep from stack roots, mark all reachable addresses, free unmarked heap objects.

> **Recommendation:** Implement Option A first — it is more elegant, teachable, and simpler to explain in an interview.

### Test additions
- Unit test: create two objects with strong cross-references in a loop. Without weak: heap grows unboundedly. With weak: stays flat.
- `test.slx`: Add a `Node` class with `weak Node next` forming a linked list. Verify it compiles and runs.

---

## Phase 8 — Static Dispatch `::` Fully Wired

**Branch:** `phase-8-scope-resolution`
**Criticality:** 🟡 Medium
**Difficulty:** ⭐⭐ Medium

### Context
`::` is tokenized and the parser sets `is_scope_resolution = true` on `MemberAccessExpression`, but the binder and assembler do not act on it. `b.Animal::speak()` does not actually bypass the vtable.

### 8.1 — Binder: handle `is_scope_resolution`
- When `is_scope_resolution` is true, the left side is a **class name** (not a variable). Look it up in `global_scope` directly, do **not** set `is_virtual_call = true`, and resolve the method only in exactly that class (no inheritance walk).

### 8.2 — Parser: allow `ClassName::staticMethod()` standalone
- Support pure static scope resolution without an object prefix.

### Test additions
- Unit test (binder): `b.Animal::speak()` resolves to `Animal.speak()`, `is_virtual_call = false`.
- `test.slx`: Add `dog.Animal::speak();` — must print `"I am an animal"` (bypasses `Dog` override).

---

## Phase 9 — Abstract Methods & Interfaces

**Branch:** `phase-9-interfaces`
**Criticality:** 🟡 Medium
**Difficulty:** ⭐⭐⭐ Hard

### 9.1 — `abstract` keyword on methods
- **Syntax:** `public abstract void speak();` (no body, ends with `;`)
- **Binder:** A class with any abstract method is implicitly abstract. `new AbstractClass()` → compile error.
- **Binder:** If a subclass does not `override` all abstract methods → compile error.
- **Assembler:** Abstract methods have no bytecode body. Their vtable slot points to a sentinel that throws at runtime.

### 9.2 — `interface` keyword (harder, do after 9.1)
- **Syntax:** `public interface Printable { void print(); }`
- **Syntax:** `public class Dog extends Animal implements Printable { ... }`
- Pure collections of abstract method signatures.
- A class can implement multiple interfaces.
- The binder verifies all interface methods are implemented.
- Each implemented interface adds a separate interface vtable.

### Test additions
- Unit test (binder): `new AbstractAnimal()` → compile error.
- Unit test (binder): `Dog` misses override of abstract `speak()` → compile error.
- `test.slx`: Add `abstract class Shape` with `abstract float64 area()`. Subclasses `Circle` and `Rectangle`. Call through `Shape` reference.

---

## Phase 10 — RTTI & Safe Downcast

**Branch:** `phase-10-rtti`
**Criticality:** 🟡 Medium
**Difficulty:** ⭐⭐⭐ Hard

### Context
The vtable ID at `obj[0]` is the only runtime type info. There is no registry to validate downcasts. `(Dog)cat` corrupts memory silently.

### 10.1 — Type ID registry in the VM
- Add `std::unordered_map<uint32_t, uint32_t> type_parent_map` to `RuntimeContext` (type ID → parent type ID).
- Assembler emits `DEFINE_TYPE <type_id> <parent_type_id>` in the boot sequence for each class.

### 10.2 — `CAST_CHECK` opcode (stub added in Phase 4)
- Implement `CAST_CHECK`: pop an address, read `obj[0]` (type ID), walk `type_parent_map` up the chain, verify the target type appears. If not: throw `"ClassCastException: cannot cast 'Cat' to 'Dog'"`.
- Assembler emits `CAST_CHECK <target_type_id>` for all downcasts.

### 10.3 — `instanceof` operator
- **Syntax:** `if (animal instanceof Dog) { ... }`
- **Binder:** `INSTANCEOF_EXPR` — left is a reference, right is a class name, result is `bool`.
- **Assembler/Runtime:** New `INSTANCE_OF <type_id>` opcode — same type-walk but pushes `true`/`false`.

### Test additions
- Unit test (runtime): `CAST_CHECK` correct type → no throw.
- Unit test (runtime): `CAST_CHECK` wrong type → throw.
- `test.slx`: `if (animal instanceof Dog) Engine.print("IS A DOG");`

---

## Phase 11 — AST Architecture: Visitor Pattern

**Branch:** `phase-11-visitor`
**Criticality:** 🟡 Medium
**Difficulty:** ⭐⭐⭐⭐ Very Hard

### Context
Every pass (Binder, Assembler) has a massive `if/else if` chain on `node_type`. Adding a new node type requires editing every pass. The canonical fix is the **Visitor Pattern**.

### 11.1 — Define `NodeVisitor` interface
```cpp
struct NodeVisitor {
    virtual void visit(BinaryExpression& node) = 0;
    virtual void visit(MethodCallExpression& node) = 0;
    // ... one per node type
    virtual ~NodeVisitor() = default;
};
```

### 11.2 — Add `accept()` to every Node subclass
```cpp
struct BinaryExpression : public Node {
    void accept(NodeVisitor& v) override { v.visit(*this); }
};
```

### 11.3 — Rewrite Binder and Assembler as Visitors
- `class Binder : public NodeVisitor` — each `visit()` method holds what was in the `if/else if` chain.
- `class Assembler : public NodeVisitor` — same.

### 11.4 — Clean up `Node` base class
Remove these fields that do not belong on the base class and move them to their correct subclasses:
- `instance_size` → `ClassDeclaration`
- `mangled_name` → `ClassDeclaration`, `MethodDeclaration`, `FieldDeclaration`
- `vtable_id`, `vtable` → `ClassDeclaration`
- `is_primitive`, `is_reference_type` → `TypeInfo`

> **Warning:** This is the largest refactor in the plan. Break it into sub-commits: (1) add `accept()` to all nodes with no behavior change, (2) convert Binder, (3) convert Assembler, (4) clean up Node base.

### Test additions
- All existing tests must pass unchanged — this is a pure internal refactor.

---

## Phase 12 — README & Full Documentation

**Branch:** `phase-12-docs`
**Criticality:** 🟢 Polish
**Difficulty:** ⭐ Easy

### 12.1 — `README.md` (root of project)
- What Solix is (elevator pitch).
- Build instructions (CMake from scratch).
- Quick start (compile and run `test.slx`).
- Language overview with code examples.
- Architecture diagram of the pipeline (Source → Lexer → Parser → Binder → Assembler → `.slxb` → VM).
- Link to `docs/PLAN.md`.

### 12.2 — `docs/ARCHITECTURE.md`
- Each compiler pass described in detail.
- Bytecode format (opcode table with encoding layout).
- Memory model (heap layout, ARC header format, vtable structure at `obj[0]`).
- Name mangling scheme (`ClassName.methodName(type1,type2)`).

### 12.3 — `docs/LANGUAGE_REFERENCE.md`
- Full syntax reference.
- All keywords, operators, types, access modifiers.
- Class declarations, inheritance, constructor initializer lists, operator overloading syntax.
- Code examples for every feature.

### 12.4 — Inline code comments
- Every public function in `.hpp` files: one-line doc comment.
- Every major handler in `binder.cpp` and `assembler.cpp`: comment explaining what it does and why.

---

## Phase 13 — Test Coverage Expansion

**Branch:** `phase-13-tests`
**Criticality:** 🟢 Polish
**Difficulty:** ⭐⭐ Medium

### Context
Current coverage: ~383 lines, 5 files, all happy-path. Zero tests for error conditions. Zero unit tests for features added in Phases 1–4B.

### 13.1 — Lexer error tests
- Unknown character → diagnostic.
- Unterminated string literal → diagnostic.
- Very long identifier → no crash.

### 13.2 — Parser error tests
- Missing `;` after statement.
- Unmatched `{` / `}`.
- Invalid modifier combination (`static native virtual`).

### 13.3 — Binder error tests
- Use of undeclared variable.
- Type mismatch on assignment.
- Calling a non-existent method.
- Accessing a `private` field from outside the class (requires Phase 5).
- `new AbstractClass()` (requires Phase 9).

### 13.4 — Feature unit tests
- Operator overloading: verify mangled name resolution.
- Inheritance: verify field offsets across one and two levels.
- Polymorphism: verify `is_virtual_call = true` for virtual methods, `false` for non-virtual.
- Scope resolution: verify `is_virtual_call = false` when `::` is used (requires Phase 8).

### 13.5 — Runtime integration tests
- ARC: allocate objects in a loop, verify `currently_used_words` stays bounded (requires Phase 3).
- Stack overflow via deep recursion → clean error, not segfault (requires Phase 2).
- Injected bad bytecode → clean `runtime_error`, not segfault (requires Phase 2).

---

## Summary Checklist

```
[ ] Phase 1  — Hotfix: Hygiene & UB           (branch: phase-1-hotfix)
[ ] Phase 2  — VM Safety: Bounds Checking      (branch: phase-2-vm-safety)
[ ] Phase 3  — ARC: Wire Emit in Assembler     (branch: phase-3-arc-emit)
[ ] Phase 4  — Cast Safety                     (branch: phase-4-cast-safety)
[ ] Phase 5  — Access Modifier Enforcement     (branch: phase-5-access-control)
[ ] Phase 6  — Error Recovery                  (branch: phase-6-error-recovery)
[ ] Phase 7  — ARC: Cycle Detection            (branch: phase-7-arc-cycles)
[ ] Phase 8  — Static Dispatch ::              (branch: phase-8-scope-resolution)
[ ] Phase 9  — Abstract Methods & Interfaces   (branch: phase-9-interfaces)
[ ] Phase 10 — RTTI & Safe Downcast            (branch: phase-10-rtti)
[ ] Phase 11 — AST Visitor Pattern             (branch: phase-11-visitor)
[ ] Phase 12 — README & Documentation          (branch: phase-12-docs)
[ ] Phase 13 — Test Coverage Expansion         (branch: phase-13-tests)
```

---

*Generated: 2026-09-17. Synthesized from two independent reviews: 72/100 and 92/100.*

---

## Phase 17 — Templates: AST Upgrades & Deep Cloning

**Branch:** `phase-17-templates-ast`
**Criticality:** 🟡 Medium
**Difficulty:** ⭐⭐ Medium

### Context
To support Generics (Templates) like `List<T>`, Solix will use **Monomorphization** (C++/Rust style). This means the VM and Assembler remain completely unchanged; the `Binder` will copy the AST and replace the generic types with concrete types before compilation.

### 17.1 — `TypeInfo` Support for Nested Templates
- Update `TypeInfo` in `ast.hpp` to include `std::vector<TypeInfo> type_args;`.
- This ensures we can represent deep nested types like `Pair<char[], int32>` or `List<Map<String, float64>>`.

### 17.2 — Declaration Template Parameters
- Add `std::vector<std::string> template_parameters;` to `ClassDeclaration`, `MethodDeclaration`, and `AliasDeclaration`.

### 17.3 — AST Deep Cloning Mechanism
- Implement a virtual `Node* clone()` method on the `Node` base class and override it in every AST node subclass.
- This allows the Binder to create fresh, un-bound copies of a generic class or method AST so it can securely replace `T` with concrete types.

---

## Phase 18 — Templates: Lexer & Parser Updates

**Branch:** `phase-18-templates-parser`
**Criticality:** 🟡 Medium
**Difficulty:** ⭐⭐⭐ Hard

### Context
Parsing `<` and `>` as template brackets without confusing them with Less-Than and Greater-Than operators is a classic compiler challenge.

### 18.1 — Declaration Syntax Parsing
- Update `parse_class_declaration`, `parse_method_declaration`, and `parse_alias_declaration` to look for `<` after the identifier.
- Parse a comma-separated list of identifiers: `class Dictionary<K, V>`.

### 18.2 — Type Syntax Parsing
- Update `parse_type()` to check if the type name is followed by `<`.
- Recursively call `parse_type()` to build the `type_args` list inside `TypeInfo`.
- **The Bracket Problem:** Carefully handle `>>` tokens so that `List<List<int32>>` parses correctly and doesn't crash if the Lexer combines `>>` into a Right-Shift operator.

---

## Phase 19 — Templates: Binder Monomorphization

**Branch:** `phase-19-templates-binder`
**Criticality:** 🔴 High
**Difficulty:** ⭐⭐⭐⭐⭐ Very Hard

### Context
This is the core engine for Generics. The Binder will act as an on-demand factory for classes.

### 19.1 — Deferred Binding
- When the Binder visits a `ClassDeclaration` or `MethodDeclaration` that has `template_parameters`, it **skips** binding it. 
- It stores the raw, un-bound AST into a new `template_registry`.

### 19.2 — On-Demand Instantiation
- When the Binder resolves a type (e.g., encountering `Pair<char[], int32>`), it checks if that specific concrete class already exists.
- If it does not exist, the Binder fetches the raw AST for `Pair<K, V>` from the `template_registry`.
- It calls `clone()` to get a fresh AST.
- It walks the cloned AST, replacing every `TypeInfo` where name == `K` with `char[]`, and name == `V` with `int32`.
- It mangles the new class name to something like `Pair<char[],int32>`.
- Finally, it recursively visits and binds this newly generated class, injecting it seamlessly into the compilation pipeline.

### Test Additions
- Unit test: Nested generics like `List<Pair<int32, char[]>>`.
- Integration test in `test.slx`: Create a generic `Box<T>` class, instantiate `Box<int32>` and `Box<float64>`, and verify the methods execute properly.

---

## Updated Summary Checklist

```
[x] Phase 14 — Refactor CLI Options and Native Registration
[x] Phase 15 — Full Symbol Mangling for Native Functions
[x] Phase 16 — Automated Native Call Interface
[ ] Phase 17 — Templates: AST Upgrades & Deep Cloning
[ ] Phase 18 — Templates: Lexer & Parser Updates
[ ] Phase 19 — Templates: Binder Monomorphization
```

---

## Phase 20 — Templates: Alias & Function Monomorphization

**Branch:** `phase-20-templates-alias`
**Criticality:** 🟡 Medium
**Difficulty:** ⭐⭐⭐ Hard

### Context
Phase 18 and 19 implemented Class Templates. Phase 20 completes the Generics system by adding support for Alias Templates (e.g. `alias List<T> = Vector<T>;` vs `alias IntegerList = List<uint32>;`) and Standalone Function Templates.

### 20.1 — Alias Template Monomorphization
- Update `Binder::instantiate_template()` to support `NodeType::ALIAS_STMT`.
- When instantiating an Alias, clone the alias, replace its types via `TemplateSubstitutionVisitor`, register it in `global_scope`, and execute `BIND_EXECUTION`.
- Ensure distinction between concrete aliases (`alias A = B<int>;`) and template aliases (`alias A<T> = B<T>;`).

### 20.2 — Function Template Monomorphization
- Update `Binder::visit(MethodCallExpression)` to trigger `instantiate_template()` if `type_args` are provided for a generic function/method.
- When calling a generic function (`foo<int32>()`), ensure the Binder dynamically instantiates the function template, registers the monomorphized signature, and validates the call.

### 20.3 — Testing
- Add Catch2 tests for `alias IntegerList = List<uint32>` vs `alias List<T> = Vector<T>`.
- Add Catch2 tests for generic method calls `foo<int32>(5)`.


---

## Phase 21 — Templates: Implicit Deduction & Explicit Specialization

**Branch:** `phase-21-templates-deduction`
**Criticality:** 🟢 Polish
**Difficulty:** ⭐⭐⭐⭐ Hard

### Context
To make the Generics system ergonomic and complete, Solix needs the ability to implicitly deduce template arguments from function calls (like modern C++) and allow explicit template specializations using angle brackets in method declarations.

### 21.1 — Explicit Template Specialization
- Update `parse_field_or_method()` in the Parser to support parsing concrete types inside `< >` after the method identifier (e.g., `print<char[]>(char[] target)`).
- Differentiate between a generic blueprint (e.g., `<T>`) and a specialization (e.g., `<char[]>`).
- Bake the concrete types into the method's `mangled_name` at compile time so the Binder handles it as a standard function overload, effortlessly shadowing the auto-generated generic pipeline.

### 21.2 — Implicit Template Parameter Deduction
- Introduce a sophisticated `deduce_template_arguments` recursive algorithm inside the Binder.
- Update `Binder::visit(MethodCallExpression)` to intercept unresolved method calls.
- Zip the provided call-site argument types against the target generic blueprint parameters.
- Resolve nested generic depths (e.g., extracting `int32` when matching `int32[]` against `T[]`, or matching `Box<int32>` against `Box<T>`).
- Seamlessly trigger `instantiate_template` in the background with the deduced types.

### 21.3 — Testing
- Add Catch2 tests for Implicit Deduction (e.g., `swap_boxes(i_box1, i_box2)` without `<int32>`).
- Add Catch2 tests for Explicit Specialization (e.g., `print<char[]>(...)`).


---

## Phase 22 — Binder Correctness: Node Stamping, Cast Precedence & Null Safety

**Branch:** `phase-22-binder-fixes`
**Criticality:** 🔴 High
**Difficulty:** ⭐⭐ Medium

### Context
Three independent correctness bugs were found and fixed that blocked multi-file compilation and correct expression semantics.

### 22.1 — AST Node Stamping (Context-Loss Fix)
- Added `std::string package_context = "";` to the base `Node` struct in `statements.hpp`.
- During Pass 1 (`REGISTER_GLOBALS` + `REGISTER_MEMBERS`), each visited top-level or member node (Class, Alias, Enum, Field, Method, Constructor) is "stamped" with `n.package_context = current_package;` to record the package it belongs to.
- Updated `Binder::resolve_type()` to prefer the stamped `error_node->package_context` over the mutable `current_package` global when resolving short type names.
- **Effect:** Pass 2 (`bind_types_and_memory`) can now safely iterate over the flat `global_scope.symbols` dictionary — because each node carries its own package context — without losing namespace information.

### 22.2 — Parser Cast Precedence Fix
- Inside `ParserState::parse_primary()`, changed the cast target from `parse_expression()` to `parse_unary()`.
- **Effect:** `(char)x + 5` now correctly parses as `((char)x) + 5`, not `(char)(x + 5)`.

### 22.3 — Null Comparison Safety
- Inside `Binder::visit(BinaryExpression)`, the `primitive_fallback` block now skips the type-mismatch error when the operator is `==` or `!=` and at least one operand is `"void"` (the type of `null`).
- **Effect:** Patterns like `if (other == null)` and `if (assigned_string == null)` compile without errors.

### 22.4 — Multi-File Compilation Support
- The above fixes combined now allow `string.slx` (package `solix.core`) to be compiled together with `test.slx` (package `com.solix.advanced.test`), with cross-package type resolution working correctly across all three passes.

### Testing
- Verified `solix_launcher compile string.slx test.slx` succeeds end-to-end.
- Added `tests/src/test_phase22.cpp` with Catch2 unit tests.

---

## Architectural VM Refactor (Phases 23–31)

---

## Phase 23 — Lexer & Parsing Fixes: Ampersand Lexing

**Branch:** `phase-23-lexer-ref-fix`  
**Criticality:** 🔴 Critical  
**Difficulty:** ⭐ Easy  

### Context
Reference annotations (`Type& name`) were incorrectly parsed because `&` was lexed as `OPERATOR_LOGICAL_AND` (`&&`) or unrecognized syntax, causing grammar ambiguities in variable and member declarations.

### 23.1 — Ampersand Lexing & Tokenization
- **File:** `language/include/solix/utilities/token.hpp`
- Add `PUNCTUATION_AMPERSAND` to `TokenType` enum.
- **File:** `language/src/processes/lexer.cpp`
- In `Lexer::tokenize`, parse a single `&` as `PUNCTUATION_AMPERSAND` (distinguishing it from `OPERATOR_LOGICAL_AND` for `&&`).

### 23.2 — Parser Reference Type Disambiguation
- **File:** `language/src/processes/parser.cpp`
- Update `ParserState::parse_variable_declaration` and `parse_field_or_method` to check for `PUNCTUATION_AMPERSAND` instead of `OPERATOR_LOGICAL_AND` when identifying reference types (e.g., `Type& name`).

---

## Phase 24 — ALU Type Corruption: Typed Opcodes (_I64 & _F64)

**Branch:** `phase-24-alu-typed-opcodes`  
**Criticality:** 🔴 Critical  
**Difficulty:** ⭐⭐ Medium  

### Context
Generic arithmetic opcodes (`ADD`, `SUBTRACT`, etc.) operated on raw 64-bit stack slots without distinguishing integer from IEEE 754 floating-point representations, causing severe float/integer corruption.

### 24.1 — Typed Opcodes Definition
- **File:** `language/include/solix/utilities/optcodes.hpp`
- Replace generic math/comparison opcodes (`ADD`, `SUBTRACT`, `MULTIPLY`, `DIVIDE`, `MODULO`, `LESS_THAN`, `GREATER_THAN`, `LESS_EQUAL`, `GREATER_EQUAL`, `EQUAL`, `NOT_EQUAL`, `INC`, `DEC`) with explicit `_I64` and `_F64` variants (`ADD_I64`, `ADD_F64`, `SUB_I64`, `SUB_F64`, `MUL_I64`, `MUL_F64`, `DIV_I64`, `DIV_F64`, `MOD_I64`, `LESS_I64`, `LESS_F64`, `GREATER_I64`, `GREATER_F64`, `LESS_EQ_I64`, `LESS_EQ_F64`, `GREATER_EQ_I64`, `GREATER_EQ_F64`, `EQ_I64`, `EQ_F64`, `NEQ_I64`, `NEQ_F64`).
- Add `OpCode::DUP2` and `OpCode::ARRAY_LENGTH`.

### 24.2 — ALU Execution Handlers
- **File:** `language/src/processes/runtime.cpp`
- Implement `_I64` handlers using `static_cast<int64_t>` for 64-bit integer arithmetic.
- Implement `_F64` handlers using `std::bit_cast<double>` to reinterpret stack slots as double precision floats, execute double math, and `std::bit_cast<uint64_t>` back to stack slots.

### 24.3 — Assembler Code Generation Routing
- **File:** `language/src/processes/assembler.cpp`
- Update `Assembler::visit(BinaryExpression)` and `visit(UnaryExpression)` to check `expression_type`: emit `_F64` opcodes for `float32`/`float64`, and `_I64` opcodes for all integral primitive types (`int8`, `int16`, `int32`, `int64`, `uint8`, `uint16`, `uint32`, `uint64`, `bool`, `char`).

---

## Phase 25 — Runtime Memory Integrity & Array Safety

**Branch:** `phase-25-memory-array-safety`  
**Criticality:** 🔴 Critical  
**Difficulty:** ⭐⭐ Medium  

### Context
Heap allocations contained uninitialized dirty memory, arrays suffered from `+1` double-offset calculation errors, and array length operations lacked native opcode support. Null pointers were not safely checked before member access.

### 25.1 — Zero-Initialization & Baseline ARC
- **File:** `language/src/processes/runtime.cpp`
- In `Memory::dynamic_allocation`, inject a zero-initialization loop for the memory payload (`heap[header_addr + 1 + i] = 0`).
- Initialize header metadata with a baseline Automatic Reference Count (ARC) of 1.

### 25.2 — Array Offsets & Bounds Checking
- **File:** `language/src/processes/runtime.cpp`
- In `op_GET_ARRAY` and `op_SET_ARRAY`, remove the redundant `+ 1` double-offset (`heap_data[array_addr + index]`).
- Extract length from allocation header (`heap_data[addr - 1] >> 32`) and throw `"Out of Bounds"` exception if `index >= length`.

### 25.3 — Array Length OpCode
- **Files:** `language/src/processes/runtime.cpp`, `language/src/processes/assembler.cpp`
- Implement `op_ARRAY_LENGTH` in `runtime.cpp` to pop array address, extract header length (`heap_data[addr - 1] >> 32`), and push length onto the stack.
- Update `Assembler::visit(MemberAccessExpression)` to emit `OpCode::ARRAY_LENGTH` for `.length` accesses on array types.

### 25.4 — Null Pointer Guards
- **File:** `language/src/processes/runtime.cpp`
- Add `if (obj == 0) throw std::runtime_error("NullPointer");` guard to `GET_PROPERTY`, `SET_PROPERTY`, `WEAK_SET_PROPERTY`, `CALL_VIRTUAL`, `GET_ARRAY`, `SET_ARRAY`, and `ARRAY_LENGTH` instruction handlers.

---

## Phase 26 — ARC Ownership & Emission

**Branch:** `phase-26-arc-ownership-emission`  
**Criticality:** 🔴 Critical  
**Difficulty:** ⭐⭐⭐ Hard  

### Context
Reference count incrementing was improperly attached to identifier evaluations rather than ownership transfers, resulting in incorrect ref counts, premature frees, or memory leaks.

### 26.1 — Read vs Store Shift
- **File:** `language/src/processes/assembler.cpp`
- Remove `OpCode::INC_REF` from `visit(IdentifierNode)` (reading an identifier must not increment reference count).
- Inject `OpCode::INC_REF` immediately *after* expression evaluation but *before* `SET_LOCAL` or `SET_PROPERTY` in assignment expressions and variable declarations.

### 26.2 — Implicit `this` Reference Ownership
- **File:** `language/src/processes/assembler.cpp`
- In `visit(MethodCallExpression)`, when pushing `this` (slot 0) implicitly for instance method calls, emit `OpCode::INC_REF`.

### 26.3 — Expression Statement Reference Cleanups
- **File:** `language/src/processes/assembler.cpp`
- In `visit(ExpressionStatement)`, if `expression_type` is a reference type (array depth > 0 or non-primitive class), emit `DEC_REF` instead of `POP` to clean up temporary unassigned evaluation results.

---

## Phase 27 — Function Cleanup & Flow Control Leaks

**Branch:** `phase-27-arc-cleanup-flow-leaks`  
**Criticality:** 🔴 Critical  
**Difficulty:** ⭐⭐⭐ Hard  

### Context
Function parameter reference counts leaked upon returning, and `break`/`continue` statements jumped across block boundaries without releasing scope-bound local references.

### 27.1 — Parameter ARC Cleanup
- **File:** `language/src/processes/assembler.cpp`
- Implement helper function `emit_cleanup_for_function(MethodDeclaration*)`.
- Loop through function `parameters` (plus implicit `this` at slot 0 for non-static methods), emitting `GET_LOCAL` + `DEC_REF` for each reference type.
- Invoke this cleanup helper before `RETURN` in `compile_function` and `visit(ReturnStatement)`.

### 27.2 — Break & Continue Scope Cleanup
- **File:** `language/src/processes/assembler.cpp`
- In `visit(BreakStatement)` and `visit(ContinueStatement)`, implement a loop walking up `node.parent`.
- For every `NodeType::BLOCK` encountered before reaching the enclosing loop node (`WhileStatement`, `ForStatement`, `DoWhileStatement`), call `emit_cleanup_for_node(current)` to release all local reference variables prior to control flow jump.

---

## Phase 28 — Object Initialization & Array Mutation

**Branch:** `phase-28-object-init-array-mutation`  
**Criticality:** 🟠 High  
**Difficulty:** ⭐⭐⭐ Hard  

### Context
Classes without constructors failed to initialize default fields, constructor field initializers were skipped, and array element mutations (`arr[idx]++`) evaluated array and index expressions twice.

### 28.1 — Programmatic Implicit Constructors
- **File:** `language/src/processes/binder.cpp`
- In `Binder::visit(ClassDeclaration)`, if a class lacks a `ConstructorDeclaration` child node during Pass 1, programmatically inject an empty default constructor AST node.

### 28.2 — Constructor Field Initializer Emission
- **File:** `language/src/processes/assembler.cpp`
- In `visit(ConstructorDeclaration)`, after compiling `super()` / base class setup, iterate through `FieldDeclaration` children.
- For non-static fields with an `initializer` expression, compile the expression, emit `GET_LOCAL 0` (`this`), and emit `SET_PROPERTY` to initialize fields before executing constructor body statements.

### 28.3 — Array Double-Evaluation Fix & `DUP2`
- **Files:** `language/src/processes/runtime.cpp`, `language/src/processes/assembler.cpp`
- Implement `op_DUP2` in `runtime.cpp` (duplicates top two 64-bit stack elements: `[arr, idx] -> [arr, idx, arr, idx]`).
- In `Assembler::visit(UnaryExpression)` for array element compound mutation or increment/decrement (`arr[idx]++`), evaluate array and index expressions exactly once, emit `DUP2`, execute `GET_ARRAY`, perform arithmetic operation, and execute `SET_ARRAY`.

---

## Phase 29 — The Static String Pool

**Branch:** `phase-29-static-string-pool`  
**Criticality:** 🟠 High  
**Difficulty:** ⭐⭐⭐ Hard  

### Context
String literals were dynamically allocated repeatedly on every access, causing excessive heap allocations and performance degradation.

### 29.1 — Binder String Pool Tracking
- **Files:** `language/include/solix/compilation.hpp`, `language/src/processes/binder.cpp`
- Add `std::unordered_map<std::string, int> string_pool;` to `CompilationContext`.
- In `visit(LiteralNode)` for string literals, check if the string exists in `string_pool`. If not, allocate a `static_variable_index`, record the string, and store the index in `node.memory_index`.

### 29.2 — Global Boot Sequence String Initialization
- **File:** `language/src/processes/assembler.cpp`
- In `compile_boot_sequence()`, include `context.string_pool.size()` in `total_globals`.
- Loop through `string_pool` emitting `PUSH_CONST_STRING` and `SET_GLOBAL <index>` to allocate all string constants once into global slots during VM initialization.

### 29.3 — Hot Path String Access
- **File:** `language/src/processes/assembler.cpp`
- In `visit(LiteralNode)` for string literals, emit `GET_GLOBAL <memory_index>` to load the pre-allocated string reference directly.

---

## Phase 30 — Compiler Edge-Cases (Nested Templates & Polymorphic Arrays)

**Branch:** `phase-30-nested-templates-poly-arrays`  
**Criticality:** 🟡 Medium  
**Difficulty:** ⭐⭐⭐ Hard  

### Context
Template parameter deduction broke on nested generic types containing commas (e.g. `Map<K, V>`), and array literals strictly checked type equality rather than assignability.

### 30.1 — Depth-Aware Template Parameter Deduction
- **File:** `language/src/processes/template_deduction.cpp` / `binder.cpp`
- In `deduce_template_arguments`, replace naive `generic_content.find(",")` with a loop tracking `<` and `>` depth, splitting on commas only when `depth == 0`.

### 30.2 — Polymorphic Array Literals
- **File:** `language/src/processes/binder.cpp`
- In `visit(ArrayLiteralExpression)`, replace strict type equality checks with `is_assignable(target_type, element_type)` so derived class elements can be assigned into base class array literals.

---

## Phase 31 — High-Performance Engine Tuning

**Branch:** `phase-31-vm-engine-tuning`  
**Criticality:** 🟢 Polish  
**Difficulty:** ⭐⭐⭐ Hard  

### Context
Per-instruction program counter bounds checks, member-function stack operations (`push()`/`pop()`), and dynamic vector allocations for call frames added substantial overhead to the VM dispatch loop.

### 31.1 — Uninterrupted OpCode Dispatch
- **File:** `language/src/processes/runtime.cpp`
- Remove per-instruction `if (program_counter >= bytecode.size())` bounds check from inner `DISPATCH()` loop/macro, allowing `op_HALT` to terminate execution cleanly.

### 31.2 — Inlined Pointer-Based Evaluation Stack
- **File:** `language/src/processes/runtime.cpp`
- Eliminate `push()` and `pop()` class methods. Define `uint64_t* sp = memory.stack.data();` inside `execute()`, manipulating stack slots via direct pointer arithmetic (`*sp++` and `*--sp`).

### 31.3 — Fixed Zero-Allocation Call Stack
- **File:** `language/src/processes/runtime.cpp`
- Replace `std::vector<Frame> call_stack` with a fixed-size `std::array<Frame, 65536>` and a `size_t call_depth = 0;` counter to eliminate heap allocations during call frame pushes and pops.

---

# Solix Code Audit: binder.cpp, assembler.cpp, and runtime.cpp

An in-depth, line-by-line audit of `binder.cpp`, `assembler.cpp`, and `runtime.cpp` was conducted to identify runtime crashes, memory safety defects (ARC use-after-free, leaks, dangling references), bytecode emission errors, and type-system gaps.

---

## Executive Summary & Priority Matrix

| Component | Critical | High | Medium | Total |
|---|:---:|:---:|:---:|:---:|
| `runtime.cpp` | 1 | 2 | 3 | 6 |
| `assembler.cpp` | 4 | 5 | 5 | 14 |
| `binder.cpp` | 4 | 8 | 6 | 18 |
| **Total Findings** | **9** | **15** | **14** | **38** |

---

## 1. Runtime VM Defects: runtime.cpp

### Critical & High Severity

#### 1. Dynamic Heap Memory Corruption via Unchecked free_blocks Reuse
- **Location:** `runtime.cpp:32-35`
- **Root Cause:** `dynamic_allocation(size_t size_in_words, Address address)` reuses any address from `free_blocks.back()` without checking if the freed block's size is ≥ requested `size_in_words`. If a small 1-word block is reused for a 64-word object or array, the allocation writes past the block and corrupts the next 63 heap words.
- **Fix:** Maintain a sized free-list (e.g. `std::multimap<size_t, Address>`) or only reuse blocks when `block_size >= size_in_words`.

#### 2. Integer Negation Corrupted to Float Negation in op_NEGATE
- **Location:** `runtime.cpp:622-627`
- **Root Cause:** `op_NEGATE` unconditionally casts the 64-bit operand to double via `bit_cast_from_u64<double>`, negates `-a`, and bit-casts back. For integer values, toggling the IEEE-754 sign bit corrupts the two's-complement integer value (e.g. `-10` becomes `9223372036854775818`).
- **Fix:** Emit `SUB_I64` from 0 for integers, or add a dedicated `NEGATE_I64` opcode.

#### 3. Missing Conversions in op_CONV_* (No-Op Dispatches)
- **Location:** `runtime.cpp:817-830`
- **Root Cause:** `op_CONV_I8` through `op_CONV_F64` immediately call `DISPATCH();` without transforming the top stack item. Value truncations (e.g. `(int8)257`) and int-to-float or float-to-int conversions perform no actual data transformation.
- **Fix:** Implement proper sign extension / truncation masking for integer types and `bit_cast_to_u64(static_cast<double>(int_val))` for numeric casts.

### Medium Severity

#### 4. Unhandled Top-Level Exception Leak
- **Location:** `runtime.cpp:1065`, `runtime.cpp:1085-1087`
- **Root Cause:** `op_THROW_EXCEPTION` increments ref count (`memory.increase_reference(exc)`). If the exception bubbles to the top level without being caught, `op_CLEAR_EXCEPTION` is never invoked, leaking the exception object.
- **Fix:** In the unhandled exception termination path, call `memory.decrease_reference(active_exception)`.

#### 5. Leftover Debug Prints in Release Code
- **Location:** `runtime.cpp:57-59`
- **Root Cause:** `if (address == 50) std::cout << "[DEBUG] Deallocating address 50!" << std::endl;` was left in the deallocator.
- **Fix:** Remove the debug print statement.

---

## 2. Assembler & Bytecode Emission: assembler.cpp

### Critical Severity

#### 1. Constructor Visitor Bypassed; All Instance Field Initializers Dropped
- **Location:** `assembler.cpp:464-468`
- **Root Cause:** `compile_class` directly invokes `compile_function(child.get())` on constructors instead of `compile_node(child.get())`. Because field initializers are emitted inside `visit(ConstructorDeclaration&)`, that method is dead code and never runs.
- **Impact:** All non-static class fields with default values (`int x = 42;`) remain 0/null.
- **Fix:** Dispatch constructors to `compile_node(child.get())`.

#### 2. Container Object Prematurely Destroyed on Weak Property Assignment
- **Location:** `assembler.cpp:1142-1146`, `assembler.cpp:1184-1187`
- **Root Cause:** Emits `DUP` followed by `DEC_REF` on `obj` before `WEAK_SET_PROPERTY`. `WEAK_SET_PROPERTY` already handles reference count adjustments.
- **Impact:** Assigning a weak property (`this.parent = p;`) decrements `this`, destroying the enclosing object while its method is still executing.
- **Fix:** Remove the superfluous `DUP` and `DEC_REF` instructions.

#### 3. Dangling Pointer Returned on return local_var;
- **Location:** `assembler.cpp:848-872`
- **Root Cause:** In `visit(ReturnStatement)`, the return value is evaluated (stack receives address with ref count = 1), and then `emit_cleanup_for_node` issues `DEC_REF` for all local variables before `RETURN`.
- **Impact:** The returned local object's reference count drops to 0 and is freed before the caller can receive it.
- **Fix:** Check if return expression is a reference type, and emit `INC_REF` on the return value prior to scope cleanup.

#### 4. Missing INC_REF on Unqualified Identifier Field Reads
- **Location:** `assembler.cpp:1048-1058`
- **Root Cause:** `visit(MemberAccessExpression)` emits `INC_REF` when reading reference fields, but `visit(IdentifierNode)` for fields does not.
- **Impact:** Reading field without `this.` leaves reference count unincremented; passing it into a method results in premature deallocation.
- **Fix:** Add `if (field->is_reference_type) emit_byte(OpCode::INC_REF);` to `visit(IdentifierNode)`.

### High Severity

#### 5. Postfix ++/-- Ignored in UnaryExpression
- **Location:** `assembler.cpp:1300-1370`
- **Root Cause:** Never inspects `uny->is_prefix`. `x = i++` returns the updated value instead of the old value.
- **Fix:** For postfix expressions, duplicate the old value before storing the updated value.

#### 6. Missing INC_REF for Elements in Array Literals
- **Location:** `assembler.cpp:1619-1626`
- **Root Cause:** `SET_ARRAY` is called on literal elements without incrementing the ref count of reference elements.
- **Fix:** Emit `INC_REF` for each reference element before `SET_ARRAY`.

#### 7. Unpatched JUMP 0xFFFFFFFF in Outer Try Statements
- **Location:** `assembler.cpp:1770-1775`
- **Root Cause:** When `exception_cleanup_patches` is empty (top-level try), an unconditional jump to `0xFFFFFFFF` is emitted without patching.
- **Fix:** Emit `OpCode::JMP_TO_OUTER_CLEANUP` when `exception_cleanup_patches.empty()`.

#### 8. Catch Parameter Variable Leaked on Clause Exit
- **Location:** `assembler.cpp:1749-1758`
- **Root Cause:** The catch variable is assigned to local slot `catch_clause->variable_memory_index`, but is omitted from `catch_clause->body` declarations. `emit_cleanup_for_node` never decrements it.
- **Fix:** Emit explicit `GET_LOCAL` + `DEC_REF` at the end of each catch block.

---

## 3. Semantic Analysis & Type System: binder.cpp

### Critical Severity

#### 1. Method Template Instantiations Return nullptr and Trigger False Duplicates
- **Location:** `binder.cpp:26-38`, `binder.cpp:158-175`, `binder.cpp:2005-2010`
- **Root Cause:** `instantiate_template` indexes methods by `name<type_args>`, while `visit(MethodDeclaration)` registers them with full signatures (`name<type_args>(param_types)`). Resolution fails, returns nullptr, and subsequent attempts crash with "Duplicate method signature".
- **Fix:** Harmonize the mangling format in `instantiate_template` to include parameter signatures.

#### 2. VTables Calculated Before Base Class Resolution
- **Location:** `binder.cpp:473-531`, `binder.cpp:546-582`
- **Root Cause:** In `bind_types_and_memory()`, `calculate_vtable` is invoked before base class references are resolved across packages. Base methods are excluded from derived vtables.
- **Fix:** Reorder `resolve_base_class` before `calculate_vtable`.

#### 3. Corrupted VTable Override Matching on Qualified Parameter Types
- **Location:** `binder.cpp:495-499`
- **Root Cause:** `mangled_name.rfind('.')` is used to strip class prefixes. If a parameter has a package prefix (e.g. `foo(net.Socket)`), `rfind('.')` truncates at `net.`, destroying signature matching.
- **Fix:** Split only at the first open parenthesis `(` before searching for class dots.

#### 4. Field Initializers Skipped in Pass 3 Semantic Analysis
- **Location:** `binder.cpp:713-723`, `binder.cpp:1960-1968`
- **Root Cause:** Pass 3 does not recurse into `FieldDeclaration::initializer`. Type errors, invalid casts, and unresolved identifiers in field initializers pass without compilation errors.
- **Fix:** Add traversal and type-checking for field initializers in Pass 3.

### High Severity

#### 5. Subtype Polymorphism Missing in Method Overload Resolution
- **Location:** `binder.cpp:1316-1317`, `binder.cpp:1404-1405`
- **Root Cause:** Method calls check only exact string matches for parameter types; passing derived instance `Dog` where `Animal` is expected triggers "No matching method".
- **Fix:** Query `is_assignable_from(param_type, arg_type)` using class inheritance hierarchies.

#### 6. Constructor Overload Resolution Mangles Parameter Types Incorrectly
- **Location:** `binder.cpp:1473-1490`
- **Root Cause:** Constructor signature lookups omit array dimensions (`[]`) and inner type qualifications.
- **Fix:** Use `type.to_string()` for all parameter tokens during lookup.

---

## Recommended Action Plan

We can remediate these findings in targeted stages:

1. **Stage 1 (Runtime & Memory Safety):**
   - Fix `dynamic_allocation` free-list sizing check.
   - Fix integer negation in `runtime.cpp:622-627` and `assembler.cpp:1324-1325`.
   - Implement primitive casts in `op_CONV_*`.
2. **Stage 2 (Assembler CodeGen & ARC):**
   - Fix constructor visitor invocation in `compile_class` so field initializers are compiled.
   - Fix weak property assignment decrements and return local variable `INC_REF`.
   - Add postfix `is_prefix` support.
3. **Stage 3 (Binder Type System & VTables):**
   - Reorder base class resolution before vtable calculation.
   - Fix qualified type vtable override matching.
   - Fix method template instantiation signature keys.

---

## Phase 32 — Runtime VM & Memory Safety

**Branch:** `phase-32-runtime-vm-memory`  
**Criticality:** 🔴 Critical  
**Difficulty:** ⭐⭐ Medium  

### 32.1 — Dynamic Allocation Free-List Sizing Check
- **File:** `language/src/runtime.cpp`
- In `Memory::dynamic_allocation(size_in_words)`, verify that a recycled block from `free_blocks` satisfies `block_size >= size_in_words`. If no fitting block exists, allocate from `next_free_dynamic`.

### 32.2 — Integer and Float Negation Fix
- **Files:** `language/src/runtime.cpp`, `language/src/processes/assembler.cpp`
- In `assembler.cpp:visit(UnaryExpression)`, differentiate integer vs floating-point negation. For integers, emit `0` then operand then `SUB_I64` (or emit `NEGATE` only for floating-point and integer subtract for integer).
- In `runtime.cpp:op_NEGATE`, ensure floating-point negate operates on `double` values correctly, or support integer negation.

### 32.3 — Implement Primitive Type Conversions in `op_CONV_*`
- **File:** `language/src/runtime.cpp`
- In `op_CONV_I8`, `op_CONV_I16`, `op_CONV_I32`, `op_CONV_I64`, `op_CONV_U8`, `op_CONV_U16`, `op_CONV_U32`, `op_CONV_U64`, `op_CONV_F32`, `op_CONV_F64`, implement proper bit masking, sign extension, and float/int conversions instead of no-op dispatches.

### 32.4 — Cleanup Unhandled Exception Reference Leak & Remove Debug Prints
- **File:** `language/src/runtime.cpp`
- In top-level unhandled exception handler, decrement ref count for `active_exception` to prevent leak.
- Remove leftover debug print at `address == 50`.

---

## Phase 33 — Assembler CodeGen & ARC Safety

**Branch:** `phase-33-assembler-codegen-arc`  
**Criticality:** 🔴 Critical  
**Difficulty:** ⭐⭐⭐ Hard  

### 33.1 — Constructor Visitor Dispatch for Field Initializers
- **File:** `language/src/processes/assembler.cpp`
- In `compile_class`, call `compile_node(child.get())` on `CONSTRUCTOR_DECL` so that `visit(ConstructorDeclaration&)` runs and emits non-static field initializers.

### 33.2 — Fix Weak Property Assignment Over-Decrement
- **File:** `language/src/processes/assembler.cpp`
- Remove the spurious `DUP` and `DEC_REF` instructions in `visit(AssignmentExpression)` before `WEAK_SET_PROPERTY`.

### 33.3 — Prevent Premature Deallocation on Returning Local References
- **File:** `language/src/processes/assembler.cpp`
- In `visit(ReturnStatement)`, check if the return expression is a reference type and emit `INC_REF` before emitting block cleanup, preventing returned objects from being deallocated.

### 33.4 — Emit Missing `INC_REF` on Identifier Field Reads & Array Literals
- **File:** `language/src/processes/assembler.cpp`
- In `visit(IdentifierNode)`, emit `INC_REF` when reading reference-typed fields.
- In `visit(ArrayLiteralExpression)`, emit `INC_REF` for reference-typed elements before `SET_ARRAY`.

### 33.5 — Support Postfix Increment and Decrement
- **File:** `language/src/processes/assembler.cpp`
- In `visit(UnaryExpression)`, inspect `uny->is_prefix`. For postfix operations, duplicate the original value before updating the target so the original value is left on the stack.

### 33.6 — Exception Cleanup Patches & Catch Variable ARC Management
- **File:** `language/src/processes/assembler.cpp`
- In `visit(TryStatement)`, emit `OpCode::JMP_TO_OUTER_CLEANUP` when `exception_cleanup_patches.empty()`.
- Emit cleanup (`GET_LOCAL` + `DEC_REF`) for `catch_clause->variable_memory_index` upon exiting catch clauses.

---

## Phase 34 — Semantic Analysis & Type System Binding

**Branch:** `phase-34-binder-types-vtables`  
**Criticality:** 🔴 Critical  
**Difficulty:** ⭐⭐⭐ Hard  

### 34.1 — Method Template Instantiation Key Harmonization
- **File:** `language/src/processes/binder.cpp`
- Harmonize the template instantiation cache key in `instantiate_template` with the signature format registered by `visit(MethodDeclaration)`.

### 34.2 — Reorder Base Class Resolution Before VTable Calculation
- **File:** `language/src/processes/binder.cpp`
- In `bind_types_and_memory()`, ensure all base class references are resolved before calculating vtables so derived classes correctly inherit virtual method slots.

### 34.3 — Robust VTable Override Signature Matching
- **File:** `language/src/processes/binder.cpp`
- In vtable override matching, split method names at `(` before searching for namespace qualifiers (`.`) to prevent qualified parameter types from corrupting method names.

### 34.4 — Type-Check Field Initializers in Pass 3
- **File:** `language/src/processes/binder.cpp`
- In Pass 3 semantic analysis, recurse into and type-check `FieldDeclaration::initializer`.

### 34.5 — Subtype Polymorphism in Method and Constructor Overload Resolution
- **File:** `language/src/processes/binder.cpp`
- In method overload resolution, support subtyping assignability (`is_assignable_from`) rather than strict type name equality.
- In constructor overload resolution, include array dimensions and full type names.


