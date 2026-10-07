# Solix CLI Subcommand Test Specification

This document is the master test specification for the Solix CLI command suite (Phase 11). It defines the comprehensive matrix of **Positive Test Scenarios** and **Negative Test Scenarios** for every CLI subcommand provided by the Solix toolchain: `compile`, `run`, `build`, `new`, `install`, `uninstall`, `list`, and `details`.

---

## Table of Contents

- [Part I: Compilation & Execution Commands](#part-i-compilation--execution-commands)
  - [CompileCommand](#compilecommand)
  - [RunCommand](#runcommand)
  - [BuildCommand](#buildcommand)
- [Part II: Project Scaffolding Commands](#part-ii-project-scaffolding-commands)
  - [NewCommand](#newcommand)
- [Part III: Local Package Management Commands](#part-iii-local-package-management-commands)
  - [InstallCommand](#installcommand)
  - [UninstallCommand](#uninstallcommand)
  - [ListCommand](#listcommand)
  - [DetailsCommand](#detailscommand)

---

# Part I: Compilation & Execution Commands

## CompileCommand

*Command*: `solix compile <files>... [options]`

### Positive Test Scenarios

#### Case 1.1: Single Source File Compilation to Default Output [IMPLEMENTED]
- **Command**: `solix compile main.slx`
- **Setup**: `main.slx` with valid class and entry point.
- **Expected**: Exits 0; creates `out.slxb` in working directory; file contains valid Solix bytecode.

#### Case 1.2: Custom Output Bytecode Path [IMPLEMENTED]
- **Command**: `solix compile main.slx -o bin/app.slxb`
- **Setup**: `main.slx` with valid code.
- **Expected**: Exits 0; creates `bin/app.slxb` creating nested parent directories if needed.

#### Case 1.3: Multiple Source Files Compilation [IMPLEMENTED]
- **Command**: `solix compile main.slx math.slx -o combined.slxb`
- **Setup**: `math.slx` defining a helper class; `main.slx` referencing it.
- **Expected**: Exits 0; produces valid `combined.slxb`.

#### Case 1.4: Disassembly / Assembly Emission [IMPLEMENTED]
- **Command**: `solix compile main.slx -o app.slxb -a app.s`
- **Setup**: `main.slx` with valid statements.
- **Expected**: Exits 0; creates both `app.slxb` and text assembly file `app.s` with non-empty content.

#### Case 1.5: Custom Entry Point Specification [IMPLEMENTED]
- **Command**: `solix compile main.slx -e custom_start -o app.slxb`
- **Setup**: `main.slx` with a method named `custom_start`.
- **Expected**: Exits 0; bytecode correctly specifies `custom_start` as entry method.

#### Case 1.6: Advanced Logging and Sink Flags [IMPLEMENTED]
- **Command**: `solix compile main.slx --log-level DEBUG --sink-type STDOUT`
- **Setup**: `main.slx` with valid statements.
- **Expected**: Exits 0; stdout captures compiler log statements.

### Negative Test Scenarios

#### Case 1.7: Missing Source File Argument [IMPLEMENTED]
- **Command**: `solix compile`
- **Expected**: Exits non-zero; error message indicating missing required `files` argument.

#### Case 1.8: Non-Existent Source File [IMPLEMENTED]
- **Command**: `solix compile missing_file.slx`
- **Expected**: Exits non-zero; error message indicating file does not exist.

#### Case 1.9: Source Code Syntax Error [IMPLEMENTED]
- **Command**: `solix compile broken.slx`
- **Setup**: `broken.slx` containing invalid syntax (`class { broken`).
- **Expected**: Exits non-zero; compilation error reported; output file not left behind.

#### Case 1.10: Source Code Semantic Error [IMPLEMENTED]
- **Command**: `solix compile semantic_err.slx`
- **Setup**: `semantic_err.slx` referencing undeclared identifier or type mismatch.
- **Expected**: Exits non-zero; compiler diagnostic logged.

#### Case 1.11: Invalid Log Level Option [IMPLEMENTED]
- **Command**: `solix compile main.slx --log-level INVALID_LEVEL`
- **Expected**: Exits non-zero; CLI validator reports error.

---

## RunCommand

*Command*: `solix run <file> [options] [args...]`

### Positive Test Scenarios

#### Case 2.1: Execute Valid Compiled Bytecode [IMPLEMENTED]
- **Command**: `solix run app.slxb`
- **Setup**: Bytecode file returning exit code 0.
- **Expected**: Exits 0; VM executes cleanly.

#### Case 2.2: Pass Program Arguments to Executing Bytecode [IMPLEMENTED]
- **Command**: `solix run app.slxb -- arg1 arg2 123`
- **Setup**: Bytecode file reading arguments.
- **Expected**: Exits 0; program receives arguments correctly.

#### Case 2.3: Custom Stack and Heap Capacities [IMPLEMENTED]
- **Command**: `solix run app.slxb -s 2048 -p 4096`
- **Setup**: Valid bytecode.
- **Expected**: Exits 0; VM initializes with customized resource limits.

#### Case 2.9: Run Uninstalled Project from Directory (Default Profile) [IMPLEMENTED]
- **Command**: `solix run ./my_project`
- **Setup**: Directory with `solix.json` defining `debug` profile and valid source files.
- **Expected**: Exits 0; automatically builds `debug` profile and executes binary on VM.

#### Case 2.10: Run Uninstalled Project with Specific Profile [IMPLEMENTED]
- **Command**: `solix run ./my_project -P release`
- **Setup**: Project with distinct `release` profile configuration.
- **Expected**: Exits 0; compiles and executes release binary.

#### Case 2.11: Run Uninstalled Project with Manifest Runtime Configuration [IMPLEMENTED]
- **Command**: `solix run ./my_project`
- **Setup**: `solix.json` profile with `runtime.heap_size`, `runtime.stack_size`, and default `runtime.arguments`.
- **Expected**: Exits 0; VM receives configured stack/heap limits and program receives default arguments.

#### Case 2.12: Run Uninstalled Project with CLI Arguments Overriding Runtime Config [IMPLEMENTED]
- **Command**: `solix run ./my_project custom_arg1 custom_arg2`
- **Setup**: `solix.json` specifying default arguments.
- **Expected**: Exits 0; CLI arguments override default manifest arguments.

#### Case 2.13: Run Installed Project by Name and Version [IMPLEMENTED]
- **Command**: `solix run -n installed_pkg -v 1.0.0`
- **Setup**: Project installed in `$SOLIX_HOME`.
- **Expected**: Exits 0; resolves installed directory from registry, builds if needed, and executes.

#### Case 2.14: Run Installed Project by Positional Specifier [IMPLEMENTED]
- **Command**: `solix run installed_pkg@1.0.0`
- **Setup**: Project installed in `$SOLIX_HOME`.
- **Expected**: Exits 0; parses `name@version`, resolves project, and executes.

#### Case 2.15: Run Installed Project with Specific Profile [IMPLEMENTED]
- **Command**: `solix run installed_pkg@1.0.0 -P release`
- **Setup**: Installed project with `release` profile.
- **Expected**: Exits 0; executes requested profile for installed package.

### Negative Test Scenarios

#### Case 2.4: Missing Bytecode File Argument [IMPLEMENTED]
- **Command**: `solix run`
- **Expected**: Exits non-zero; error message indicating missing `file`.

#### Case 2.5: Non-Existent Bytecode File [IMPLEMENTED]
- **Command**: `solix run non_existent.slxb`
- **Expected**: Exits non-zero; error message indicating file does not exist.

#### Case 2.6: Empty Bytecode File [IMPLEMENTED]
- **Command**: `solix run empty.slxb`
- **Setup**: Empty (0-byte) file.
- **Expected**: Exits non-zero; error message indicating bytecode file is empty.

#### Case 2.7: Runtime Fault / Unhandled Exception [IMPLEMENTED]
- **Command**: `solix run app_fault.slxb`
- **Setup**: Bytecode triggering an unhandled exception at runtime.
- **Expected**: Exits non-zero; "Runtime error" reported.

#### Case 2.8: Non-Zero Exit Code Propagation [IMPLEMENTED]
- **Command**: `solix run app_exit7.slxb`
- **Setup**: Bytecode returning 7 from `main`.
- **Expected**: Exits with exit code 7.

#### Case 2.16: Run Project with Non-Existent Directory Path [IMPLEMENTED]
- **Command**: `solix run ./non_existent_folder`
- **Expected**: Exits non-zero; error message indicating path does not exist.

#### Case 2.17: Run Project in Directory Lacking solix.json [IMPLEMENTED]
- **Command**: `solix run ./empty_folder`
- **Setup**: Existing directory without `solix.json`.
- **Expected**: Exits non-zero; error message indicating manifest does not exist.

#### Case 2.18: Run Project Requesting Non-Existent Profile [IMPLEMENTED]
- **Command**: `solix run ./my_project -P missing_profile`
- **Setup**: Project with only `debug` profile.
- **Expected**: Exits non-zero; error message indicating profile is not defined.

#### Case 2.19: Run Installed Project Not Found in Registry [IMPLEMENTED]
- **Command**: `solix run -n missing_pkg -v 1.0.0`
- **Expected**: Exits non-zero; error message indicating installed project was not found.

#### Case 2.20: Run Installed Project with --package but Missing --version [IMPLEMENTED]
- **Command**: `solix run -n my_pkg`
- **Expected**: Exits non-zero; error message indicating `--version is required`.

#### Case 2.21: Run Project with Compilation Error [IMPLEMENTED]
- **Command**: `solix run ./broken_project`
- **Setup**: Project containing syntax/type error in source code.
- **Expected**: Exits non-zero; compilation error reported before VM execution.

#### Case 2.22: Run Project with Native Auto-Discovery from lib/ Directory [IMPLEMENTED]
- **Command**: `solix run ./project_with_lib`
- **Setup**: Project directory containing native shared library (`.so`/`.dll`/`.dylib`) inside `./lib/`.
- **Expected**: Exits 0; automatically discovers and loads library from `./lib/`; native methods execute successfully.

#### Case 2.23: Run Bytecode Binary with Co-located Native Library Auto-Discovery [IMPLEMENTED]
- **Command**: `solix run path/to/app.slxbin`
- **Setup**: Standalone bytecode binary with a shared library in the same directory.
- **Expected**: Exits 0; automatically discovers and loads co-located shared library; executes native methods.

#### Case 2.24: Run Project with Explicit native_libraries in Manifest [IMPLEMENTED]
- **Command**: `solix run ./project_manifest_native`
- **Setup**: Project where `solix.json` defines `"native_libraries": ["path/to/plugin.so"]`.
- **Expected**: Exits 0; manifest paths are loaded and bound before execution; native methods succeed.

#### Case 2.25: Run Bytecode with Explicit CLI --native-lib / -L Flag [IMPLEMENTED]
- **Command**: `solix run app.slxbin -L path/to/plugin.so`
- **Setup**: Standalone bytecode execution with shared library provided via CLI flag.
- **Expected**: Exits 0; library specified on CLI loaded into VM; native methods succeed.

#### Case 2.26: Negative: Run Binary Requiring Native Library Without Providing It [IMPLEMENTED]
- **Command**: `solix run app_requiring_native.slxbin`
- **Setup**: Bytecode file invoking a native function without co-located library or CLI flag.
- **Expected**: Exits non-zero (exit code 1); reports missing native function diagnostic.

---

## BuildCommand

*Command*: `solix build [target] [-p <profile>]`

### Positive Test Scenarios

#### Case 3.1: Build Default (Debug) Profile from Working Directory [IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: Project with valid `solix.json` containing `debug` profile and source files.
- **Expected**: Exits 0; compiles target sources to specified output path.

#### Case 3.2: Build Specific (Release) Profile [IMPLEMENTED]
- **Command**: `solix build -p release`
- **Setup**: `solix.json` with `release` profile specifying different output and optimization settings.
- **Expected**: Exits 0; outputs release artifact.

#### Case 3.3: Custom Manifest Path via Positional Argument [IMPLEMENTED]
- **Command**: `solix build path/to/project/solix.json`
- **Setup**: Manifest in custom subdirectory.
- **Expected**: Exits 0; resolves project paths relative to manifest's parent directory.

#### Case 3.4: Multi-File Project with Dependency Resolution [IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: Project with entry file and auxiliary source files linked via manifest.
- **Expected**: Exits 0; all source files compiled into output artifact.

#### Case 3.10: Direct Project Dependency via Relative Path [IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: Project `app` depends on project `lib` using `"type": "project"` and relative `"path": "../lib"`.
- **Expected**: Exits 0; resolves `lib`, aggregates `lib` source files, and produces binary.

#### Case 3.11: Transitive Project Dependency Chain [IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: Project `app` depends on `libA`, which depends on `libB`.
- **Expected**: Exits 0; recursively resolves all transitive projects and sources; compiles all into final output.

#### Case 3.12: Circular / Mutual Project Dependencies [IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: Project `libA` depends on `libB`, and `libB` depends on `libA`.
- **Expected**: Exits 0; circularity is gracefully de-duplicated without infinite loop or failure; builds cleanly.

#### Case 3.13: SemVer Patch Difference Silent Resolution [IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: Transitive tree includes `math_lib@1.0.1` and `math_lib@1.0.4`.
- **Expected**: Exits 0; silently picks highest patch version (`1.0.4`) and compiles without warning.

#### Case 3.14: SemVer Minor Difference Warning and Resolution [IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: Transitive tree includes `util_lib@1.1.0` and `util_lib@1.3.0`.
- **Expected**: Exits 0; outputs warning about minor version divergence, selects highest minor (`1.3.0`), and succeeds.

#### Case 3.15: Project Dependency Resolved from Installed Registry [IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: Project references installed package `helper_lib` by name and SemVer requirement without explicit local `path`.
- **Expected**: Exits 0; discovers installed package in local repository (`$SOLIX_HOME`) and links sources.

#### Case 3.19: Build Project by Directory Path Positional Argument [IMPLEMENTED]
- **Command**: `solix build path/to/project_dir`
- **Setup**: Directory containing `solix.json` passed as positional argument.
- **Expected**: Exits 0; automatically infers `solix.json` within directory and builds successfully.

#### Case 3.20: Build Installed Package by name@version Positional Argument [IMPLEMENTED]
- **Command**: `solix build pkg_name@1.0.0`
- **Setup**: Package installed in `$SOLIX_HOME`.
- **Expected**: Exits 0; locates installed package manifest in registry and compiles to its output directory.

#### Case 3.21: Build Project with Combined name@version in Manifest 'name' Field [IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: Consumer project manifest specifies `{"type": "project", "name": "helper_pkg@1.2.0"}` without separate version.
- **Expected**: Exits 0; successfully splits name and version, locates package, and compiles.

#### Case 3.22: Build Project with Combined name@version in Manifest 'package' Field [IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: Consumer project manifest specifies `{"type": "project", "package": "helper_pkg@1.2.0"}`.
- **Expected**: Exits 0; successfully resolves dependency and compiles.

### Negative Test Scenarios

#### Case 3.5: Missing Manifest File [IMPLEMENTED]
- **Command**: `solix build non_existent_solix.json`
- **Expected**: Exits non-zero; error message "Manifest file does not exist".

#### Case 3.6: Malformed Manifest JSON [IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: `solix.json` containing syntax error (unclosed brace).
- **Expected**: Exits non-zero; error message "Failed to parse solix.json".

#### Case 3.7: Manifest Missing Profiles Section [IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: `solix.json` with only `name` and `version` fields.
- **Expected**: Exits non-zero; error message indicating 'profiles' section is missing or invalid.

#### Case 3.8: Requested Profile Does Not Exist [IMPLEMENTED]
- **Command**: `solix build -p non_existent_profile`
- **Setup**: Valid `solix.json` containing only `debug` profile.
- **Expected**: Exits non-zero; error message indicating profile was not found.

#### Case 3.9: Missing Source File Specified in Profile [IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: `solix.json` pointing to non-existent source file.
- **Expected**: Exits non-zero; error reported indicating file does not exist.

#### Case 3.16: Incompatible Major SemVer Collision [IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: Dependency tree requires both `core_lib@1.0.0` and `core_lib@2.0.0`.
- **Expected**: Exits non-zero; error message indicating incompatible major versions before compilation begins.

#### Case 3.17: Missing Project Dependency Manifest Path [IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: Project dependency specifies `"path": "../missing_lib"`, which does not exist.
- **Expected**: Exits non-zero; error message indicating project dependency manifest was not found.

#### Case 3.18: Missing Source File in Dependent Project [IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: Dependent project manifest references a non-existent source file.
- **Expected**: Exits non-zero; pre-compilation validation error for missing dependency source.

#### Case 3.23: Build Non-Existent Installed Package name@version [IMPLEMENTED]
- **Command**: `solix build non_existent_pkg@1.0.0`
- **Setup**: Package not present in `$SOLIX_HOME` registry.
- **Expected**: Exits non-zero; error message "Error: Installed project 'non_existent_pkg' with version '1.0.0' not found".

#### Case 3.24: Build with Legacy -m Flag Reports CLI Error [IMPLEMENTED]
- **Command**: `solix build -m path/to/solix.json`
- **Expected**: Exits non-zero; CLI validator reports error for unrecognized option `-m`.

---

# Part II: Project Scaffolding Commands

## NewCommand

*Command*: `solix new <path> [options]`

### Positive Test Scenarios

#### Case 4.1: Default Project Scaffolding [IMPLEMENTED]
- **Command**: `solix new my_project`
- **Expected**: Exits 0; generates `my_project/solix.json` and `my_project/src/main.slx`.

#### Case 4.2: Scaffolding with Custom Metadata Options [IMPLEMENTED]
- **Command**: `solix new my_lib -n custom_lib -v 1.2.3 -a "Alice <alice@example.com>" -d "A math lib" -l Apache-2.0 -t math -t fast -e start`
- **Expected**: Exits 0; `solix.json` contains specified name, version, author, description, license, tags, and entry point.

#### Case 4.3: Scaffolding into an Existing Empty Directory [IMPLEMENTED]
- **Command**: `solix new empty_dir`
- **Setup**: `empty_dir` pre-created and empty.
- **Expected**: Exits 0; files populated inside `empty_dir`.

#### Case 4.4: Overwriting Non-Empty Directory with Force Flag [IMPLEMENTED]
- **Command**: `solix new existing_dir --force`
- **Setup**: `existing_dir` contains existing files.
- **Expected**: Exits 0; overwrites project files without failing.

#### Case 4.5: Scaffolding Using Custom Template Folder [IMPLEMENTED]
- **Command**: `solix new templated_proj --template /path/to/custom_template`
- **Setup**: Custom template folder containing template files with placeholders like `{{PROJECT_NAME}}`.
- **Expected**: Exits 0; files copied from template folder and placeholders substituted.

### Negative Test Scenarios

#### Case 4.6: Missing Target Path Argument [IMPLEMENTED]
- **Command**: `solix new`
- **Expected**: Exits non-zero; CLI error indicating `path` is required.

#### Case 4.7: Target Directory Already Exists and Non-Empty Without Force [IMPLEMENTED]
- **Command**: `solix new occupied_dir`
- **Setup**: `occupied_dir` contains `some_file.txt`.
- **Expected**: Exits non-zero; error message stating directory is not empty and recommending `--force`.

#### Case 4.8: Target Path Exists as a Regular File [IMPLEMENTED]
- **Command**: `solix new file_as_dir`
- **Setup**: `file_as_dir` is a regular file.
- **Expected**: Exits non-zero; error message stating path exists and is not a directory.

#### Case 4.9: Custom Template Path Does Not Exist [IMPLEMENTED]
- **Command**: `solix new my_proj --template /non_existent_template_dir`
- **Expected**: Exits non-zero; error message stating template directory does not exist.

---

# Part III: Local Package Management Commands

## InstallCommand

*Command*: `solix install [path] [-f|--force]`

### Positive Test Scenarios

#### Case 5.1: Install Valid Project into Local Registry [IMPLEMENTED]
- **Command**: `solix install my_proj`
- **Setup**: Valid project directory with `solix.json`.
- **Expected**: Exits 0; project copied into `$SOLIX_HOME/installed/<hash_id>/`; entry recorded in `$SOLIX_HOME/installed.json`.

#### Case 5.2: Deterministic SHA-256 Hash ID Generation [IMPLEMENTED]
- **Setup**: Project with name `"my_pkg"` and version `"1.0.0"`.
- **Expected**: ID matches first 16 characters of SHA-256(`my_pkg@1.0.0`); folder in `installed/` is named exactly this hash.

#### Case 5.3: Overwrite Existing Installation with Force Flag [IMPLEMENTED]
- **Command**: `solix install my_proj --force`
- **Setup**: Project already installed.
- **Expected**: Exits 0; updates existing installation and timestamp in registry.

#### Case 5.7: Install Solix Standard Library Project (solixlib) [IMPLEMENTED]
- **Command**: `solix install solixlib/project`
- **Setup**: `solixlib/project` directory containing valid `solix.json` (`solixlib` v0.1.0) and compiled native shared library `solixlib_native` in `lib/`.
- **Expected**: Exits 0; installs into `$SOLIX_HOME/installed/<hash_id>/`; lists project as `solixlib` version 0.1.0 in registry.

#### Case 5.8: Verify Installed Standard Library Contains Native lib Directory [IMPLEMENTED]
- **Setup**: `solixlib` installed via `solix install solixlib/project`.
- **Expected**: The target folder `$SOLIX_HOME/installed/<hash_id>/lib/` exists and contains `solixlib_native` shared library.

#### Case 5.9: Consumer Project Builds and Runs with Installed Standard Library (solixlib) [IMPLEMENTED]
- **Command**: `solix run my_consumer_proj`
- **Setup**: Consumer project with dependency `[{"type": "project", "name": "solixlib", "version": "0.1.0"}]` in `solix.json`.
- **Expected**: Exits 0; resolves `solixlib` from `$SOLIX_HOME`, builds and runs successfully.

### Negative Test Scenarios

#### Case 5.4: Target Directory Does Not Exist [IMPLEMENTED]
- **Command**: `solix install /non_existent_dir`
- **Expected**: Exits non-zero; error message indicating directory does not exist.

#### Case 5.5: Target Directory Missing solix.json [IMPLEMENTED]
- **Command**: `solix install invalid_dir`
- **Setup**: Directory exists but contains no `solix.json`.
- **Expected**: Exits non-zero; error message indicating missing manifest.

#### Case 5.6: Reinstall Existing Project Without Force Flag [IMPLEMENTED]
- **Command**: `solix install my_proj`
- **Setup**: Project already installed.
- **Expected**: Exits non-zero; error message stating project is already installed and recommending `--force`.

#### Case 5.10: Reinstall solixlib Without Force Flag Fails [IMPLEMENTED]
- **Command**: `solix install solixlib/project`
- **Setup**: `solixlib` already installed in `$SOLIX_HOME`.
- **Expected**: Exits non-zero (exit code 1); error message indicating `solixlib` version 0.1.0 is already installed and recommending `--force`.

---

## UninstallCommand

*Command*: `solix uninstall <name> <version>`

### Positive Test Scenarios

#### Case 6.1: Uninstall Successfully Removes Project and Directory [IMPLEMENTED]
- **Command**: `solix uninstall my_pkg 1.0.0`
- **Setup**: `my_pkg` 1.0.0 installed in registry.
- **Expected**: Exits 0; directory under `$SOLIX_HOME/installed/<hash_id>` is deleted; entry removed from `installed.json`.

### Negative Test Scenarios

#### Case 6.2: Missing Required Name or Version Arguments [IMPLEMENTED]
- **Command**: `solix uninstall my_pkg`
- **Expected**: Exits non-zero; CLI error indicating missing argument.

#### Case 6.3: Package Not Found in Registry [IMPLEMENTED]
- **Command**: `solix uninstall unknown_pkg 9.9.9`
- **Expected**: Exits non-zero; error message stating project is not installed.

---

## ListCommand

*Command*: `solix list`

### Positive Test Scenarios

#### Case 7.1: Empty Installation Registry [IMPLEMENTED]
- **Command**: `solix list`
- **Setup**: Clean `$SOLIX_HOME` with no installed projects.
- **Expected**: Exits 0; outputs "No Solix projects are currently installed."

#### Case 7.2: List Multiple Installed Projects [IMPLEMENTED]
- **Command**: `solix list`
- **Setup**: Multiple projects installed in registry.
- **Expected**: Exits 0; output contains table with headers `NAME`, `VERSION`, `ID`, `INSTALLED PATH` and includes every installed project.

---

## DetailsCommand

*Command*: `solix details <name> [version]`

### Positive Test Scenarios

#### Case 8.1: Extensive Details by Name and Version [IMPLEMENTED]
- **Command**: `solix details my_pkg 1.0.0`
- **Setup**: `my_pkg` 1.0.0 installed with full metadata (description, author, license, profiles, dependencies).
- **Expected**: Exits 0; displays name, version, hash ID, install date, installed path, disk size, author, description, license, profiles, and dependencies.

#### Case 8.2: Details by Name When Single Version Installed [IMPLEMENTED]
- **Command**: `solix details my_pkg`
- **Setup**: Only one version of `my_pkg` is installed.
- **Expected**: Exits 0; automatically resolves the unique version and prints detailed metadata.

### Negative Test Scenarios

#### Case 8.3: Project Not Found by Name [IMPLEMENTED]
- **Command**: `solix details nonexistent_pkg`
- **Expected**: Exits non-zero; error message stating no installed project found.

#### Case 8.4: Specific Version Not Found [IMPLEMENTED]
- **Command**: `solix details my_pkg 9.9.9`
- **Setup**: `my_pkg` 1.0.0 installed, but 9.9.9 requested.
- **Expected**: Exits non-zero; error message stating version not found.

#### Case 8.5: Multiple Versions Installed and Version Omitted [IMPLEMENTED]
- **Command**: `solix details my_multi_pkg`
- **Setup**: `my_multi_pkg` has versions 1.0.0 and 2.0.0 installed.
- **Expected**: Prompts user with available versions and their IDs without crashing.

---

# Part IV: Toolchain Version Commands

## VersionCommand

*Command*: `solix version` or `solix --version` / `solix -v`

### Positive Test Scenarios

#### Case 9.1: Version Subcommand [IMPLEMENTED]
- **Command**: `solix version`
- **Setup**: None.
- **Expected**: Exits 0; outputs Solix version string (e.g. `Solix version 0.1.0`).

#### Case 9.2: Version Top-Level Flag (`--version` / `-v`) [IMPLEMENTED]
- **Command**: `solix --version` and `solix -v`
- **Setup**: None.
- **Expected**: Exits 0; outputs Solix version string.

---

# Part V: Language Server Command

## LspCommand

*Command*: `solix lsp`

### Positive Test Scenarios

#### Case 10.1: LSP Lifecycle Handshake [IMPLEMENTED]
- **Command**: `solix lsp`
- **Input**: JSON-RPC `initialize` request followed by `shutdown` and `exit`.
- **Expected**: Exits 0; responds with `capabilities` (textDocumentSync, hoverProvider, definitionProvider, etc.); returns null on shutdown; terminates cleanly on exit.

#### Case 10.2: LSP Document Sync & Diagnostics Publication [IMPLEMENTED]
- **Command**: `solix lsp`
- **Input**: JSON-RPC `textDocument/didOpen` with a valid Solix source file.
- **Expected**: Emits `textDocument/publishDiagnostics` with empty diagnostic array `[]`.

#### Case 10.3: LSP Live Syntax Error Diagnostics [IMPLEMENTED]
- **Command**: `solix lsp`
- **Input**: JSON-RPC `textDocument/didOpen` with invalid syntax (e.g. `int32 a = ;`).
- **Expected**: Emits `textDocument/publishDiagnostics` containing error diagnostic with severity 1, code `E_PARSE`, and accurate line/column range.

#### Case 10.4: LSP Live Diagnostic Resolution on Edit [IMPLEMENTED]
- **Command**: `solix lsp`
- **Input**: JSON-RPC `textDocument/didOpen` with error, followed by `textDocument/didChange` providing the fixed syntax.
- **Expected**: Emits updated `textDocument/publishDiagnostics` clearing the error diagnostic.

#### Case 10.5: LSP Project Manifest Integration (`solix.json`) [IMPLEMENTED]
- **Command**: `solix lsp`
- **Setup**: Workspace directory containing `solix.json` declaring dependencies.
- **Input**: `initialize` with `rootUri` set to workspace; `didOpen` referencing symbols from imported packages.
- **Expected**: Symbols from manifest dependencies resolve successfully without false-positive undeclared symbol diagnostics.

---

# Part VI: Language Server Navigation & Inspection Commands

## LspNavigationCommand

*Command*: `solix lsp` (JSON-RPC requests `textDocument/definition`, `textDocument/typeDefinition`, `textDocument/hover`)

### Positive Test Scenarios

#### Case 11.1: Go-to-Definition for Local Variables [IMPLEMENTED]
- **Request**: `textDocument/definition` at position of a local variable reference.
- **Expected**: Returns `Location` pointing to line/column of variable declaration.

#### Case 11.2: Go-to-Definition for Class Declarations & Methods [IMPLEMENTED]
- **Request**: `textDocument/definition` at method call or class instantiation.
- **Expected**: Returns `Location` pointing to target method or class declaration.

#### Case 11.3: Go-to-Type-Definition for Instance Expressions [IMPLEMENTED]
- **Request**: `textDocument/typeDefinition` at variable usage.
- **Expected**: Returns `Location` pointing to declaring class of variable's resolved type.

#### Case 11.4: Hover Tooltip for Variables, Methods, and Primitives [IMPLEMENTED]
- **Request**: `textDocument/hover` at variable, method, or primitive type reference.
- **Expected**: Returns `Hover` object with markdown formatted signature, doc comments, or type information.

### Negative Test Scenarios

#### Case 11.5: Definition and Hover on Whitespace / Unresolved Tokens [IMPLEMENTED]
- **Request**: `textDocument/definition` and `textDocument/hover` at empty space or comment.
---

# Part VII: Language Server Intelligence Commands

## LspIntelligenceCommand

*Command*: `solix lsp` (JSON-RPC requests `textDocument/completion`, `textDocument/signatureHelp`, `textDocument/documentSymbol`)

### Positive Test Scenarios

#### Case 12.1: Member Completion on Dot Access [NOT IMPLEMENTED]
- **Request**: `textDocument/completion` after typing `.` on an instance expression (e.g. `h.`).
- **Expected**: Returns `CompletionList` containing accessible fields and methods of the receiver's type with correct `CompletionItemKind`.

#### Case 12.2: Scope & Keyword Completion [NOT IMPLEMENTED]
- **Request**: `textDocument/completion` within a function or method body.
- **Expected**: Returns `CompletionList` containing visible local variables, parameters, enclosing class members, and language keywords.

#### Case 12.3: Signature Help on Method Call [NOT IMPLEMENTED]
- **Request**: `textDocument/signatureHelp` inside argument list parentheses (e.g. `compute(`).
- **Expected**: Returns `SignatureHelp` with `SignatureInformation` detailing parameter names and active parameter index.

#### Case 12.4: Hierarchical Document Symbols Outline [NOT IMPLEMENTED]
- **Request**: `textDocument/documentSymbol` on a source file.
- **Expected**: Returns array of `DocumentSymbol` representing class declarations and nested methods/fields with their respective `SymbolKind` and ranges.

### Negative Test Scenarios

#### Case 12.5: Completion on Unresolved Expression [NOT IMPLEMENTED]
- **Request**: `textDocument/completion` after dot access on unknown/invalid identifier.
- **Expected**: Returns empty `CompletionList` (`items: []`) gracefully without crashing.
