# Solix v1.0.0 Release Notes

**Release Date:** October 8, 2026  
**Tag:** [`v1.0.0`](https://github.com/Jehudme/solix/releases/tag/v1.0.0)  
**License:** MIT  

---

## 🚀 Overview

We are thrilled to announce the official **General Availability release of Solix v1.0.0**! 

Solix is a modern, statically-typed, object-oriented systems programming language featuring deterministic Automatic Reference Counting (ARC), dynamic VTable dispatch, high-performance bytecode virtual machine execution, native C ABI interoperability, and an integrated end-to-end toolchain.

This milestone release introduces complete multi-OS distribution packaging (Windows setup wizard, macOS installer package, Linux DEB/RPM/tarballs), toolchain zero-configuration standard library auto-seeding, and automated VS Code / Cursor IDE extension installation.

---

## ✨ Key Features & Capabilities

### 1. Unified Toolchain (`solix`)
A single, modular command-line interface managing compilation, execution, build profiles, project scaffolding, package management, and IDE configuration:
* `solix compile`: Compiles `.slx` source modules into optimized `.slxb` bytecode binaries.
* `solix run`: Executes compiled bytecode binaries with standard streams, command-line arguments, and configurable VM heap/stack constraints.
* `solix build`: Comprehensive project builder supporting multi-profile builds (`debug`, `release`), transitive dependency graphs, circularity elimination, SemVer conflict resolution, and custom output artifact naming.
* `solix new`: Scaffolds standard Solix application projects with valid `solix.json` manifests, directory layouts, and entry points.
* `solix install` / `uninstall` / `list` / `details`: Isolated, reproducible local package management with deterministic 16-character SHA-256 package identifiers stored under `$SOLIX_HOME`.
* `solix ide install`: Automated detection and extension installation for Visual Studio Code, VS Code Insiders, Cursor AI Editor, and VSCodium.
* `solix version`: Semantic version reporting (`1.0.0`).

### 2. Standard Library (`solixlib`)
A production-ready standard library written entirely in Solix and bundled natively with every installer:
* **Core & Memory**: Primitives, string manipulation, `IStringable`, boxing/unboxing, object lifecycle management.
* **Collections**: Dynamically resizing generic `List<T>`, key-value hash maps, sets, and queue collections.
* **I/O & File System**: Stream readers/writers, `MemoryStream`, recursive file and directory operations (`File`, `Directory`, `Path`).
* **Systems & Processes**: Subprocess spawning, synchronous process execution with standard I/O redirection, environment variable queries.
* **Math & Numerics**: Trigonometric functions, logarithms, boundary handling, random number generation (`Random`).
* **Date & Time**: `DateTime`, `Instant`, `Duration`, high-precision monotonic timing (`Stopwatch`).
* **Cryptography**: RFC-compliant hash algorithms (MD5, SHA-256) with native acceleration.
* **Native Companion (`solixlib_native`)**: Precompiled native C/C++ shared library companion linked seamlessly across platforms.

### 3. Language Features & Type System
* **Deterministic ARC Memory Management**: Deterministic object cleanup without stop-the-world garbage collection pauses, weak references to prevent cyclic leaks, and LIFO destruction.
* **Object-Oriented Programming**: Single inheritance, multiple interface implementation, virtual and interface table dynamic dispatch.
* **Generics & Parameterized Types**: Generic classes, generic interfaces, generic collections, and safe `default(T)` slot clearance.
* **First-Class Functions**: Lambdas, lexical closures, function pointers, and higher-order functions.
* **Robust Control Flow & Exceptions**: `try` / `catch` / `finally` structured exception handling, pattern matching, `switch` expressions, and loops (`for`, `while`, `do-while`).
* **Native Interoperability (FFI)**: Direct dynamic linking to native C shared libraries (`.so`, `.dylib`, `.dll`) via `extern native` declarations.

### 4. Language Server Protocol (`solix-lsp`) & IDE Integration
* Standalone LSP daemon (`solix-lsp`) conforming to the official Language Server Protocol specification.
* Real-time semantic diagnostics, tokenization, error highlighting, and syntax validation.
* Context-aware autocompletion: member access (`foo.`), general scope completions, type annotations, and import statements.
* Hover documentation, symbol definitions, and document outline symbols.
* Pre-packaged VS Code / Cursor extension (`solix-1.0.0.vsix`) with syntax grammar, language configurations, and automatic client-server lifecycle management.

### 5. Zero-Configuration Auto-Discovery & Seeding
* Installed toolchain automatically discovers bundled `solixlib` relative to the executable path (`<prefix>/share/solix/solixlib`).
* When building or compiling a project requiring `solixlib`, the compiler automatically seeds the standard library into the user's `$SOLIX_HOME` on first use without manual intervention.

---

## 📦 Download & Verification Matrix

| Platform | Installer / Archive | Size | SHA-256 Checksum |
|---|---|---|---|
| **Windows x64** | [`solix-1.0.0-win64.exe`](https://github.com/Jehudme/solix/releases/download/v1.0.0/solix-1.0.0-win64.exe) | 2.53 MiB | `498d0d70e020d5246566c8f49cd3e7c5e99d8f48d59e0906f2371e6d7ca02411` |
| **Windows x64** | [`solix-1.0.0-win64.zip`](https://github.com/Jehudme/solix/releases/download/v1.0.0/solix-1.0.0-win64.zip) | 3.14 MiB | `f98e11f3c05f6d5b1abb1f50f4ced237e77ed953e79be14174307bf6edffb2d9` |
| **Linux (Debian/Ubuntu)** | [`solix-1.0.0-Linux.deb`](https://github.com/Jehudme/solix/releases/download/v1.0.0/solix-1.0.0-Linux.deb) | 2.93 MiB | `843fb6cbf5facfbb8929e428d2f15477e92fb07d5673c3c2b95edadbb283b927` |
| **Linux (Fedora/RHEL)** | [`solix-1.0.0-Linux.rpm`](https://github.com/Jehudme/solix/releases/download/v1.0.0/solix-1.0.0-Linux.rpm) | 2.93 MiB | `4da5be6746087572801fc05efbe677e5615cdbabd3d7b673a7cf8b5fda273d52` |
| **Linux (Generic)** | [`solix-1.0.0-Linux.tar.gz`](https://github.com/Jehudme/solix/releases/download/v1.0.0/solix-1.0.0-Linux.tar.gz) | 2.93 MiB | `ff6a261b9dfa7bf61a995099ab9e8622f391547f71f800a5b160f8e78b3a47c2` |
| **macOS (Apple Silicon)** | [`solix-1.0.0-Darwin.pkg`](https://github.com/Jehudme/solix/releases/download/v1.0.0/solix-1.0.0-Darwin.pkg) | 1.82 KiB | `5714d295eabf5569f1d27372285ff104cf7e3c56e8d122397f68f496b1f44e0b` |
| **macOS (Apple Silicon)** | [`solix-1.0.0-Darwin.tar.gz`](https://github.com/Jehudme/solix/releases/download/v1.0.0/solix-1.0.0-Darwin.tar.gz) | 2.49 MiB | `af156ec79de558e5ecf99fb77439882e6b354a115eaf68db20868d66d80b7b85` |
| **VS Code Extension** | [`solix-1.0.0.vsix`](https://github.com/Jehudme/solix/releases/download/v1.0.0/solix-1.0.0.vsix) | 463 KiB | `a5c2cfea4afc76d0deb13a9b22117d824bdfb410a998b8d877d27c90aef9eadf` |

---

## 🛠️ Installation Instructions

### Windows
1. Download [`solix-1.0.0-win64.exe`](https://github.com/Jehudme/solix/releases/download/v1.0.0/solix-1.0.0-win64.exe).
2. Run the installer wizard. It will install the toolchain to `C:\Program Files\Solix` and automatically add `C:\Program Files\Solix\bin` to your system `PATH`.
3. Open PowerShell or Command Prompt:
   ```cmd
   solix version
   solix ide install
   ```

### Linux

#### Debian / Ubuntu / Mint (`.deb`)
```bash
wget https://github.com/Jehudme/solix/releases/download/v1.0.0/solix-1.0.0-Linux.deb
sudo dpkg -i solix-1.0.0-Linux.deb
solix ide install
```

#### Fedora / RHEL / openSUSE (`.rpm`)
```bash
wget https://github.com/Jehudme/solix/releases/download/v1.0.0/solix-1.0.0-Linux.rpm
sudo rpm -i solix-1.0.0-Linux.rpm
solix ide install
```

#### Generic Linux Tarball
```bash
wget https://github.com/Jehudme/solix/releases/download/v1.0.0/solix-1.0.0-Linux.tar.gz
tar -xzf solix-1.0.0-Linux.tar.gz
export PATH="$PWD/bin:$PATH"
solix ide install
```

### macOS
1. Download [`solix-1.0.0-Darwin.pkg`](https://github.com/Jehudme/solix/releases/download/v1.0.0/solix-1.0.0-Darwin.pkg) and run the installer, OR unpack [`solix-1.0.0-Darwin.tar.gz`](https://github.com/Jehudme/solix/releases/download/v1.0.0/solix-1.0.0-Darwin.tar.gz).
2. Verify installation:
   ```bash
   solix version
   solix ide install
   ```

---

## ⚡ Quickstart Guide

### 1. Scaffold a New Project
```bash
solix new hello_world
cd hello_world
```

### 2. View Project Structure
```text
hello_world/
├── solix.json          # Project manifest, dependencies, and build profiles
└── src/
    └── main.slx        # Main application source code
```

### 3. Build & Run
```bash
# Build the project
solix build

# Execute the built binary
solix run build/debug/hello_world.slxb
```

### 4. Install the IDE Extension
```bash
# Automatically detects and installs into VS Code, Cursor, or VSCodium:
solix ide install

# Or target a specific editor:
solix ide install --editor cursor
```

---

## 🧪 Quality & Verification Summary

* **Test Suite Coverage**: 76 comprehensive test suites consisting of 300+ Catch2 positive and negative test cases.
* **Continuous Integration**: 100% passing across all matrix targets on GitHub Actions:
  * `ubuntu-latest (gcc)`: ✅ 100% Passed (76/76)
  * `ubuntu-latest (clang)`: ✅ 100% Passed (76/76)
  * `macos-latest (apple-clang)`: ✅ 100% Passed (76/76)
  * `windows-latest (clang)`: ✅ 100% Passed (76/76)
* **Packaging Integrity**: CPack installers staged and verified on each target operating system.
