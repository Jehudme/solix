# Solix LSP Intelligence Specification

The Solix Language Server provides intelligent code-authoring capabilities conforming to LSP 3.17:
- `textDocument/completion` (Context-aware Autocompletion)
- `textDocument/signatureHelp` (Parameter & Signature Hints)
- `textDocument/documentSymbol` (Hierarchical Document Outline)

---

## 1. Context-Aware Autocompletion (`textDocument/completion`)

The language server handles completions based on cursor context and trigger characters (`.`):

### Member Dot Completion (`receiver.`)
When the user types `.` after an expression:
1. The language server determines the receiver expression to the left of `.`.
2. Resolves the receiver's type using the spatial AST index and symbol table:
   - `this.`: Resolves to the enclosing class declaration, offering all class fields and methods.
   - Field access (e.g. `this._map.` or `_map.`): Inspects enclosing class fields, extracting declared type information and stripping generic arguments to resolve the underlying class (e.g. `HashMap<K, V>` -> `HashMap`).
3. Enumerates members of the declaring class:
   - Methods: mapped to `CompletionItemKind::Method` (`2`) with parameter signatures and `()` snippet insert text.
   - Fields: mapped to `CompletionItemKind::Field` (`5`) with field type details.
4. **Resilient AST Preservation**: During live typing, transient syntax errors on unfinished statements do not clear cached class declarations, ensuring completions remain responsive and available on every keystroke.
5. If the receiver is invalid or unknown, returns an empty item list without error.

### General Scope Completion
When requested outside member dot access:
1. **Language Keywords**: Offers keywords (`class`, `interface`, `public`, `return`, `if`, `while`, `var`, `int32`, `string`, etc.) with `CompletionItemKind::Keyword` (`14`).
2. **Local Variables & Parameters**: Suggests visible parameters and local variables in the enclosing method/block scope.
3. **Types & Classes**: Suggests known classes, interfaces, enums, and declared type aliases (`alias`) across the project with `CompletionItemKind::Class` (`7`), `CompletionItemKind::Enum` (`13`), or `CompletionItemKind::Reference` (`18`).

### Context-Aware Grammar Completions
The language server inspects the lexical tokens leading up to the cursor to refine completions:
1. **Class Declaration (`class <cursor>`)**:
   - Existing class names are suppressed from completion results so typing a new class name does not autocomplete or collide with existing declarations.
2. **Inheritance Extension (`extends <cursor>`)**:
   - Suggests only concrete/abstract classes with `CompletionItemKind::Class`.
   - Interfaces are strictly excluded.
3. **Interface Implementation (`implements <cursor>`, `implements IFoo, <cursor>`)**:
   - Suggests only interfaces with `CompletionItemKind::Interface`.
   - Regular classes are strictly excluded.
4. **Instantiations (`new <cursor>`)**:
   - Suggests instantiable classes (excluding interfaces and `abstract` classes).
   - Injects constructor snippets with `()` automatically via `insertText`.
5. **Switch Cases (`case <cursor>`)**:
   - Inspects the enclosing `switch` condition variable type.
   - If switching over an `enum`, suggests enum members (e.g. `Color.RED`, `Color.GREEN`, `Color.BLUE`) with `CompletionItemKind::EnumMember`.
6. **Package Declarations (`package <cursor>`)**:
   - Offers package name completions inferred from file directory path relative to project root or `src/` (e.g. `src/net/http/Client.slx` -> `net.http`).
   - Suggests known package names declared across all compilation units in the workspace with `CompletionItemKind::Module`.
   - Package suggestions are suppressed in general scope or method body contexts.
7. **Type Alias Target (`alias <Name> = <cursor>`)**:
   - When declaring a type alias target after `=`, suggests primitive types (`int8`, `int16`, `int32`, `int64`, `uint8`, `uint16`, `uint32`, `uint64`, `float32`, `float64`, `bool`, `char`, `string`, `void`, `any`), classes, interfaces, enums, and previously declared type aliases.
   - While typing the alias name before `=` (`alias <cursor>`), existing type suggestions are suppressed.
8. **Import Statements (`import <cursor>`, `import pkg.<cursor>`)**:
   - At statement root (`import <cursor>`): suggests all known workspace packages, subpackage prefixes, and standard library modules (`solix.core`, `solix.system`, `solix.collections`, `solix.io`, `solix.math`, `solix.time`, `solix.exceptions`, `solix.crypto`).
   - After package delimiter (`import pkg.<cursor>`): suggests immediate subpackages, classes, interfaces, enums, type aliases belonging to the package, and wildcard `*`.

---

## 2. Parameter Hints (`textDocument/signatureHelp`)

Triggered on `(` and `,` inside function and method invocation argument lists:
1. Scans backward from cursor to the matching unclosed `(` while tracking parenthesis nesting depth.
2. Identifies the callee name and resolves its declaration in the project context.
3. Counts unnested commas `,` up to the cursor position to determine the zero-based `activeParameter`.
4. Returns `SignatureHelp` with:
   - Full signature string label (e.g. `public void compute(int32 delta, string tag)`).
   - Parameter information array describing each parameter.
   - `activeSignature` and `activeParameter` indices.

---

## 3. Hierarchical Document Symbols (`textDocument/documentSymbol`)

Provides editors and IDEs (e.g. VS Code Outline view, symbol breadcrumbs) with a full hierarchical representation of the file:
- **Classes & Interfaces**: `SymbolKind::Class` (`5`) or `SymbolKind::Interface` (`11`) with enclosing ranges.
- **Nested Members**:
  - Methods: `SymbolKind::Method` (`6`) with return type details.
  - Fields: `SymbolKind::Field` (`8`) with field type details.
  - Constructors: `SymbolKind::Constructor` (`9`).
- **Enums**: `SymbolKind::Enum` (`10`) containing enum member children with `SymbolKind::EnumMember` (`22`).
- **Type Aliases**: `SymbolKind::TypeParameter` (`26`) representing `alias` declarations.
- **Top-level Functions**: `SymbolKind::Function` (`12`).
