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
