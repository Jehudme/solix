# Solix Language Specification: Statements & Constructs

This directory contains the definitive, formal language specification for all 37 statement and expression constructs in Solix, structured in accordance with the industry-standard specification model (modeled after the Java Language Specification, ECMA-334 C# Specification, and The Rust Reference).

---

## Specification Standard Structure
Every construct in this directory is authored with rigorous adherence to the following 8 sections:

1. **§1. Overview & Scope**: Architectural definition, conceptual role, and legal syntactic placement.
2. **§2. Syntax & Production Rules**: Formal grammar production rules and canonical code patterns.
3. **§3. Scope & Declaration Space (Static Semantics)**: Identifier visibility, downward reach, upward boundary confinement, masking/shadowing, and static extent.
4. **§4. Operational Semantics (Dynamic Execution)**:
   - **4.1 Normal Completion**: Step-by-step numbered execution sequence ($1, 2, 3 \dots$).
   - **4.2 Abrupt Completion**: Non-local control transfers (`return`, `break`, `continue`, `throw`).
   - **4.3 Exception Unwinding & Trampolines**: Dynamic exception propagation and trampoline routing.
5. **§5. Memory Model & ARC Invariants**: Reference counting semantics (`INC_REF`, `DEC_REF`, `RELEASE`), strict LIFO deallocation order, stack/register layout, and bytecode lowering.
6. **§6. Compile-Time Constraints & Diagnostic Errors**: Formal rules that render code ill-formed, complete with code examples and exact compiler diagnostic strings.
7. **§7. Runtime Fault Conditions**: Dynamic runtime traps, panics, and VM exceptions.
8. **§8. Conformance & Verification Examples**: Concrete test listings with verifiable invariant guarantees.

---

## Table of Specifications

### Part I: Compilation Unit & Module Statements (`modules/`)

| § | Construct | AST NodeType | Specification Document |
|---|---|---|---|
| §1 | `PackageStatement` | `NodeType::PACKAGE_STMT` | [package_statement.md](modules/package_statement.md) |
| §2 | `ImportStatement` | `NodeType::IMPORT_STMT` | [import_statement.md](modules/import_statement.md) |
| §3 | `AliasStatement` | `NodeType::ALIAS_STMT` | [alias_statement.md](modules/alias_statement.md) |

### Part II: Type & Structural Declarations (`declarations/`)

| § | Construct | AST NodeType | Specification Document |
|---|---|---|---|
| §4 | `ClassDeclaration` | `NodeType::CLASS_DECL` | [class_declaration.md](declarations/class_declaration.md) |
| §5 | `InterfaceDeclaration` | `NodeType::INTERFACE` | [interface_declaration.md](declarations/interface_declaration.md) |
| §6 | `EnumDeclaration` | `NodeType::ENUM_DECL` | [enum_declaration.md](declarations/enum_declaration.md) |
| §7 | `FieldDeclaration` | `NodeType::FIELD_DECL` | [field_declaration.md](declarations/field_declaration.md) |
| §8 | `ConstructorDeclaration` | `NodeType::CONSTRUCTOR_DECL` | [constructor_declaration.md](declarations/constructor_declaration.md) |
| §9 | `MethodDeclaration` | `NodeType::METHOD_DECL` | [method_declaration.md](declarations/method_declaration.md) |
| §10 | `OperatorDeclaration` | `NodeType::OPERATOR` | [operator_declaration.md](declarations/operator_declaration.md) |

### Part III: Local Scope & Execution Statements (`control_flow/`)

| § | Construct | AST NodeType | Specification Document |
|---|---|---|---|
| §11 | `BlockStatement` | `NodeType::BLOCK` | [block_statement.md](control_flow/block_statement.md) |
| §12 | `VariableDeclarationStatement` | `NodeType::VAR_DECL` | [variable_declaration.md](control_flow/variable_declaration.md) |
| §13 | `ExpressionStatement` | `NodeType::EXPR_STMT` | [expression_statement.md](control_flow/expression_statement.md) |
| §14 | `IfStatement` | `NodeType::IF_STMT` | [if_statement.md](control_flow/if_statement.md) |
| §15 | `WhileStatement` | `NodeType::WHILE_STMT` | [while_statement.md](control_flow/while_statement.md) |
| §16 | `DoWhileStatement` | `NodeType::DO_WHILE_STMT` | [do_while_statement.md](control_flow/do_while_statement.md) |
| §17 | `ForStatement` | `NodeType::FOR_STMT` | [for_statement.md](control_flow/for_statement.md) |
| §18 | `SwitchStatement` | `NodeType::SWITCH_STMT` | [switch_statement.md](control_flow/switch_statement.md) |
| §19 | `BreakStatement` | `NodeType::BREAK_STMT` | [break_statement.md](control_flow/break_statement.md) |
| §20 | `ContinueStatement` | `NodeType::CONTINUE_STMT` | [continue_statement.md](control_flow/continue_statement.md) |
| §21 | `ReturnStatement` | `NodeType::RETURN_STMT` | [return_statement.md](control_flow/return_statement.md) |
| §22 | `ThrowStatement` | `NodeType::THROW_STMT` | [throw_statement.md](control_flow/throw_statement.md) |
| §23 | `TryCatchFinallyStatement` | `NodeType::TRY_STMT`, `CATCH_CLAUSE` | [try_catch_finally_statement.md](control_flow/try_catch_finally_statement.md) |

### Part IV: Expressions & Operators (`expressions/`)

| § | Construct | AST NodeType | Specification Document |
|---|---|---|---|
| §24 | `AssignmentExpression` | `NodeType::ASSIGNMENT_EXPR` | [assignment_expression.md](expressions/assignment_expression.md) |
| §25 | `TernaryExpression` | `NodeType::TERNARY_EXPR` | [ternary_expression.md](expressions/ternary_expression.md) |
| §26 | `BinaryExpression` | `NodeType::BINARY_EXPR` | [binary_expression.md](expressions/binary_expression.md) |
| §27 | `UnaryExpression` | `NodeType::UNARY_EXPR` | [unary_expression.md](expressions/unary_expression.md) |
| §28 | `CastExpression` | `NodeType::CAST_EXPR` | [cast_expression.md](expressions/cast_expression.md) |
| §29 | `InstanceOfExpression` | `NodeType::INSTANCEOF_EXPR` | [instanceof_expression.md](expressions/instanceof_expression.md) |
| §30 | `NewInstanceExpression` | `NodeType::NEW_INSTANCE` | [new_instance_expression.md](expressions/new_instance_expression.md) |
| §31 | `ArrayCreationExpression` | `NodeType::ARRAY_CREATION` | [array_creation_expression.md](expressions/array_creation_expression.md) |
| §32 | `ArrayAccessExpression` | `NodeType::ARRAY_ACCESS` | [array_access_expression.md](expressions/array_access_expression.md) |
| §33 | `ArrayLiteralExpression` | `NodeType::ARRAY_LITERAL` | [array_literal_expression.md](expressions/array_literal_expression.md) |
| §34 | `MemberAccessExpression` | `NodeType::MEMBER_ACCESS` | [member_access_expression.md](expressions/member_access_expression.md) |
| §35 | `MethodCallExpression` | `NodeType::METHOD_CALL` | [method_call_expression.md](expressions/method_call_expression.md) |
| §36 | `IdentifierNode` | `NodeType::IDENTIFIER` | [identifier_expression.md](expressions/identifier_expression.md) |
| §37 | `LiteralNode` | `NodeType::LITERAL` | [literal_expression.md](expressions/literal_expression.md) |
