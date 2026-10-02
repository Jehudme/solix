# Native Interoperability & Dynamic Shared Library Specification

## 1. Overview & Scope

The Solix Native Interoperability specification defines the binary interface, calling conventions, registration protocols, and cross-platform dynamic library loading mechanisms that allow the Solix Virtual Machine to execute native code compiled into external shared libraries (`.dll` on Windows, `.so` on Linux, `.dylib` on macOS).

Native interop enables:
- High-performance execution of CPU-intensive algorithms written in C or C++.
- Direct access to platform operating system APIs, graphics drivers, and hardware resources.
- Zero-overhead C-ABI function pointer dispatch without intermediary wrapper overhead.

---

## 2. Binary Interface & Calling Convention

### 2.1 The Native Function Pointer Signature (`NativeFunctionPtr`)

All native functions invoked by the Solix Virtual Machine conform to the canonical C-ABI signature:

```cpp
typedef uint64_t (*NativeFunctionPtr)(
    solix::RuntimeContext &vm,
    uint64_t self_address,
    uint64_t *args,
    size_t argc
);
```

#### Parameter Contract:
1. **`vm` (`solix::RuntimeContext &`)**:
   Direct reference to the executing VM context. Allows native implementations to access heap memory, allocate new instances, manipulate reference counters, or inspect environment states.
2. **`self_address` (`uint64_t`)**:
   - For **static native methods**: passed as `0`.
   - For **instance native methods**: passed as the 32-bit heap address (`Address`) of the target instance (`this` pointer), zero-extended to 64 bits.
3. **`args` (`uint64_t *`)**:
   Contiguous array of arguments passed to the call, ordered from first argument (`args[0]`) to last argument (`args[argc - 1]`).
4. **`argc` (`size_t`)**:
   The exact count of evaluated arguments passed at the call site.

#### Return Value Protocol:
The function returns a 64-bit unsigned integer (`uint64_t`). Primitive types are marshaled as follows:
- `bool`, `int8`, `uint8`, `int16`, `uint16`, `int32`, `uint32`: Zero-extended or sign-extended to 64 bits.
- `int64`, `uint64`: Returned directly as raw 64-bit bits.
- `float32`, `float64`: Bit-cast to `uint64_t` via `std::bit_cast<uint64_t>`.
- Reference types (objects, strings, arrays): Heap `Address` zero-extended to 64 bits (or `0` for `null`).
- `void`: Return value is ignored by the VM caller (typically returns `0`).

---

## 3. Object Layout & Memory Access

In the Solix heap, objects are arranged as contiguous blocks of 64-bit words:

```
Heap Address:
[obj + 0] : vtable_id (Class runtime type identifier)
[obj + 1] : Field 0 (Instance field offset 1)
[obj + 2] : Field 1 (Instance field offset 2)
...
[obj + N] : Field N - 1
```

Native functions inspect and mutate heap fields using header-inlined helpers on `vm.memory`:

```cpp
// Read 64-bit word at field offset
uint64_t val = vm.memory.read_u64(static_cast<solix::Address>(self_address), field_offset);

// Write 64-bit word at field offset
vm.memory.write_u64(static_cast<solix::Address>(self_address), field_offset, new_val);
```

---

## 4. Cross-Platform Dynamic Library Loader (`solix::SharedLibrary`)

Dynamic shared libraries are loaded using the RAII wrapper `solix::SharedLibrary`:

```cpp
namespace solix {

class SharedLibrary {
public:
  explicit SharedLibrary(const std::filesystem::path &path);
  ~SharedLibrary();

  bool load(const std::filesystem::path &path);
  void unload();
  bool is_loaded() const;

  void *get_symbol(const std::string &symbol_name) const;

  template <typename T>
  T get_symbol(const std::string &symbol_name) const;

  const std::filesystem::path &path() const;
};

} // namespace solix
```

