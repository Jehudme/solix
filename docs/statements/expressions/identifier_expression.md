# IdentifierNode (`NodeType::IDENTIFIER`)

## 1. Description & Purpose

An `identifier` node represents a symbolic name reference within source code, addressing a local variable, function parameter, class field, global variable, function, or type name. The symbol binder resolves the identifier against the active scope hierarchy, determining its storage category (stack index, global slot, or field offset) and type.

## 2. Syntax & Grammar

```solix
<identifier>
```

## 3. Underlying Systems & Mechanics

- Resolved via `resolve_symbol`:
  1. Active local scopes (variables, parameters).
  2. Current class fields / methods (`this`).
  3. Imported symbols table.
  4. Active package prefix.
  5. Global scope exact match.
  6. Sub-namespace suffix search across all registered symbols.
- Emits ambiguity error if multiple packages define the same name without explicit import.

## 4. Positive Test Scenarios (Valid Variations)

1. **Local Variable Identifier**: `x`
2. **Unqualified Unique Class Name**: `String s = ...;`
3. **Explicitly Imported Name**: `import core.String; String s = ...;`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Undefined Identifier**:
   - `x = 10;` (without `int32 x;`)  
     *Error*: `Undefined identifier: x`
2. **Ambiguous Identifier Collision**:
   - `Item.code();` where both `pkg_a.Item` and `pkg_b.Item` exist  
     *Error*: `Ambiguous symbol 'Item': multiple candidates found (pkg_a.Item, pkg_b.Item). Specify full package or use import to disambiguate.`
