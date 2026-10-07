# Solix Language Server Protocol (LSP) Specification

The Solix Language Server (`solix-lsp`, or invoked via `solix lsp`) implements the standard Language Server Protocol (LSP 3.17) over standard I/O (`stdin`/`stdout`). It provides editor-agnostic language intelligence, live diagnostic squiggles, and native project manifest integration (`solix.json`).

---

## 1. Transport & Framing

Communication adheres to the JSON-RPC 2.0 protocol using HTTP-style header framing:

```
Content-Length: <byte_length>\r\n
\r\n
<json_payload>
```

### Lifecycle Messages

1. **`initialize` (Request)**:
   - Parameters:
     - `rootUri` (string): Absolute `file://` URI to the project workspace root.
     - `workspaceFolders` (array): List of open workspace folders.
     - `capabilities` (object): Client capability flags.
   - Response:
     - Server capabilities:
       ```json
       {
         "capabilities": {
           "textDocumentSync": 1,
           "hoverProvider": false,
           "definitionProvider": false,
           "completionProvider": {
             "resolveProvider": false,
             "triggerCharacters": [".", "::"]
           }
         },
         "serverInfo": {
           "name": "solix-lsp",
           "version": "0.1.0"
         }
       }
       ```

2. **`initialized` (Notification)**:
   - Acknowledges handshake readiness.

3. **`shutdown` (Request)**:
   - Signals server shutdown. Returns `null` result.

4. **`exit` (Notification)**:
   - Exits the server process with exit code `0` if shutdown was received, or `1` if unexpected exit.

---

## 2. Document Synchronization

The Solix Language Server maintains an in-memory document cache (`DocumentStore`) to provide live feedback without requiring users to save files to disk.

- **`textDocument/didOpen`**:
  - Registers open file buffers and immediately triggers frontend analysis.
- **`textDocument/didChange`**:
  - Updates buffer contents in memory and re-evaluates syntax and semantic rules.
- **`textDocument/didClose`**:
  - Unregisters file buffer and emits an empty `textDocument/publishDiagnostics` list to clear editor squiggles.
- **`textDocument/didSave`**:
  - Synchronizes disk changes and triggers re-analysis.

---

## 3. Project Manifest Integration (`solix.json`)

When `initialize` provides a `rootUri`, the server automatically discovers `solix.json` in the root workspace folder:

1. **Dependency Resolution**:
   - Uses `solix::cli::DependencyManager` and `DependencyResolver` to resolve:
     - Standard Library (`solixlib`) from `$SOLIX_HOME/installed/solixlib` registry.
     - Local source dependencies (`type: "source"`).
     - Local and installed project dependencies (`type: "project"`).
2. **Buffer Overrides**:
   - Any open virtual document in the editor takes precedence over on-disk files.
3. **Multi-File Context**:
   - Symbols across all referenced packages and modules are bound together, preventing false-positive undeclared symbol warnings.

---

## 4. Live Diagnostics (`textDocument/publishDiagnostics`)

On every document modification, the server runs the compiler frontend pipeline:
$$\text{Lexer} \longrightarrow \text{Parser} \longrightarrow \text{Binder}$$

Errors and warnings captured by `solix::Diagnostic` are transformed into LSP diagnostic objects:

```json
{
  "method": "textDocument/publishDiagnostics",
  "params": {
    "uri": "file:///path/to/File.slx",
    "diagnostics": [
      {
        "severity": 1,
        "code": "E_PARSE",
        "source": "solix",
        "message": "Expected ';' after expression",
        "range": {
          "start": { "line": 4, "character": 12 },
          "end": { "line": 4, "character": 13 }
        }
      }
    ]
  }
}
```

- **Coordinates**: Converted from 1-indexed compiler source positions to LSP 0-indexed positions.
- **Spans**: Utilizes accurate start and end token coordinates (`end_line`, `end_column`).
