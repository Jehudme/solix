# Solix for Visual Studio Code

Official Visual Studio Code extension for the **Solix** programming language.

## Features

- **Syntax Highlighting**: Rich TextMate grammar for Solix keywords, types, identifiers, strings, and operators.
- **Language Server Protocol (LSP)**:
  - Live Diagnostics & Squiggles (`textDocument/publishDiagnostics`)
  - Go-to-Definition (`textDocument/definition`)
  - Go-to-Type-Definition (`textDocument/typeDefinition`)
  - Hover Documentation (`textDocument/hover`)
  - Autocompletion for Keywords & Member Access (`textDocument/completion`)
  - Parameter Hints & Signature Help (`textDocument/signatureHelp`)
  - Hierarchical Outline & Breadcrumbs (`textDocument/documentSymbol`)
- **Project Manifest Integration**: Automatically resolves project dependencies declared in `solix.json`.

## Requirements

The extension communicates with the Solix compiler toolchain over stdio via the `solix lsp` subcommand.
Ensure `solix` is accessible in your system `PATH`, or configure its absolute location in VS Code Settings:
`"solix.lsp.path": "/usr/local/bin/solix"`

## Installation

```bash
code --install-extension solix-0.1.0.vsix
```
