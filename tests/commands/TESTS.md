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

---

## BuildCommand

*Command*: `solix build [-p <profile>] [-m <manifest>]`

### Positive Test Scenarios

#### Case 3.1: Build Default (Debug) Profile from Working Directory [NOT IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: Project with valid `solix.json` containing `debug` profile and source files.
- **Expected**: Exits 0; compiles target sources to specified output path.

#### Case 3.2: Build Specific (Release) Profile [NOT IMPLEMENTED]
- **Command**: `solix build -p release`
- **Setup**: `solix.json` with `release` profile specifying different output and optimization settings.
- **Expected**: Exits 0; outputs release artifact.

#### Case 3.3: Custom Manifest Path via Flag [NOT IMPLEMENTED]
- **Command**: `solix build -m path/to/project/solix.json`
- **Setup**: Manifest in custom subdirectory.
- **Expected**: Exits 0; resolves project paths relative to manifest's parent directory.

#### Case 3.4: Multi-File Project with Dependency Resolution [NOT IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: Project with entry file and auxiliary source files linked via manifest.
- **Expected**: Exits 0; all source files compiled into output artifact.

### Negative Test Scenarios

#### Case 3.5: Missing Manifest File [NOT IMPLEMENTED]
- **Command**: `solix build -m non_existent_solix.json`
- **Expected**: Exits non-zero; error message "Manifest file does not exist".

#### Case 3.6: Malformed Manifest JSON [NOT IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: `solix.json` containing syntax error (unclosed brace).
- **Expected**: Exits non-zero; error message "Failed to parse solix.json".

#### Case 3.7: Manifest Missing Profiles Section [NOT IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: `solix.json` with only `name` and `version` fields.
- **Expected**: Exits non-zero; error message indicating 'profiles' section is missing or invalid.

#### Case 3.8: Requested Profile Does Not Exist [NOT IMPLEMENTED]
- **Command**: `solix build -p non_existent_profile`
- **Setup**: Valid `solix.json` containing only `debug` profile.
- **Expected**: Exits non-zero; error message indicating profile was not found.

#### Case 3.9: Missing Source File Specified in Profile [NOT IMPLEMENTED]
- **Command**: `solix build`
- **Setup**: `solix.json` pointing to non-existent source file.
- **Expected**: Exits non-zero; error reported indicating file does not exist.

---

# Part II: Project Scaffolding Commands

## NewCommand

*Command*: `solix new <path> [options]`

### Positive Test Scenarios

#### Case 4.1: Default Project Scaffolding [NOT IMPLEMENTED]
- **Command**: `solix new my_project`
- **Expected**: Exits 0; generates `my_project/solix.json`, `my_project/src/main.slx`, `.gitignore`, `README.md`.

#### Case 4.2: Scaffolding with Custom Metadata Options [NOT IMPLEMENTED]
- **Command**: `solix new my_lib -n custom_lib -v 1.2.3 -a "Alice <alice@example.com>" -d "A math lib" -l Apache-2.0 -t math -t fast -e start`
- **Expected**: Exits 0; `solix.json` contains specified name, version, author, description, license, tags, and entry point.

#### Case 4.3: Scaffolding into an Existing Empty Directory [NOT IMPLEMENTED]
- **Command**: `solix new empty_dir`
- **Setup**: `empty_dir` pre-created and empty.
- **Expected**: Exits 0; files populated inside `empty_dir`.

#### Case 4.4: Overwriting Non-Empty Directory with Force Flag [NOT IMPLEMENTED]
- **Command**: `solix new existing_dir --force`
- **Setup**: `existing_dir` contains existing files.
- **Expected**: Exits 0; overwrites project files without failing.

#### Case 4.5: Scaffolding Using Custom Template Folder [NOT IMPLEMENTED]
- **Command**: `solix new templated_proj --template /path/to/custom_template`
- **Setup**: Custom template folder containing template files with placeholders like `{{PROJECT_NAME}}`.
- **Expected**: Exits 0; files copied from template folder and placeholders substituted.

### Negative Test Scenarios

#### Case 4.6: Missing Target Path Argument [NOT IMPLEMENTED]
- **Command**: `solix new`
- **Expected**: Exits non-zero; CLI error indicating `path` is required.

#### Case 4.7: Target Directory Already Exists and Non-Empty Without Force [NOT IMPLEMENTED]
- **Command**: `solix new occupied_dir`
- **Setup**: `occupied_dir` contains `some_file.txt`.
- **Expected**: Exits non-zero; error message stating directory is not empty and recommending `--force`.

#### Case 4.8: Target Path Exists as a Regular File [NOT IMPLEMENTED]
- **Command**: `solix new file_as_dir`
- **Setup**: `file_as_dir` is a regular file.
- **Expected**: Exits non-zero; error message stating path exists and is not a directory.

#### Case 4.9: Custom Template Path Does Not Exist [NOT IMPLEMENTED]
- **Command**: `solix new my_proj --template /non_existent_template_dir`
- **Expected**: Exits non-zero; error message stating template directory does not exist.

---

# Part III: Local Package Management Commands

