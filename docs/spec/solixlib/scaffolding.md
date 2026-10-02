# Solix Standard Library (`solixlib`) Architecture & Scaffolding

This document specifies the layout, build pipeline, and package lifecycle of the Solix standard library (`solixlib`).

---

## 1. Architectural Philosophy

The Solix standard library (`solixlib`) is intentionally **decoupled** from the compiler and VM launcher executable (`solix`). Unlike traditional languages that bundle hardcoded stdlib sources or link them statically into the CLI runner:

1. **Strict Codebase Separation**: The pure Solix code and the companion C/C++ native code reside in independent subdirectories (`solixlib/project/` and `solixlib/native/`), preventing language and build toolchain mixing.
2. **First-Class Package**: The standard library is a standard Solix project governed by [`solixlib/project/solix.json`](file:///home/jehud/Projects/solix/solixlib/project/solix.json).
3. **Deterministic SemVer Versioning**: Projects declare dependencies on specific stdlib versions (e.g. `0.1.0`), preventing breaking runtime changes across compiler upgrades.
4. **Opt-In (`no_std` Ready)**: Embedded or minimal applications can omit standard library dependencies entirely.
5. **Dogfooding**: The standard library exercises the Solix package manager, project dependency resolution, multi-pass type binder, and dynamic shared library auto-discovery.

---

## 2. Directory Layout

```
solixlib/
├── project/                     # 100% Pure Solix Project
│   ├── solix.json               # Package manifest (name: "solixlib", version: "0.1.0")
│   ├── lib/                     # Destination for compiled native shared library
│   │   └── libsolixlib_native.so / solixlib_native.dll / libsolixlib_native.dylib
│   └── src/                     # Pure Solix declarations and implementations
│       └── solix/
│           └── core/
│               └── Internal.slx # Core internal package declaration
│
└── native/                      # 100% Pure C/C++ CMake Project
    ├── CMakeLists.txt           # Dedicated CMake project building solixlib_native
    └── src/
        └── register.cpp         # solix_register_natives registration hook
```

---

## 3. Package Manifest (`solixlib/project/solix.json`)

```json
{
  "project": "solixlib",
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
      "exe_filename": "solixlib.slxbin",
      "compilation": {
        "entry_point": "main",
        "multithreaded": false
      }
    },
    "release": {
      "output_directory": "build/release",
      "exe_filename": "solixlib.slxbin",
      "compilation": {
        "entry_point": "main",
        "multithreaded": false
      }
    }
  }
}
```

---

## 4. Native Companion Shared Library (`solixlib/native/CMakeLists.txt`)

The native companion library (`solixlib_native`) is built via CMake and configured to deposit its compiled binaries directly into `solixlib/project/lib/`:

```cmake
add_library(solixlib_native SHARED
    src/register.cpp
)

target_include_directories(solixlib_native PRIVATE
    ${CMAKE_SOURCE_DIR}/language/include
    ${CMAKE_SOURCE_DIR}/language/src
)

set(SOLIXLIB_OUTPUT_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../project/lib")

set_target_properties(solixlib_native PROPERTIES
    OUTPUT_NAME "solixlib_native"
    RUNTIME_OUTPUT_DIRECTORY "${SOLIXLIB_OUTPUT_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${SOLIXLIB_OUTPUT_LIB_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${SOLIXLIB_OUTPUT_LIB_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${SOLIXLIB_OUTPUT_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${SOLIXLIB_OUTPUT_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${SOLIXLIB_OUTPUT_LIB_DIR}"
)
```

The native hook in `solixlib/native/src/register.cpp` exports the standard registration function:

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

### Step 1: Compile Native Companion
Building the Solix engine automatically builds `solixlib_native`:
```bash
cmake --build build
```
This compiles `solixlib_native` and places it in `solixlib/project/lib/`.

### Step 2: Install into Local Registry
```bash
solix install solixlib/project
```
The package manager copies `solix.json`, `src/`, and `lib/` into:
`$SOLIX_HOME/installed/<hash_id>/`
and registers `solixlib` version `0.1.0` in `$SOLIX_HOME/installed.json`.

### Step 3: Consumer Dependency Resolution
In consumer projects, reference `solixlib` in `solix.json`:
```json
{
  "project": "my_application",
  "version": "1.0.0",
  "dependencies": [
    { "type": "source", "path": "src/main.slx" },
    { "type": "project", "name": "solixlib", "version": "0.1.0" }
  ]
}
```
When running `solix run .`:
1. `DependencyManager` resolves `solixlib` from `$SOLIX_HOME`.
2. The compiler ingests the standard library source files.
3. The VM auto-discovers and loads `solixlib_native` from the package's `./lib/` folder.
