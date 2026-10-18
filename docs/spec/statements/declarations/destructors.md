# DestructorDeclaration (~ClassName())

## 1. Overview & Purpose

A `DestructorDeclaration` defines an automatic, deterministic cleanup routine executed immediately when a class instance's reference count reaches zero. Solix employs C++/C#-style syntax (`~ClassName()`) combined with Automatic Reference Counting (ARC) to provide deterministic Resource Acquisition Is Initialization (RAII).

### Language Invariants & Rules
- **Syntax**: `~ClassName()` declared directly inside a class body.
- **Signatures**: Takes zero parameters and returns no value.
- **Cardinality**: Exactly zero or one destructor per concrete or abstract class.
- **Interfaces**: Interfaces cannot declare destructors.
- **Modifiers**: Destructors cannot declare access or behavior modifiers (`static`, `virtual`, `override`, `abstract`, `native`, etc.).
- **Automatic Inheritance Chaining**: Derived class destructors execute first, followed automatically by the immediate superclass destructor in strict reverse hierarchy order (`~Child()` -> `~Parent()` -> `~Grandparent()`). No manual `super` call is required or allowed. If a subclass declares no destructor, the inherited ancestor destructor runs automatically.
- **Deterministic Exception Stack Unwinding**: When an exception is thrown, the Solix runtime unwinds call frames and lexical scopes. All live local references in unwound scopes are decremented in reverse declaration order (LIFO), invoking destructors before execution transfers to matching `catch` handlers.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Definition and VTable Registration
During compilation, each class with a destructor receives a compiled bytecode routine starting with local slot 0 mapped to `this`. During VM boot or class loading, the assembler emits `DEFINE_DESTRUCTOR <class_id> <func_ip>`, storing the entry point in the VM's `vtable_destructors` dispatch table:

```bytecode
DEFINE_DESTRUCTOR 12, 0x004A   // Associates Class ID 12 with Destructor Bytecode IP 0x004A
```

### Inheritance Chaining Mechanics
When a derived class compiles its destructor, the compiler automatically appends an invocation of its base class destructor with `this` (slot 0) passed as receiver, before terminating with `PUSH_NULL; RETURN;`:

```bytecode
// ~DerivedResource()
// --- User body begins ---
GET_GLOBAL 0                  // Load BaseResource.sequence
PUSH_INT 10
MUL
PUSH_INT 2
ADD
SET_GLOBAL 0
// --- Automatic Super Destructor Chaining ---
GET_LOCAL 0                   // Push 'this'
CALL <BaseResource.~BaseResource IP>
POP                           // Discard return null
PUSH_NULL
RETURN
```

### Deterministic Lifetime & Exception Unwinding
When an object reference drops out of scope or is reassigned to `null`:
1. `DEC_REF_SLOT` or `DEC_REF` decrements the object's reference counter.
2. When the counter reaches 0, `Memory::decrease_reference()` triggers `RuntimeContext::invoke_destructor(address, dtor_ip)`.
3. To protect against resurrection and re-entrant calls, the runtime temporarily increments the refcount to 1 and places the address in `active_destructors`.
4. If an exception triggers frame unwinding:
   - Each unwound block executes its cleanup segment (`DEC_REF_SLOT` for each active local in LIFO order).
   - Each unwound function frame cleans up parameters and receiver before jumping to outer exception handlers.
   - All destructors finish executing before control reaches the first instruction of the matching `catch` block.
