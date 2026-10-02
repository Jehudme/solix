# Standard Library Architecture & Project Scaffolding

This document specifies the layout, build pipeline, and package lifecycle of the Solix standard library (`solix.stdlib`).

---

## 1. Architectural Philosophy

The Solix standard library is intentionally **decoupled** from the compiler and VM launcher executable (`solix`). Unlike traditional languages that bundle hardcoded stdlib sources or link them statically into the CLI runner:

1. **First-Class Package**: The standard library is a standard Solix project governed by [`solix.json`](file:///home/jehud/Projects/solix/stdlib/solix.json).
2. **Deterministic SemVer Versioning**: Projects declare dependencies on specific stdlib versions (e.g. `0.1.0`), preventing breaking runtime changes across compiler upgrades.
3. **Opt-In (`no_std` Ready)**: Embedded or minimal applications can omit standard library dependencies entirely.
4. **Dogfooding**: The standard library exercises the Solix package manager, project dependency resolution, multi-pass type binder, and dynamic shared library auto-discovery.

---

## 2. Directory Layout

```
stdlib/
├── solix.json                   # Standard library package manifest
├── CMakeLists.txt               # CMake target for native companion shared library
├── lib/                         # Auto-discovery target folder for native shared library
│   └── libsolix_stdlib_native.so / solix_stdlib_native.dll / libsolix_stdlib_native.dylib
├── native/                      # C/C++ native implementation routines
│   └── src/
│       └── register.cpp         # solix_register_natives registration hook
└── src/                         # Pure Solix declarations and implementations
    └── solix/
        └── core/
            └── Internal.slx     # Core internal package declaration
```

---

## 3. Package Manifest (`stdlib/solix.json`)

```json
{
  "project": "solix.stdlib",
  "version": "0.1.0",
  "author": "Solix Engine Team",
  "description": "Standard Library for the Solix Programming Language",
  "license": "MIT",
  "dependencies": [
    {
      "type": "source",
      "path": "src/solix/core/Internal.slx"
    }
  ],
  "profiles": {
    "debug": {
      "output_directory": "build/debug",
      "exe_filename": "stdlib.slxbin",
      "compilation": {
        "entry_point": "main",
        "multithreaded": false
      }
    },
    "release": {
      "output_directory": "build/release",
      "exe_filename": "stdlib.slxbin",
      "compilation": {
        "entry_point": "main",
        "multithreaded": false
      }
    }
  }
}
```

---

## 4. Native Companion Shared Library (`stdlib/CMakeLists.txt`)

The native companion library (`solix_stdlib_native`) is built via CMake and configured to deposit its compiled binaries directly into `stdlib/lib/`:

```cmake
add_library(solix_stdlib_native SHARED
    native/src/register.cpp
)

target_include_directories(solix_stdlib_native PRIVATE
    ${CMAKE_SOURCE_DIR}/language/include
    ${CMAKE_SOURCE_DIR}/language/src
)

set_target_properties(solix_stdlib_native PROPERTIES
    OUTPUT_NAME "solix_stdlib_native"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)
```

The native hook in `native/src/register.cpp` exports the standard registration function:

```cpp
#include "solix/native.h"
#include "solix/native_registry.hpp"

extern "C" SOLIX_EXPORT void solix_register_natives(solix::NativeRegistry &registry) {
    (void)registry;
    // Native function registrations will be added in subsequent phases
}
```

---

## 5. Build and Installation Lifecycle

### Step 1: Compile Native Library
Building the Solix engine builds the stdlib companion library:
```bash
cmake --build build
```
This compiles `solix_stdlib_native` and places it in `stdlib/lib/`.

### Step 2: Install into Local Registry
```bash
solix install stdlib
```
The package manager copies `solix.json`, `src/`, and `lib/` into:
`$SOLIX_HOME/installed/<hash_id>/`
and registers `solix.stdlib` version `0.1.0` in `$SOLIX_HOME/installed.json`.

### Step 3: Consumer Dependency Resolution
In consumer projects, reference `solix.stdlib` in `solix.json`:
```json
{
  "project": "my_application",
  "version": "1.0.0",
  "dependencies": [
    { "type": "source", "path": "src/main.slx" },
    { "type": "project", "name": "solix.stdlib", "version": "0.1.0" }
  ]
}
```
When running `solix run .`:
1. `DependencyManager` resolves `solix.stdlib` from `$SOLIX_HOME`.
2. The compiler ingests the standard library source files.
3. The VM auto-discovers and loads `solix_stdlib_native` from the package's `./lib/` folder.
