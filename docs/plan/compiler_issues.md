# Solix Compiler & Analyzer Issues Plan

This document lists all the bugs, mistakes, and omissions in the current codebase, structured by compilation phase. It also outlines the strict error-handling requirements going forward.

## 1. General Error Handling Rules (System-wide)
- **No silent failures:** The compiler and semantic analyzer must **never** skip an unknown or unhandled AST node. If a node is missing an implementation, the compiler must throw a hard error.
- **Detailed Error Messages:** All exceptions in the Semantic Analyzer and Compiler must include the exact location of the statement. The format must be:
  `[Phase Error] <file_path>:<line>:<column> - <Error Message>`
  *(Currently, `semantic.cpp` and `compiler.cpp` throw raw `std::runtime_error("string")` without location data.)*

## 2. Lexer (`lexer.cpp`)
- **String Escaping:** The lexer currently extracts string literals verbatim, keeping the surrounding quotes and raw backslashes (`\n` instead of actual newline). The `handle_string()` method must strip the quotes and correctly process escape sequences.

## 3. Parser (`parser.cpp`)
- The parser's `throw_parse_error` correctly handles file paths and line numbers, but we must ensure every node tracks its parent correctly. (This was partially fixed recently with `parent_node`).
- **Array Literal Parsing:** Heuristics for differentiating `BLOCK_STATEMENT` and `ARRAY_LITERAL_EXPRESSION` are currently weak.

## 4. Semantic Analyzer (`semantic.cpp`)
- **Missing Break/Continue Validation:** `BREAK_STATEMENT` and `CONTINUE_STATEMENT` are entirely unhandled in `resolveAndCheck`. The analyzer must enforce that these statements only occur when `loop_depth > 0`.
- **Enum Member Values:** The analyzer does not assign integer values to enum members. This means `CoreStatus.OVERHEATING` has no backing integer value to emit. The analyzer must map enum members to `0, 1, 2...` and store these constants.
- **Unhandled Nodes:** If `resolveAndCheck` hits an unhandled node, it must throw a detailed error instead of silently returning.
- **Location Context:** Update all `throw std::runtime_error(...)` calls to use a new helper function that formats the error with `node->file_path`, `node->line`, and `node->column`.

## 5. Compiler (`compiler.cpp` & `compiler.hpp`)
- **Missing OpCodes:** We need to add `GET_PROPERTY`, `SET_PROPERTY` (or `GET_FIELD`/`SET_FIELD`), and `SET_ARRAY` to the `OpCode` enum in `compiler.hpp`. (We currently only have `GET_GLOBAL` and `GET_ARRAY`).
- **Missing AST Statement Handlers (`compileNode`):**
  1. `SWITCH_STATEMENT` & `CASE_STATEMENT`: Must emit chained `EQUAL` + `JUMP_IF_FALSE` logic.
  2. `BREAK_STATEMENT`: Must emit `JUMP` to the end of the current loop block.
  3. `CONTINUE_STATEMENT`: Must emit `JUMP` to the start of the current loop block.
  *Note: The compiler needs a stack (`std::vector<uint32_t> loop_start_ips` and a way to track break patches) to implement break/continue.*
- **Missing AST Expression Handlers (`compileExpression`):**
  1. `MEMBER_ACCESS_EXPRESSION`: Must evaluate the object, then emit `GET_PROPERTY <field_offset>`.
  2. `ARRAY_ACCESS_EXPRESSION`: Must emit `GET_ARRAY`.
  3. `ARRAY_CREATION_EXPRESSION`: Must emit `ALLOC_DYNAMIC`.
  4. `TERNARY_EXPRESSION`: Must emit `JUMP_IF_FALSE` branching logic.
  5. `ARRAY_LITERAL_EXPRESSION`: Must allocate an array and sequentially emit `SET_ARRAY` for each element.
- **Assignment Target Resolution (`ASSIGNMENT_EXPRESSION`):**
  Currently, assignments only work for local variables (`SET_LOCAL`). The logic must be expanded to support:
  - `MEMBER_ACCESS_EXPRESSION` targets -> Emits `SET_PROPERTY`
  - `ARRAY_ACCESS_EXPRESSION` targets -> Emits `SET_ARRAY`
  - Global variable targets -> Emits `SET_GLOBAL`
