# Solix v1.2.1 Release Notes

**Release Date:** October 17, 2026  
**Tag:** [`v1.2.1`](https://github.com/Jehudme/solix/releases/tag/v1.2.1)  
**License:** MIT  

---

## 🚀 Overview

We are pleased to announce **Solix v1.2.1**!

This release introduces a clean, unified companion library directory structure (`libs/`), native CPack bundling for all companion libraries, generalized bundled project discovery, and automatic dependency installation for bundled companion packages including `solixlib` and `raylib`.

Every platform—Ubuntu Linux (GCC & Clang), macOS (Apple Clang), and Windows (Clang/MSVC)—as well as Linux Address/UndefinedBehavior Sanitizers (ASan/UBSan) has verified a **100% test pass rate across all 83 test suites** with zero compiler warnings under `-Wall -Wextra`.

---

## ✨ Key Enhancements & Bug Fixes

### 1. Unified Companion Library Hierarchy (`libs/`)
* **Consolidated Library Layout**: Both the standard library (`solixlib`) and companion hardware/graphics libraries (`raylib`) are now organized under a top-level `libs/` workspace folder:
  - `libs/solixlib/` — Official Standard Library with native runtime companions
  - `libs/raylib/` — Official 2D/3D hardware-accelerated Raylib 6.2 wrapper library
* **Clean Repository Separation**: Core language engine, compiler, runtime VM, and CLI tools remain strictly decoupled from peripheral standard and third-party companion libraries.

### 2. Multi-Library CPack Staging & Packaging
* **Automatic Distribution Bundling**: CMake and CPack packaging scripts have been updated to stage all companion libraries:
  - `libs/solixlib` stages to `${CMAKE_INSTALL_PREFIX}/share/solix/solixlib`
  - `libs/raylib` stages to `${CMAKE_INSTALL_PREFIX}/share/solix/raylib`
* **Zero-Configuration Distribution Packages**: All installer distributions (Debian `.deb`, RPM `.rpm`, macOS `.pkg` / `.tar.gz`, Windows NSIS `.exe` / `.zip`) include pre-bundled companion libraries ready for use out-of-the-box.

### 3. Generalized Bundled Library Auto-Discovery & Auto-Installation
* **Automatic Bundled Library Resolution**: `PackageManager::discover_bundled_library(name)` and `PackageManager::discover_all_bundled_libraries()` dynamically search installed prefixes (`<prefix>/share/solix/<name>`) and local development directories (`libs/<name>`).
* **Seamless Dependency Synchronization**: Whenever a Solix project declares a dependency on a bundled library (e.g. `solixlib` or `raylib`), the package manager automatically discovers, registers, and installs the companion package into the local cache (`~/.local/share/solix/installed/`) on demand.

### 4. Cross-Compilation Pipeline Portability Fixes
* **Multi-Platform Zig Companion Script Hardening**: Updated `libs/solixlib/scripts/build_native.py` and `libs/raylib/scripts/build_native.py` to seamlessly locate `core/include` regardless of nested directory hierarchies.

---

## 🧪 Quality & Verification Summary

* **Test Suites**: 100% of Catch2 test suites passed across all 83 test suites (83/83 suites, 0 failures).
* **Compiler Cleanliness**: 0 compiler warnings under `-Wall -Wextra` on Clang and GCC, and `/W4` on MSVC.
* **Release Artifacts**: Validated CMake install targets, CPack configuration, CLI version reporting, and automated bundled library discovery.

---

## 📦 Installation & Upgrading

### Linux (Debian / Ubuntu)
```bash
wget https://github.com/Jehudme/solix/releases/download/v1.2.1/solix-1.2.1-Linux.deb
sudo dpkg -i solix-1.2.1-Linux.deb
```

### Linux (RHEL / Fedora)
```bash
wget https://github.com/Jehudme/solix/releases/download/v1.2.1/solix-1.2.1-Linux.rpm
sudo rpm -i solix-1.2.1-Linux.rpm
```

### Linux / macOS (Tarball)
```bash
wget https://github.com/Jehudme/solix/releases/download/v1.2.1/solix-1.2.1-Linux.tar.gz
tar -xzf solix-1.2.1-Linux.tar.gz
sudo cp -r solix-1.2.1-Linux/* /usr/local/
```

### macOS (Installer)
Download [`solix-1.2.1-Darwin.pkg`](https://github.com/Jehudme/solix/releases/download/v1.2.1/solix-1.2.1-Darwin.pkg) and run the installer, or unpack [`solix-1.2.1-Darwin.tar.gz`](https://github.com/Jehudme/solix/releases/download/v1.2.1/solix-1.2.1-Darwin.tar.gz).

### Windows
Download and run [`solix-1.2.1-win64.exe`](https://github.com/Jehudme/solix/releases/download/v1.2.1/solix-1.2.1-win64.exe) to install Solix and automatically configure your system `PATH`.
