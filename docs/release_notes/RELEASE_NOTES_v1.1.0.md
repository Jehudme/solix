# Solix v1.1.0 Release Notes

**Release Date:** October 14, 2026  
**Tag:** [`v1.1.0`](https://github.com/Jehudme/solix/releases/tag/v1.1.0)  
**License:** MIT  

---

## 🚀 Overview

We are thrilled to announce the release of **Solix v1.1.0**!

Following the v1.0.0 General Availability milestone, v1.1.0 introduces fundamental architectural hardening across the virtual machine memory subsystem, a high-performance native segregated-fit heap allocator, robust Automatic Reference Counting (ARC) destruction guarantees, and modern project manifest standardization.

Every platform—Ubuntu Linux (GCC & Clang), macOS (Apple Clang), and Windows (Clang/MSVC)—as well as Linux Address/UndefinedBehavior Sanitizers (ASan/UBSan) has verified 100% test pass rates across the entire Solix test suite.

---

## ✨ Key Enhancements & Milestones

### 1. Byte-Addressed Memory Model & Alignment Hardening (Phase 4)
* **Byte-Addressed Addressing Subsystem**: Complete transition of the Solix runtime virtual machine to standard byte-level addressing, eliminating word/byte pointer mismatch and off-by-index errors.
* **8-Byte Memory Alignment**: Enforced 8-byte boundary alignment across all stack frames, operand slots, and heap-allocated object blocks, guaranteeing safe native type punning, vectorization readiness, and zero unaligned access faults on strict architectures.
* **Endian-Safe Data Serialization**: Byte-level data loads, stores, and bytecode instruction fetch routines are fully portable across endian architectures.
* **Architectural Reference**: Comprehensive architectural documentation published at [`docs/architecture/memory_model.md`](https://github.com/Jehudme/solix/blob/master/docs/architecture/memory_model.md).

### 2. High-Performance Segregated-Fit Heap Allocator (Phase 5)
* **Segregated Free Lists**: Heap memory allocation now organizes available blocks into segregated size-class bins (`free_by_size`), enabling $O(1)$ allocation times for common object and array allocations.
* **Address Indexing & Bidirectional Coalescing**: Memory blocks maintain an address-sorted index (`free_by_addr`). When memory is freed, adjacent left and right free chunks are immediately merged in $O(1)$ time, virtually eliminating heap fragmentation under heavy allocation workloads.
* **Dynamic Top-of-Heap Retraction**: Freeing allocations at the top boundary of the heap dynamically retracts the heap pointer back toward the baseline, returning virtual address space and preventing unbounded memory growth.
* **Stack Bounds Hardening**: VM operand stack and call stack frames enforce strict overflow and underflow boundary validation on every push, pop, invocation, and return instruction.

### 3. ARC Destruction Safety & Type-Confusion Prevention (Phase 5)
* **Explicit Type-Tagged Deallocation**: Object headers store explicit type identifiers, completely preventing type confusion and invalid dynamic VTable calls during object destruction.
* **Array Mutation Reference Tracking**: The `op_SET_ARRAY` instruction now strictly decrements the reference count of the displaced element and increments the incoming reference count, preventing subtle memory leaks or premature frees during array mutation.
* **Runtime Dynamic Type Verification**: Introduced and hardened `op_INSTANCEOF` and `op_CAST_CHECK` instructions for runtime safety during polymorphic downcasting and interface queries.
* **Deep Cyclic & Graph Reclamation**: Strengthened LIFO destructor traversal for complex object graphs, nested collections, and boxed interface references.
* **Architectural Reference**: Detailed documentation published at [`docs/architecture/arc_internals.md`](https://github.com/Jehudme/solix/blob/master/docs/architecture/arc_internals.md).

### 4. Canonical Project Manifest Modernization (Phases 1 - 3)
* **Canonical `solix.json` Manifest**: Transitioned project manifests to standard `solix.json` across compiler, runtime, package manager (`solix build`, `solix install`), and language server daemon (`solix-lsp`).
* **Legacy Compatibility**: Full dual-format backward compatibility preserving support for existing `manifest.json` projects with automated deprecation warnings and migration guidance.
* **Scaffolding Overhaul**: `solix new` generates modern `solix.json` configurations out of the box with default debug and release profiles.
* **Module & Dependency Resolution**: Robust multi-root package graph resolution, canonical namespace mapping, and cycle detection.

---

## 🧪 Quality & Verification Summary

* **Test Suite**: 100% of Catch2 test suites passed across all suites and sections.
* **Continuous Integration**: 100% green pipeline across all matrix targets on GitHub Actions:
  * `ubuntu-latest (gcc)`: ✅ 100% Passed
  * `ubuntu-latest (clang)`: ✅ 100% Passed
  * `macos-latest (apple-clang)`: ✅ 100% Passed
  * `windows-latest (clang)`: ✅ 100% Passed
  * `Linux ASan/UBSan (gcc)`: ✅ 100% Clean (Zero memory leaks, zero undefined behavior)
* **Packaging Verification**: Native CPack multi-platform installers staged and validated for Windows (NSIS `.exe` & `.zip`), macOS (`.pkg` & `.tar.gz`), Linux (`.deb`, `.rpm`, `.tar.gz`), and VS Code Extension (`.vsix`).

---

## 📥 Installation

### Debian / Ubuntu (`.deb`)
```bash
wget https://github.com/Jehudme/solix/releases/download/v1.1.0/solix-1.1.0-Linux.deb
sudo dpkg -i solix-1.1.0-Linux.deb
solix ide install
```

### Fedora / RHEL (`.rpm`)
```bash
wget https://github.com/Jehudme/solix/releases/download/v1.1.0/solix-1.1.0-Linux.rpm
sudo rpm -i solix-1.1.0-Linux.rpm
solix ide install
```

### Generic Linux Tarball
```bash
wget https://github.com/Jehudme/solix/releases/download/v1.1.0/solix-1.1.0-Linux.tar.gz
tar -xzf solix-1.1.0-Linux.tar.gz
export PATH="$PWD/bin:$PATH"
solix ide install
```

### macOS Installer
Download [`solix-1.1.0-Darwin.pkg`](https://github.com/Jehudme/solix/releases/download/v1.1.0/solix-1.1.0-Darwin.pkg) and run the installer, or unpack [`solix-1.1.0-Darwin.tar.gz`](https://github.com/Jehudme/solix/releases/download/v1.1.0/solix-1.1.0-Darwin.tar.gz).

### Windows Setup Wizard
Download and run [`solix-1.1.0-win64.exe`](https://github.com/Jehudme/solix/releases/download/v1.1.0/solix-1.1.0-win64.exe) to install Solix and automatically configure your system `PATH`.
