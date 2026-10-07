# Solix LSP Navigation & Inspection Specification

The Solix Language Server implements symbol navigation and semantic inspection endpoints conforming to LSP 3.17:
- `textDocument/definition` (Go-to-Definition)
- `textDocument/typeDefinition` (Go-to-Type-Definition)
- `textDocument/hover` (Hover Inspection Tooltips)

---

## 1. Spatial AST Indexing

The Language Server maintains an in-memory spatial index (`AstSpatialIndex`) derived from the frontend compilation pipeline (`Lexer -> Parser -> Binder`).

### Query Resolution
When receiving position-based queries:
1. Coordinates are translated from LSP 0-indexed line/column to Solix 1-indexed source positions.
2. The spatial index traverses the target file's AST, matching bounding boxes `[start_line:start_col, end_line:end_col]`.
3. If multiple AST nodes overlap the target point, the visitor selects the smallest enclosing node span (e.g. an identifier token over its enclosing method call or expression statement).

---

## 2. Go-to-Definition (`textDocument/definition`)

Finds the declaration site of symbols under the cursor with multi-step chaining and type resolution:

### 2.1 Multi-Step Definition Chaining
- **Variable Usage to Declaration**: Invoking Go-to-Definition on a variable reference (e.g. `calc` in `calc.add()`) navigates to its `VariableDeclaration` coordinates.
- **Declaration to Type Chaining**: Invoking Go-to-Definition while already at the declaration site (e.g. cursor on `calc` in `Calculator calc = ...`) chains directly into the `ClassDeclaration` or `EnumDeclaration` of the variable's declared type.

### 2.2 Type Annotation Resolution
Type annotations across all language constructs directly resolve to their defining `ClassDeclaration`, `EnumDeclaration`, or `AliasStatement`:
- **Variable & Field Declarations**: `Calculator c;` or `private Helper h;` resolves to `class Calculator` / `class Helper`.
- **Constructor Calls / Allocations**: `new Calculator()` on the type identifier navigates to `class Calculator`.
- **Explicit Casts**: `(Calculator) obj` navigates to `class Calculator`.
- **Exception Handlers**: `catch (MyException e)` on `MyException` navigates to `class MyException`.

### 2.3 Inheritance & Method Overrides
- **`extends <Base>`**: Navigates from the inheritance clause identifier to the base class declaration.
- **`implements <Interface>`**: Navigates to the interface declaration.
- **`override` Methods**: When invoking definition on an `override` method declaration (e.g. `public override void run()`), the server traverses the inheritance hierarchy to locate and jump to the matching method in the base class or interface.

### 2.4 Keyword Suppression
In accordance with language ergonomics, queries on keywords (`if`, `class`, `public`, `return`, `extends`, `implements`, etc.) or punctuation return `null` without error.

### Request Payload
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "textDocument/definition",
  "params": {
    "textDocument": { "uri": "file:///path/to/source.slx" },
    "position": { "line": 10, "character": 12 }
  }
}
```

### Response Payload
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "uri": "file:///path/to/source.slx",
    "range": {
      "start": { "line": 2, "character": 4 },
      "end": { "line": 2, "character": 26 }
    }
  }
}
```
*Note: Returns `null` if the cursor is on whitespace, unresolved symbols, or language keywords.*

---

## 3. Go-to-Type-Definition (`textDocument/typeDefinition`)

Resolves the underlying type declaration of a variable, parameter, field, or instance expression.

### Behavior
- Given an expression of type `MyClass`, jumps directly to `class MyClass` rather than the variable declaration.
- For primitive types (`int32`, `bool`, etc.), returns `null` or points to the built-in primitive declaration if defined in source.

### Response Payload
```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "result": {
    "uri": "file:///path/to/source.slx",
    "range": {
      "start": { "line": 0, "character": 0 },
      "end": { "line": 0, "character": 12 }
    }
  }
}
```

---

## 4. Hover Tooltip Inspection (`textDocument/hover`)

Renders rich markdown documentation and type signature tooltips when hovering over symbols in the editor.

### Supported Constructs
- **Methods**: Formats signature showing access modifier, return type, name, and parameters (e.g., ````solix\npublic void compute()\n````).
- **Variables & Parameters**: Displays declared type and variable name (e.g., ````solix\nHelper h\n````).
- **Fields**: Displays field type and containing class info.
- **Classes & Types**: Displays class or enum declarations.

### Response Payload
```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "result": {
    "contents": {
      "kind": "markdown",
      "value": "```solix\npublic void compute()\n```"
    },
    "range": {
      "start": { "line": 2, "character": 4 },
      "end": { "line": 2, "character": 26 }
    }
  }
}
```
