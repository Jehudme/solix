# Compiler Diagnostics & Error Reporting Specification

## 1. Overview & Purpose

The Solix compiler enforces uniform and deterministic error reporting across all compilation phases:
1. **Lexical Analysis (`Lexer`)**: Tokenization errors, unterminated literals, and unrecognized characters.
2. **Syntax Analysis (`Parser`)**: Grammar productions, missing tokens, and malformed statements.
3. **Semantic Analysis (`Binder`)**: Type checking, scope resolution, duplicate symbols, and inheritance contracts.
4. **Code Generation (`Assembler`)**: Frame layout, vtable calculation, and entry point resolution.

---

## 2. Invariant Diagnostic Format

Every diagnostic message produced by the compiler follows the canonical format:

```text
[<file_path>:<line>:<column>] <Severity>: <Message>
```

When rendered to terminal / stdout via `Diagnostic::print_reports`, the diagnostic displays the source line and caret indicator (`^`) pointing to the offending character:

```text
/path/to/source.slx:4:17: error: [E_PARSE]: Expected expression, got ';'
    int32 x = ;
              ^
```

### Report Attributes

Each diagnostic report records the following properties:
- `source_path`: The file path or source identifier (never empty; defaults to `<unknown>` only when no source context exists).
- `line`: 1-indexed row number where the error was encountered.
- `column`: 1-indexed column number pointing to the beginning of the offending token or character.
- `code`: Diagnostic code (`E_LEX`, `E_PARSE`, `E_BIND`, `E_ASM`).
- `severity`: Severity level (`ERROR`, `WARNING`, `NOTE`).
- `message`: Clear, human-readable description of the error.