## InstallCommand

*Command*: `solix install [path] [-f|--force]`

### Positive Test Scenarios

#### Case 5.1: Install Valid Project into Local Registry [NOT IMPLEMENTED]
- **Command**: `solix install my_proj`
- **Setup**: Valid project directory with `solix.json`.
- **Expected**: Exits 0; project copied into `$SOLIX_HOME/installed/<hash_id>/`; entry recorded in `$SOLIX_HOME/installed.json`.

#### Case 5.2: Deterministic SHA-256 Hash ID Generation [NOT IMPLEMENTED]
- **Setup**: Project with name `"my_pkg"` and version `"1.0.0"`.
- **Expected**: ID matches first 16 characters of SHA-256(`my_pkg@1.0.0`); folder in `installed/` is named exactly this hash.

#### Case 5.3: Overwrite Existing Installation with Force Flag [NOT IMPLEMENTED]
- **Command**: `solix install my_proj --force`
- **Setup**: Project already installed.
- **Expected**: Exits 0; updates existing installation and timestamp in registry.

### Negative Test Scenarios

#### Case 5.4: Target Directory Does Not Exist [NOT IMPLEMENTED]
- **Command**: `solix install /non_existent_dir`
- **Expected**: Exits non-zero; error message indicating directory does not exist.

#### Case 5.5: Target Directory Missing solix.json [NOT IMPLEMENTED]
- **Command**: `solix install invalid_dir`
- **Setup**: Directory exists but contains no `solix.json`.
- **Expected**: Exits non-zero; error message indicating missing manifest.

#### Case 5.6: Reinstall Existing Project Without Force Flag [NOT IMPLEMENTED]
- **Command**: `solix install my_proj`
- **Setup**: Project already installed.
- **Expected**: Exits non-zero; error message stating project is already installed and recommending `--force`.

---

## UninstallCommand

*Command*: `solix uninstall <name> <version>`

### Positive Test Scenarios

#### Case 6.1: Uninstall Successfully Removes Project and Directory [NOT IMPLEMENTED]
- **Command**: `solix uninstall my_pkg 1.0.0`
- **Setup**: `my_pkg` 1.0.0 installed in registry.
- **Expected**: Exits 0; directory under `$SOLIX_HOME/installed/<hash_id>` is deleted; entry removed from `installed.json`.

### Negative Test Scenarios

#### Case 6.2: Missing Required Name or Version Arguments [NOT IMPLEMENTED]
- **Command**: `solix uninstall my_pkg`
- **Expected**: Exits non-zero; CLI error indicating missing argument.

#### Case 6.3: Package Not Found in Registry [NOT IMPLEMENTED]
- **Command**: `solix uninstall unknown_pkg 9.9.9`
- **Expected**: Exits non-zero; error message stating project is not installed.

---

## ListCommand

*Command*: `solix list`

### Positive Test Scenarios

#### Case 7.1: Empty Installation Registry [NOT IMPLEMENTED]
- **Command**: `solix list`
- **Setup**: Clean `$SOLIX_HOME` with no installed projects.
- **Expected**: Exits 0; outputs "No Solix projects are currently installed."

#### Case 7.2: List Multiple Installed Projects [NOT IMPLEMENTED]
- **Command**: `solix list`
- **Setup**: Multiple projects installed in registry.
- **Expected**: Exits 0; output contains table with headers `NAME`, `VERSION`, `ID`, `INSTALLED PATH` and includes every installed project.

---

## DetailsCommand

*Command*: `solix details <name> [version]`

### Positive Test Scenarios

#### Case 8.1: Extensive Details by Name and Version [NOT IMPLEMENTED]
- **Command**: `solix details my_pkg 1.0.0`
- **Setup**: `my_pkg` 1.0.0 installed with full metadata (description, author, license, profiles, dependencies).
- **Expected**: Exits 0; displays name, version, hash ID, install date, installed path, disk size, author, description, license, profiles, and dependencies.

#### Case 8.2: Details by Name When Single Version Installed [NOT IMPLEMENTED]
- **Command**: `solix details my_pkg`
- **Setup**: Only one version of `my_pkg` is installed.
- **Expected**: Exits 0; automatically resolves the unique version and prints detailed metadata.

### Negative Test Scenarios

#### Case 8.3: Project Not Found by Name [NOT IMPLEMENTED]
- **Command**: `solix details nonexistent_pkg`
- **Expected**: Exits non-zero; error message stating no installed project found.

#### Case 8.4: Specific Version Not Found [NOT IMPLEMENTED]
- **Command**: `solix details my_pkg 9.9.9`
- **Setup**: `my_pkg` 1.0.0 installed, but 9.9.9 requested.
- **Expected**: Exits non-zero; error message stating version not found.

#### Case 8.5: Multiple Versions Installed and Version Omitted [NOT IMPLEMENTED]
- **Command**: `solix details my_multi_pkg`
- **Setup**: `my_multi_pkg` has versions 1.0.0 and 2.0.0 installed.
- **Expected**: Prompts user with available versions and their IDs without crashing.
