# Solix Packaging & Automated Release Guide

This guide covers building, packaging, and releasing the Solix programming language across Linux, macOS, and Windows.

---

## 1. Overview of Release Artifacts

Every Solix release includes binary installers and standalone portable archives:

| Platform | Format | Generator | Description |
|---|---|---|---|
| **Ubuntu / Debian** | `.deb` | CPack DEB | System package with `/usr/bin/solix` and dependencies |
| **Fedora / RHEL** | `.rpm` | CPack RPM | Red Hat package installer |
| **Linux (Generic)** | `.tar.gz` | CPack TGZ | Portable archive with precompiled standard library |
| **macOS** | `.pkg` | CPack productbuild | Native installer package |
| **macOS (Generic)** | `.tar.gz` | CPack TGZ | Portable archive for Apple Silicon & Intel |
| **Windows** | `.exe` | CPack NSIS | Setup wizard with automatic `PATH` environment setup |
| **Windows (Generic)**| `.zip` | CPack ZIP | Portable archive |
| **VS Code Extension**| `.vsix` | vsce | Language Server, syntax highlighting, and snippets |

---

## 2. Generating Release Packages Locally

To build and package Solix locally using CMake and CPack:

### Step 1: Configure and Build in Release Mode
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

### Step 2: Run CPack
To create all configured generators for your platform:
```bash
cd build
cpack -C Release
```

To build a specific package format:
```bash
# Portable archive:
cpack -G TGZ -C Release

# Debian package (on Linux):
cpack -G DEB -C Release

# Windows installer (on Windows):
cpack -G NSIS -C Release
```

---

## 3. Package Structure

Generated installers extract the following directory hierarchy:

```
<install_prefix>/
├── bin/
│   ├── solix            # CLI launcher & compiler runner
│   └── solix-lsp        # Language server daemon
├── share/
│   ├── doc/solix/
│   │   ├── LICENSE
│   │   └── README.md
│   └── solix/
│       ├── solixlib/    # Bundled official standard library
│       │   ├── solix.json
│       │   ├── lib/libsolixlib_native.so
│       │   └── src/solix/...
│       └── vscode/
│           └── solix-0.1.0.vsix # Bundled editor extension
```

---

## 4. Automated GitHub Actions CI/CD Pipeline

The `.github/workflows/release.yml` workflow triggers on semantic tag pushes (e.g. `v0.1.0`):

1. **Build Matrix**: Concurrently compiles Release binaries on `ubuntu-latest`, `macos-latest`, and `windows-latest`.
2. **CPack Execution**: Runs `cpack -C Release` on each host runner to build native `.deb`, `.rpm`, `.pkg`, `.exe`, `.tar.gz`, and `.zip` installers.
3. **Asset Publishing**: Publishes all installers, archives, and the VS Code `.vsix` extension to the GitHub Releases page automatically.
