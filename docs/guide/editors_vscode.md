# Solix Visual Studio Code Extension Guide

The official Solix Visual Studio Code extension provides first-class language support and IDE capabilities for the Solix programming language, powered by the Solix Language Server (`solix lsp`).

---

## 1. Features

- **TextMate Syntax Highlighting**: Comprehensive grammar coverage for keywords, control flow, modifiers, primitives, custom types, string escapes, numbers, and operators.
- **Language Configuration**: Smart bracket matching (`{}`, `()`, `[]`), quote auto-closing, and line/block comment toggling (`//`, `/* */`).
- **Live Diagnostics**: Real-time error and warning squiggles updated on buffer edits (`textDocument/publishDiagnostics`).
- **Code Navigation**:
  - **Go-to-Definition** (`F12`): Jump directly to local variables, methods, fields, and classes across the project and dependencies.
  - **Go-to-Type-Definition**: Jump to declaring class or interface of variable instances.
- **Hover Inspection**: Rich markdown tooltips displaying symbol kinds, type signatures, and doc comments.
- **Autocompletion**:
  - Contextual dot access (`receiver.`): Auto-suggests accessible methods and fields of the receiver type.
  - Scope completion: Suggests language keywords, visible local variables, and project classes.
- **Signature Help**: Inline parameter hints and active parameter tracking during function/method calls (`(` and `,`).
- **Document Outline**: Hierarchical symbol tree in the VS Code Outline view and breadcrumb navigation.
- **Project Manifest Integration**: Automatically discovers and respects `solix.json` project dependencies.

---

## 2. Installation

### Option A: Install Pre-packaged `.vsix`
To install the pre-built `.vsix` extension file directly into Visual Studio Code:

```bash
code --install-extension editors/vscode/solix-0.1.0.vsix
```

Alternatively, inside Visual Studio Code:
1. Open the Extensions view (`Ctrl+Shift+X` or `Cmd+Shift+X`).
2. Click the `...` (Views and More Actions) menu in the top-right corner.
3. Select **Install from VSIX...**.
4. Browse to `editors/vscode/solix-0.1.0.vsix` and click **Install**.

### Option B: Development Mode (Run from Source)
1. Open the `editors/vscode` folder in VS Code:
   ```bash
   code editors/vscode
   ```
2. Press `F5` to launch an Extension Development Host window.
3. Open any workspace containing `.slx` files.

---

## 3. Configuration Settings

The extension can be configured in your user or workspace `settings.json`:

```json
{
  "solix.lsp.path": "/usr/local/bin/solix",
  "solix.trace.server": "verbose"
}
```

| Setting | Type | Default | Description |
|---|---|---|---|
| `solix.lsp.path` | `string` | `""` | Custom path to the `solix` executable or `solix-lsp` server. If empty, searches the system `PATH`. |
| `solix.trace.server` | `string` | `"off"` | Traces JSON-RPC communication between VS Code and the Solix language server (`"off"`, `"messages"`, `"verbose"`). |

---

## 4. Building and Packaging from Source

To compile and package the extension from source:

```bash
cd editors/vscode

# 1. Install dependencies
npm install

# 2. Compile TypeScript
npm run compile

# 3. Package .vsix
npm run package
```

This generates `solix-0.1.0.vsix` ready for distribution.
