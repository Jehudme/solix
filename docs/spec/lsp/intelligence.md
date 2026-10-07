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
2. Resolves the receiver's type using the spatial AST index and symbol table.
3. Enumerates members of the declaring class:
   - Methods: mapped to `CompletionItemKind::Method` (`2`) with parameter signatures and `()` snippet insert text.
   - Fields: mapped to `CompletionItemKind::Field` (`5`) with field type details.
4. If the receiver is invalid or unknown, returns an empty item list without error.

### General Scope Completion
When requested outside member dot access:
1. **Language Keywords**: Offers keywords (`class`, `interface`, `public`, `return`, `if`, `while`, `var`, `int32`, `string`, etc.) with `CompletionItemKind::Keyword` (`14`).
2. **Local Variables & Parameters**: Suggests visible parameters and local variables in the enclosing method/block scope.
3. **Types & Classes**: Suggests known classes and interfaces declared across the project with `CompletionItemKind::Class` (`7`).

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
- **Top-level Functions**: `SymbolKind::Function` (`12`).