### Platform Implementation Details:
- **Windows**:
  - Implementation: `LoadLibraryW(path.wstring().c_str())`.
  - Symbol Resolution: `GetProcAddress(handle, symbol_name.c_str())`.
  - Unloading: `FreeLibrary(handle)`.
  - Diagnostics: Formatted via `GetLastError()` and `FormatMessageA`.
- **Linux & macOS (POSIX)**:
  - Implementation: `dlopen(path.string().c_str(), RTLD_NOW | RTLD_LOCAL)`.
  - Symbol Resolution: `dlsym(handle, symbol_name.c_str())`.
  - Unloading: `dlclose(handle)`.
  - Diagnostics: Formatted via `dlerror()`.

Failure to load an invalid or missing library throws `solix::SharedLibraryException`.

---

## 5. Registry & Symbol Resolution Architecture

### 5.1 The Two-Tier Native Registry

Native function lookup operates through a two-tier model:

1. **Global Process Registry (`solix::NativeRegistry::global()`)**:
   A thread-safe singleton protected by a recursive mutex (`std::recursive_mutex`). Holds shared libraries and registered native symbols across the process.
2. **Fast Execution Table (`RuntimeContext::native_table`)**:
   A flat vector (`std::vector<NativeFunctionPtr>`) indexed directly by the bytecode method ID (`memory_index`). Populated during `op_DEFINE_NATIVE`, enabling lock-free $O(1)$ dispatch during bytecode execution.

### 5.2 Registration Protocols

External shared libraries provide functions to Solix through two mechanisms:

#### Mechanism 1: The Batch Registration Hook (`solix_register_natives`)
A shared library exports a standard entry point:
```cpp
extern "C" SOLIX_EXPORT void solix_register_natives(solix::NativeRegistry &registry);
```
When `NativeRegistry::load_library` loads the binary, it immediately searches for `solix_register_natives`. If present, it executes the hook, passing a reference to the registry. The hook registers functions in bulk:
```cpp
extern "C" SOLIX_EXPORT void solix_register_natives(solix::NativeRegistry &registry) {
    registry.register_function("NativeMath_add", &native_add);
    registry.register_function("Counter_increment", &counter_increment);
}
```

#### Mechanism 2: Direct Dynamic Symbol Export Fallback
If a function is not explicitly registered in the registry, the VM searches all loaded shared libraries dynamically using `dlsym` / `GetProcAddress`. Any exported C function conforming to `NativeFunctionPtr` can be resolved on-demand:
```cpp
extern "C" SOLIX_EXPORT uint64_t direct_export(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    return args[0] * 2;
}
```

### 5.3 Symbol Candidate Generation

When `op_DEFINE_NATIVE` encounters a native method in bytecode (e.g. `NativeMath.add(int32,int32)`), the VM tests the following candidates in order:

1. **Exact mangled signature**: `"NativeMath.add(int32,int32)"`
2. **Signature without parameters**: `"NativeMath.add"`
3. **Normalized C identifier**: `"NativeMath_add"`
4. **Unqualified method name**: `"add"`

The first candidate found in the local options, global registry, or loaded shared libraries is bound to the method ID.

---

## 6. Bytecode Emission & Runtime Execution

### 6.1 Definition: `OpCode::DEFINE_NATIVE`
- **Format**: `DEFINE_NATIVE <id: u32> <mangled_name: string>`
- **Execution**: The runtime resolves the name through candidate generation. If resolved, it writes the function pointer into `native_table[id]`. If unresolved, an informative warning diagnostic is logged.

### 6.2 Invocation: `OpCode::CALL_NATIVE`
- **Format**: `CALL_NATIVE <id: u32> <argc: u32> <is_static: u8>`
- **Execution**:
  1. Pops `argc` arguments from the stack in reverse order and packs into an array.
  2. If `!is_static`, pops `self_address` from the stack.
  3. Dispatches via `native_table[id](*this, self_address, args.data(), argc)`.
  4. Pushes the returned `uint64_t` onto the VM stack.
  5. If `id` is not registered or points to null, throws `std::runtime_error("Call to unknown native function: " + id)`.
