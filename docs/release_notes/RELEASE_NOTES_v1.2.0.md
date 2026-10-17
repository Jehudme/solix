# Solix v1.2.0 Release Notes

**Release Date:** October 17, 2026  
**Tag:** [`v1.2.0`](https://github.com/Jehudme/solix/releases/tag/v1.2.0)  
**License:** MIT  

---

## 🚀 Overview

We are excited to announce the General Availability of **Solix v1.2.0**!

Building upon the memory model and allocator hardening of the v1.1 release series, Solix v1.2.0 introduces end-to-end packaging infrastructure, profile-driven dependency management, deterministic command lifecycle hooks, and a canonical multi-platform native library compilation pipeline.

With v1.2.0, Solix developers can bundle entire applications—including compiled bytecode, application assets and resources, manifests, and multi-platform native C/C++ companion libraries—into a single, highly compressed `.slxpac` archive that runs anywhere with `solix run app.slxpac`.

Every platform—Ubuntu Linux (GCC & Clang), macOS (Apple Clang), and Windows (Clang/MSVC)—as well as Linux Address/UndefinedBehavior Sanitizers (ASan/UBSan) has verified a **100% test pass rate across all 83 test suites** with zero warnings under `-Wall -Wextra`.

---

## ✨ Key Enhancements & Milestones

### 1. Universal Package Format (`.slxpac`) & Archiving Engine (Phases 6–10)
* **Portable Self-Contained Packages**: Introduced the `.slxpac` format, a Deflate-compressed portable ZIP archive encapsulating `bytecode.slxbin`, project `manifest.json`, bundled assets in `resources/`, and companion dynamic libraries in `lib/`.
* **CLI `solix pack` Command**: Introduced `solix pack` with `--profile <name>`, `--output <path>`, and multi-depth dependency asset harvesting.
* **Direct Archive Execution**: The Solix runtime VM and CLI launcher transparently ingest `.slxpac` archives directly via `solix run <archive>.slxpac`, mounting extracted binaries into isolated sandboxes and automatically linking all staged dynamic libraries.
* **Transitive Asset & Resource Staging**: Multi-depth project dependencies recursively harvest and stage their declared `resources/` and `lib/` companion libraries into the root distribution tree during both `solix build` and `solix pack`.

### 2. Profile-Aware Dependencies & Deterministic Lifecycle Hooks (Phases 11–14)
* **Dependency Profiles**: Projects can specify dependency targets with custom profiles (`{ "type": "project", "path": "../lib", "profile": "release" }`). Variables and profile-specific dependency graphs propagate seamlessly.
* **Pre-Compile & Post-Compile Lifecycle Commands**: Project manifests now support root-level and profile-level `pre_compile` / `precompile` and `post_compile` / `postcompile` shell command sequences.
* **Topologically Deterministic Execution**: Command hooks execute in strict dependency order:
  $$\text{Deepest Dependency Pre-Compile} \to \text{Root Pre-Compile} \to \text{Profile Pre-Compile} \to \text{Build/Pack} \to \text{Profile Post-Compile} \to \text{Root Post-Compile}$$
* **Host Platform Native Library Filtering**: The Solix VM runtime dynamic loader strictly validates and imports only the native library format matching the current host operating system (`.so` on Linux, `.dll` on Windows, `.dylib` on macOS), ignoring non-host shared libraries bundled within universal packages.

### 3. Canonical Solixlib Standard Library Restructuring (Phase 15)
* **First-Class Project Layout**: The official Solix standard library `solixlib` has been restructured from a split layout into a canonical, standalone Solix project directly at `solixlib/` (`manifest.json`, `src/solix/`, `lib/`, `native/`).
* **Automated CPack Staging**: Build and packaging rules stage the standard library seamlessly into `<install_prefix>/share/solix/solixlib`, with automatic discovery by `solix install` and `PackageManager`.

### 4. Universal Multi-Platform Zig Cross-Compilation Pipeline (Phases 16–17)
* **Zero-Setup Cross-Compilation**: Introduced `solixlib/scripts/build_native.py`, leveraging Zig's embedded Clang/LLVM cross-compiler to generate host `.so`, `.dll`, and `.dylib` native companion libraries simultaneously from a single machine.
* **Release Profile Hook Integration**: Configured `solixlib/manifest.json`'s `release` profile `pre_compile` hook to automatically compile all three operating system binaries before packaging into universal `.slxpac` archives.
* **Inline Dynamic Allocation**: Inlined runtime memory allocation primitives, eliminating external library linkage dependencies for native shared library plugins.

### 5. Standard Library Resource Management API (Phase 8)
* **`solix.core.Resource`**: Standard library module enabling applications to query, inspect, and read text and binary resources across both packaged `.slxpac` bundles and local directory structures.
* **Dual-Location Filesystem Resolution**: Applications seamlessly select between internal bundled resource streams and external host filesystem paths with absolute safety.

---

## 🧪 Quality & Verification Summary

* **Test Suites**: 100% of Catch2 test suites passed across all 83 test suites (83/83 suites, 0 failures).
* **Compiler Cleanliness**: 0 compiler warnings under `-Wall -Wextra` on Clang and GCC, and `/W4` on MSVC.
* **Continuous Integration**:
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
wget https://github.com/Jehudme/solix/releases/download/v1.2.0/solix-1.2.0-Linux.deb
sudo dpkg -i solix-1.2.0-Linux.deb
solix ide install
```

### Fedora / RHEL (`.rpm`)
```bash
wget https://github.com/Jehudme/solix/releases/download/v1.2.0/solix-1.2.0-Linux.rpm
sudo rpm -i solix-1.2.0-Linux.rpm
solix ide install
```

### Generic Linux Tarball
```bash
wget https://github.com/Jehudme/solix/releases/download/v1.2.0/solix-1.2.0-Linux.tar.gz
tar -xzf solix-1.2.0-Linux.tar.gz
export PATH="$PWD/bin:$PATH"
solix ide install
```

### macOS Installer
Download [`solix-1.2.0-Darwin.pkg`](https://github.com/Jehudme/solix/releases/download/v1.2.0/solix-1.2.0-Darwin.pkg) and run the installer, or unpack [`solix-1.2.0-Darwin.tar.gz`](https://github.com/Jehudme/solix/releases/download/v1.2.0/solix-1.2.0-Darwin.tar.gz).

### Windows Setup Wizard
Download and run [`solix-1.2.0-win64.exe`](https://github.com/Jehudme/solix/releases/download/v1.2.0/solix-1.2.0-win64.exe) to install Solix and automatically configure your system `PATH`.
