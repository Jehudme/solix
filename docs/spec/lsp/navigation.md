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

Finds the declaration site of symbols under the cursor:
- **Local Variables & Parameters**: Jumps to the `VariableDeclaration` node where the variable was defined.
- **Methods**: Jumps to the `MethodDeclaration` in the declaring class or interface.
- **Fields**: Jumps to the `FieldDeclaration` within the class or struct.
- **Classes, Enums, Aliases**: Jumps to the corresponding declaration header.

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
