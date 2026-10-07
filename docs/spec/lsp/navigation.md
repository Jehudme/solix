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

### Supported Constructs & Formats
- **Methods**: Displays complete method signatures including access modifiers, qualifiers (`static`, `inline`, `native`, `virtual`, `override`, `abstract`), return type, method name, and all parameters with types, reference modifiers, and names:
  ```solix
  public virtual void doWork(int32 n, string& label)
  ```
- **Constructors & `new` Instances**: Displays constructor signature including access modifier and parameter types/names:
  ```solix
  public ConcreteJob(int32 cap, string name)
  ```
- **Fields**: Displays field access modifiers, `static`, `const`, `weak`, type, and name:
  ```solix
  private const int32 limit
  ```
- **Local Variables & Parameters**: Displays variable qualifiers (`const`, `weak`), type, reference flag, and identifier name (without spurious access modifiers):
  ```solix
  const int32 localMax
  ```
- **Classes & Interfaces**: Displays class declaration with access modifier, `abstract`/`interface`, template parameters, base class (`extends`), and implemented interfaces (`implements`):
  ```solix
  public class ConcreteJob extends BaseJob implements IWorker
  ```
- **Enums**: Displays enum declaration header:
  ```solix
  public enum Status
  ```
- **Aliases**: Displays full type alias equivalence:
  ```solix
  alias Callback = (int32) -> void
  ```
- **Package Statements**: Displays package declaration:
  ```solix
  package my.service;
  ```

### Keyword Suppression
Hover requests targeting Solix keywords (`if`, `class`, `public`, `return`, `new`, `extends`, `implements`, etc.) are explicitly suppressed and return `null` (`{"result": null}`), preventing redundant tooltip popups on syntax elements.

### Response Payload
```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "result": {
    "contents": {
      "kind": "markdown",
      "value": "```solix\npublic virtual void doWork(int32 n, string& label)\n```"
    },
    "range": {
      "start": { "line": 5, "character": 4 },
      "end": { "line": 5, "character": 56 }
    }
  }
}
```

---

## 5. Unbound Generic Blueprint Scope & Manifest Discovery

### 5.1 Upward Project Manifest Discovery
When opening or editing a file in a multi-directory workspace or nested package (such as `solixlib/project/src/solix/collections/HashSet.slx`), the Language Server searches upwards from the target file directory to locate the nearest `solix.json`. If found, all source dependencies declared in the project manifest are loaded and compiled into the analysis context, preventing false-positive undefined import errors regardless of the root workspace URI passed during LSP initialization.

### 5.2 Lexical Scope Traversal for Generic Blueprints
In the Solix compiler, generic template blueprints (classes with template parameters `<T>`) defer semantic binding until instantiation. To provide full LSP navigation and hover within uninstantiated generic classes:
1. **Callable Parameter & Local Resolution**: When cursor targets an identifier inside a generic method or constructor, the server traverses the enclosing callable's parameters and AST statement blocks to locate declarations (e.g. `contains(T item)` -> `T item`).
2. **Chained Member Resolution**: Member accesses on fields (e.g. `this._map.contains_key(item)`) resolve receiver types by inspecting enclosing class fields, mapping generic field types (e.g. `HashMap<T, bool>` -> `HashMap`), and cross-referencing member declarations in external project sources.

