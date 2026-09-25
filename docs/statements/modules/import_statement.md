# ImportStatement (`NodeType::IMPORT_STMT`)

## 1. Description & Purpose

The `import` statement brings external symbols or entire package namespaces into the current lexical scope. Solix supports both wildcards (`import package.*;`) for bulk symbol importation and selective imports (`import package.Class;`) to prevent namespace pollution. Imports are resolved at compile time during the symbol resolution passes (Pass 1a and Pass 1b), allowing unqualified references to imported types and functions while retaining strict disambiguation rules if naming collisions occur.

## 2. Syntax & Grammar

```solix
import <package-path> ('.' '*' | ('.' | '::') <symbol>) ';'
```

## 3. Underlying Systems & Mechanics

- Collected in Pass 1a into `pending_imports` and bound after all packages and symbols are cataloged (`process_imports`).
- Individual symbol imports register `imported_symbols[symbol_name] = full_mangled_name`.
- Wildcard imports register `known_packages.insert(target_pkg)`.
- Symbol lookups check `imported_symbols` before falling back to cross-package suffix searches, enabling explicit conflict resolution.

## 4. Positive Test Scenarios (Valid Variations)

1. **Full Symbol Import**: `import solix.core.String;`
2. **Sub-Namespace Symbol Import**: `import core.String;`
3. **Wildcard Package Import**: `import solix.collections.*;` or `import collections.*;`
4. **C++ Scope Resolution Import**: `import solix::core::Objects;`
5. **Multiple Imports in Single File**: Importing dozens of modules sequentially.

## 5. Negative Test Scenarios (Invalid Variations)

1. **Importing Non-Existent Symbol**:
   - `import solix.core.NonExistentClass;`  
     *Error*: `Cannot resolve imported symbol: solix.core.NonExistentClass`
2. **Importing Non-Existent Wildcard Package**:
   - `import fake.unknown.pkg.*;`  
     *Error*: `Cannot resolve imported package: fake.unknown.pkg`
3. **Wildcard Not at Tail**:
   - `import solix.*.String;`  
     *Error*: `Expected identifier or '*' after '.'`
4. **Wildcard Ambiguity Conflict**:
   - `import pkg_a.*; import pkg_b.*;` where both define `Widget`, accessed as unqualified `Widget w = ...;`  
     *Error*: `Ambiguous symbol 'Widget': multiple candidates found (pkg_a.Widget, pkg_b.Widget). Specify full package or use import to disambiguate.`
