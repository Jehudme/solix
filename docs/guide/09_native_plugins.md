# Native C/C++ Plugins and Shared Libraries

Solix allows developers to interface directly with compiled native C and C++ shared libraries (`.dll` on Windows, `.so` on Linux, and `.dylib` on macOS). Through native interoperability, you can implement high-performance numeric computations, bind existing C libraries, or interact with operating system hardware interfaces.

---

## 1. Declaring Native Methods in Solix

In Solix, native methods are declared using the `native` keyword. Like abstract methods, native methods terminate with a semicolon and do not have an inline body:

```solix
public class MathLib {
    // Static native method
    public static native int32 fast_add(int32 a, int32 b);
    public static native int32 fast_multiply(int32 a, int32 b);
}

public class Counter {
    public int32 value;

    // Instance native method operating on 'this'
    public native int32 increment();
}
```

---

## 2. The Native C++ Function Signature

Every native function callable by the Solix Virtual Machine must conform to the `NativeFunctionPtr` C-ABI signature:

```cpp
#include <solix/native.h>
#include <solix/runtime.hpp>

uint64_t my_native_function(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc);
```

### Parameter Breakdown:
- **`vm` (`solix::RuntimeContext &`)**: Reference to the executing virtual machine. Gives you direct access to the VM heap, memory allocation, and runtime options.
- **`self` (`uint64_t`)**: 
  - For `static` methods: always `0`.
  - For instance methods: the 32-bit heap `Address` of the calling object (`this`), zero-extended to 64 bits.
- **`args` (`uint64_t *`)**: Array of 64-bit machine words containing the arguments in left-to-right order (`args[0]`, `args[1]`, etc.).
- **`argc` (`size_t`)**: The count of arguments passed to the call.

---

## 3. Authoring a Native Plugin in C++

Here is a complete C++ plugin file (`math_plugin.cpp`):

```cpp
#include <solix/native.h>
#include <solix/runtime.hpp>
#include <solix/native_registry.hpp>

// 1. Static addition
static uint64_t native_fast_add(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm;
    (void)self;
    (void)argc;
    int32_t a = static_cast<int32_t>(args[0]);
    int32_t b = static_cast<int32_t>(args[1]);
    return static_cast<uint64_t>(a + b);
}

// 2. Instance method modifying object field
static uint64_t native_counter_increment(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)args;
    (void)argc;
    solix::Address obj = static_cast<solix::Address>(self);

    // In Solix, field 0 is reserved for vtable_id; the first user field is at offset 1
    uint64_t current_val = vm.memory.read_u64(obj, 1);
    current_val += 1;
    vm.memory.write_u64(obj, 1, current_val);

    return current_val;
}

// 3. Batch Registration Hook exported to the Solix runtime
extern "C" SOLIX_EXPORT void solix_register_natives(solix::NativeRegistry &registry) {
    registry.register_function("MathLib_fast_add", native_fast_add);
    registry.register_function("Counter_increment", native_counter_increment);
}
```

### Direct Export Fallback (Alternative to `solix_register_natives`)
If you prefer not to write a registration hook, you can directly export any C function conforming to `NativeFunctionPtr`. Solix dynamically discovers exported symbols matching the method name:

```cpp
extern "C" SOLIX_EXPORT uint64_t fast_multiply(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    return args[0] * args[1];
}
```

---

## 4. Compiling the Shared Library

You can compile your native plugin using any standard C++ compiler or CMake:

### Using GCC / Clang on Linux:
```bash
clang++ -shared -fPIC -std=c++17 math_plugin.cpp -I/path/to/solix/include -o libmath_plugin.so
```

### Using Apple Clang on macOS:
```bash
clang++ -shared -fPIC -std=c++17 math_plugin.cpp -I/path/to/solix/include -o libmath_plugin.dylib
```

### Using MSVC / Clang on Windows:
```bash
clang++ -shared -std=c++17 math_plugin.cpp -I/path/to/solix/include -o math_plugin.dll
```

### Minimal `CMakeLists.txt` for Plugin Projects:
```cmake
cmake_minimum_required(VERSION 3.20)
project(my_solix_plugin CXX)

set(CMAKE_CXX_STANDARD 17)

add_library(math_plugin SHARED math_plugin.cpp)
target_include_directories(math_plugin PRIVATE /path/to/solix/language/include)
```

---

## 5. Using the Plugin in Solix

Solix makes loading and running your native plugin effortless:

### Method A: Zero-Config Auto-Discovery (Recommended)
Place your compiled `.dll`, `.so`, or `.dylib` in:
- A `lib/` directory inside your project (`my_project/lib/math_plugin.so`).
- Or right next to your compiled `.slxbin` bytecode file.

When you run `solix run`, Solix automatically discovers and loads all native libraries found in `lib/`:
```bash
solix run
```

### Method B: Manifest Declaration (`solix.json`)
Explicitly register your libraries in your project manifest:
```json
{
  "name": "my_app",
  "version": "1.0.0",
  "native_libraries": [
    "lib/math_plugin.so"
  ]
}
```

### Method C: Command-Line Flag (`-L, --native-lib`)
Pass one or more libraries directly on the command line when running standalone bytecode:
```bash
solix run app.slxbin -L ./lib/math_plugin.so
```

---

## 6. End-to-End Example

### Project Structure
```text
calculator/
├── solix.json
├── lib/
│   └── libmath_plugin.so    # Compiled C++ plugin
└── src/
    └── main.slx             # Solix code
```

### `src/main.slx`
```solix
public class MathLib {
    public static native int32 fast_add(int32 a, int32 b);
}

static int32 main() {
    int32 sum = MathLib.fast_add(100, 250);
    return sum == 350 ? 0 : 1;
}
```

### Run
```bash
solix run
```
Solix automatically discovers `lib/libmath_plugin.so`, binds `MathLib.fast_add`, and executes at raw C speed!
