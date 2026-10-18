# Solix v3.0.0 Release Notes

**Release Date:** October 18, 2026  
**Tag:** [`v3.0.0`](https://github.com/Jehudme/solix/releases/tag/v3.0.0)  
**License:** MIT  

---

## 🚀 Overview

We are thrilled to announce **Solix v3.0.0**!

Solix v3.0.0 is a major milestone release delivering a modern, robust command-line experience, comprehensive package management workflows, clean distribution staging, direct `.slxpac` package bundle execution and installation, active profile dependency overriding, and resilient execution guards across Linux, macOS, and Windows.

Additionally, this release upgrades the companion **Raylib** library to version **1.0.0**, packages both `solixlib` and `raylib` into standalone `.slxpac` archives alongside pre-staged distributions, and integrates automated post-install registration in CPack packages across Debian (`.deb`), RPM (`.rpm`), and Windows NSIS (`.exe`).

Every platform—Ubuntu Linux (GCC & Clang), macOS (Apple Clang), and Windows (Clang/MSVC)—as well as Linux Address/UndefinedBehavior Sanitizers (ASan/UBSan) has verified a **100% test pass rate across all 84 test suites** with zero compiler warnings under `-Wall -Wextra`.

---

## ✨ Key Enhancements & New Features

### 1. Unified Command Scaffolding & Aliasing
* **`solix create` with `new` Alias**: Registered `solix create` as the primary project scaffolding command, retaining `solix new` as a transparent backward-compatible alias.
* **Plugin Scaffolding**: Added `-T, --type plugin` to generate native plugin projects pre-configured with manifest and starter sources alongside `app` and `library`.

### 2. Modernized Package Management
* **`solix list` Type Column & Advanced Queries**:
  - Added dedicated `TYPE` column (`app`, `library`, `plugin`).
  - Added `-t, --type <type>` filtering.
  - Added `-f, --filter <query>` case-insensitive search by name, description, or ID.
  - Added `-s, --sort <field>` (`name`, `version`, `type`, `date`) and `--reverse` sorting.
  - Added `--json` flag emitting a formatted JSON array of all installed packages.
* **`solix details` Scripting Flags & Direct Archive Inspection**:
  - Query flags for scripting: `-v` (version), `-n` (name), `-t` (type), `-d` (description), `-a` (author), `-l` (license), `--id`, `--path`, `--entry`, `--json`.
  - Convenient `<name>@<version>` syntax parsing for unambiguous version targeting.
  - Direct inspection of `.slxpac` package bundles without extracting them.
* **`solix install` & `solix archive` Enhancements**:
  - Replaced legacy `--as-is` with `--archive` (retaining `--as-is` as backward-compatible alias).
  - Direct installation of `.slxpac` bundles: `solix install my_app.slxpac` extracts and installs the package locally as an application.
  - Added `--alias, --as <name>` to install or stage distributions under an alternative project name.

### 3. Streamlined `.slxpac` Packaging Pipeline
* **Unified Staging Architecture**: Refactored `solix pack` to leverage the clean staging distribution pipeline from `solix archive`.
* **Packaging Installed Applications**: Run `solix pack <name>@<version> -o <out.slxpac>` to package any locally installed application directly into a `.slxpac` bundle.
* **Reduced Manifest Preservation**: Bundles now preserve root `manifest.json` metadata for instant inspection and seamless installation.

### 4. Active Profile Additional Dependencies Overrides
* **Profile-Specific Dependency Control**: Active profiles can declare `compilation.additional_dependencies` with custom profiles (e.g. `{"name": "libA", "profile": "prod"}`) that cleanly override root dependency profiles declared in `manifest.json`.

### 5. Robust Execution Guards & Edge-Case Protection
* **Library Execution Rejection**: `solix run` cleanly rejects library projects and library `.slxpac` bundles with:  
  `Cannot run '<name>' because it is a library (libraries do not contain an executable entry point).`
* **Pre-Built Profile Warning**: Emits a warning when `-P/--profile` is passed to a pre-built binary, installed app, or `.slxpac` package bundle.
* **Safe Path Dependency Resolution**: Auto-installs path dependencies without overwriting existing registry installations.

### 6. Companion Library Bundling & CPack Automation
* **Raylib v1.0.0**: Upgraded official Raylib companion library to v1.0.0.
* **Automated CPack Installation**: Updated Debian `postinst`, RPM `%post`, and NSIS installer scripts to automatically execute `solix install --archive` for `share/solix/solixlib` and `share/solix/raylib`.
* **Release Artifacts**: Release pipeline automatically builds standalone `solixlib.slxpac` and `raylib.slxpac` bundles.

---

## 🧪 Quality & Verification Summary

* **Test Suites**: 100% of Catch2 test suites passed across all 84 test suites (84/84 suites, 0 failures, 1300+ assertions passed).
* **Compiler Cleanliness**: 0 compiler warnings under `-Wall -Wextra` on Clang and GCC, and `/W4` on MSVC.
* **Sanitizers**: ASan and UBSan runs pass cleanly with zero memory leaks, heap-use-after-free, or undefined behavior faults.
