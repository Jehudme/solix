# Solix Language Specification

This directory contains the formal specification of the Solix programming language. Each document covers a distinct layer of the language — from how source text is tokenized, through the type system and its static rules, to the bytecode format executed by the virtual machine.

---

## Specification Documents

### Core Language

| Document | Description |
|----------|-------------|
| [**Lexical Grammar**](lexical.md) | Character encoding, comments, whitespace, identifiers, all 39 reserved keywords, all 13 primitive type keywords, numeric/character/string literals, operators, and punctuation tokens |
| [**Type System & Static Semantics**](types.md) | Primitive types with ranges and defaults, implicit widening conversions, explicit narrowing casts, reference types, null safety, array types, type compatibility, nominal subtyping, method override rules, templates and generics, and type name resolution order |
| [**VM Instruction Set Architecture**](vm_isa.md) | Stack-based VM architecture, flat heap memory model, ARC object header format, all 90 opcodes with encodings and stack effects, call frame layout, object and array memory layout, ARC reference-counting semantics, and trampoline-based exception unwinding |

### Statements, Declarations & Expressions

| Document | Description |
|----------|-------------|
| [**Statements Reference**](statements/README.md) | Practical, code-first reference for all 37 Solix statements, declarations, and expressions — each covering overview, compilation mechanics, and real bytecode examples |

---

## Document Map

```
docs/spec/
├── README.md                   ← this file
├── lexical.md                  ← Lexical Grammar Specification
├── types.md                    ← Type System & Static Semantics
├── vm_isa.md                   ← VM Instruction Set Architecture
└── statements/
    ├── README.md               ← Statements reference index
    ├── modules/
    │   ├── package_statement.md
    │   ├── import_statement.md
    │   └── alias_statement.md
    ├── declarations/
    │   ├── class_declaration.md
    │   ├── interface_declaration.md
    │   ├── enum_declaration.md
    │   ├── field_declaration.md
    │   ├── constructor_declaration.md
    │   ├── method_declaration.md
    │   └── operator_declaration.md
    ├── control_flow/
    │   ├── block_statement.md
    │   ├── variable_declaration.md
    │   ├── expression_statement.md
    │   ├── if_statement.md
    │   ├── while_statement.md
    │   ├── do_while_statement.md
    │   ├── for_statement.md
    │   ├── switch_statement.md
    │   ├── break_statement.md
    │   ├── continue_statement.md
    │   ├── return_statement.md
    │   ├── throw_statement.md
    │   └── try_catch_finally_statement.md
    └── expressions/
        ├── assignment_expression.md
        ├── ternary_expression.md
        ├── binary_expression.md
        ├── unary_expression.md
        ├── cast_expression.md
        ├── instanceof_expression.md
        ├── new_instance_expression.md
        ├── array_creation_expression.md
        ├── array_access_expression.md
        ├── array_literal_expression.md
        ├── member_access_expression.md
        ├── method_call_expression.md
        ├── identifier_expression.md
        └── literal_expression.md
```

---

## Reading Order

For a new implementor or language learner, the recommended reading order is:

1. **[Lexical Grammar](lexical.md)** — understand how source text becomes tokens.
2. **[Type System](types.md)** — understand the static type rules the compiler enforces.
3. **[Statements Reference](statements/README.md)** — understand each language construct with examples and emitted bytecode.
4. **[VM ISA](vm_isa.md)** — understand how the bytecode is executed at runtime.
