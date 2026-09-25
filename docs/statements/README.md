# Solix Language Reference: Statements & Constructs

This directory contains the practical, code-first language reference for all 37 statements, declarations, and expressions in Solix.

Each document focuses strictly on the architecture and mechanics of the construct:
1. **Overview & Purpose**: Direct, plain-English explanation of what the construct does, why it exists, and its core language rules.
2. **Compilation & Runtime Mechanics (With Real Bytecode)**: How the Solix compiler lowers the code, showing side-by-side Solix source code and compiled VM bytecode, along with stack and ARC refcount operations.

> [!NOTE]
> All positive test scenarios (valid variations) and negative test scenarios (expected compiler errors and runtime exceptions) are centralized in the master [TEST_SPECIFICATION.md](../TEST_SPECIFICATION.md).

---

## Directory Index

### Part I: Compilation Unit & Modules (`modules/`)
- [`package_statement.md`](modules/package_statement.md) — Namespace boundaries & compilation unit isolation
- [`import_statement.md`](modules/import_statement.md) — Selective & wildcard symbol imports
- [`alias_statement.md`](modules/alias_statement.md) — Type synonyms & generic alias substitutions

### Part II: Declarations (`declarations/`)
- [`class_declaration.md`](declarations/class_declaration.md) — Reference types, single inheritance, and VTables
- [`interface_declaration.md`](declarations/interface_declaration.md) — Abstract contracts & multiple interface conformance
- [`enum_declaration.md`](declarations/enum_declaration.md) — Strongly typed discrete enumerations
- [`field_declaration.md`](declarations/field_declaration.md) — Static, instance, and `weak` cycle-breaking fields
- [`constructor_declaration.md`](declarations/constructor_declaration.md) — Initializer lists, `super()` chaining, and heap allocation
- [`method_declaration.md`](declarations/method_declaration.md) — Static, virtual, and abstract member functions
- [`operator_declaration.md`](declarations/operator_declaration.md) — Operator overloading for `+`, `-`, `*`, `/`, `=`

### Part III: Control Flow & Execution Statements (`control_flow/`)
- [`block_statement.md`](control_flow/block_statement.md) — Lexical scope boundaries & ARC scope-exit cleanup
- [`variable_declaration.md`](control_flow/variable_declaration.md) — Frame slotting, primitives vs. reference types
- [`expression_statement.md`](control_flow/expression_statement.md) — Side-effect evaluation & temporary reference `DEC_REF` / `POP`
- [`if_statement.md`](control_flow/if_statement.md) — Boolean predicate branching & short-circuit jumps
- [`while_statement.md`](control_flow/while_statement.md) — Pre-test loop iteration & loop context labels
- [`do_while_statement.md`](control_flow/do_while_statement.md) — Post-test single-pass guarantee
- [`for_statement.md`](control_flow/for_statement.md) — Induction variable scoping, step expressions, and iteration
- [`switch_statement.md`](control_flow/switch_statement.md) — Multi-way jump tables, case matching, and fall-through
- [`break_statement.md`](control_flow/break_statement.md) — Non-local escape & intermediate block ARC unwinding
- [`continue_statement.md`](control_flow/continue_statement.md) — Iteration advancement & intermediate block cleanup
- [`return_statement.md`](control_flow/return_statement.md) — Frame termination, return values, receiver/parameter cleanup
- [`throw_statement.md`](control_flow/throw_statement.md) — `std.Exception` hierarchy enforcement & cleanup trampolining
- [`try_catch_finally_statement.md`](control_flow/try_catch_finally_statement.md) — Exception matching tables, parameter binding, `finally` guarantees

### Part IV: Expressions & Operators (`expressions/`)
- [`assignment_expression.md`](expressions/assignment_expression.md) — Lvalue mutations & ARC ownership transfers
- [`ternary_expression.md`](expressions/ternary_expression.md) — Inline lazy conditional expressions
- [`binary_expression.md`](expressions/binary_expression.md) — Arithmetic, relational, logical, and bitwise evaluation
- [`unary_expression.md`](expressions/unary_expression.md) — Prefix and postfix scalar operations
- [`cast_expression.md`](expressions/cast_expression.md) — Conversions and dynamic `CAST_CHECK`
- [`instanceof_expression.md`](expressions/instanceof_expression.md) — VTable runtime hierarchy querying
- [`new_instance_expression.md`](expressions/new_instance_expression.md) — Heap allocation and constructor dispatch
- [`array_creation_expression.md`](expressions/array_creation_expression.md) — Contiguous array buffer allocation
- [`array_access_expression.md`](expressions/array_access_expression.md) — Subscript indexing & runtime bounds checks
- [`array_literal_expression.md`](expressions/array_literal_expression.md) — Inline array literals (`[...]` and `{...}`)
- [`member_access_expression.md`](expressions/member_access_expression.md) — Property resolution & null checking
- [`method_call_expression.md`](expressions/method_call_expression.md) — Static & dynamic virtual VTable invocations
- [`identifier_expression.md`](expressions/identifier_expression.md) — Scope resolution (stack slot, property, global)
- [`literal_expression.md`](expressions/literal_expression.md) — Primitive immediates, strings, chars, and null
