# AliasStatement (`NodeType::ALIAS_STMT`)

## 1. Description & Purpose

The `alias` statement introduces a type synonym or parameterized generic type alias into the symbol table. Aliases enhance code clarity and reusability by creating concise aliases for complex types, generic specializations (e.g., `alias StringList = List<String>;`), or primitive aliases. Solix aliases are transparently substituted during the semantic analysis pass (Pass 2: Type Checking and Resolution), incurring zero runtime performance overhead.

## 2. Syntax & Grammar

```solix
alias <identifier> ['<' <T...> '>'] '=' <type-info> ';'
```

## 3. Underlying Systems & Mechanics

- Registered in Pass 1a in `global_scope.symbols` as an `AliasStatement` node.
- Supports generic parameterization: if template parameters `<T...>` are present, stored as a parameterized alias blueprint.
- In Pass 2, `target_type` is resolved and `resolved_declaration` is linked directly to the underlying `ClassDeclaration`, `EnumDeclaration`, or template blueprint.
- Re-export alias deduplication: unifies multiple aliases referencing the same underlying type so that re-exports (such as `solix.NullPointerException` aliasing `solix.core.NullPointerException`) never cause false-positive collision errors.
- Monomorphizes lazily upon instantiation if the alias targets a generic template.

## 4. Positive Test Scenarios (Valid Variations)

1. **Primitive Array Shorthand**: `alias Matrix = float64[][];`
2. **Full Symbol Namespace Alias**: `alias Str = solix.core.String;`
3. **Partial Symbol Namespace Alias**: `alias Str = core.String;`
4. **Template Blueprint Alias**: `alias List = solix.collections.List;`
5. **Templated Generic Alias**: `alias IntMap<V> = Map<int32, V>;`
6. **Re-Export Backward Compatibility Alias**:
   - In `package solix; alias String = solix.core.String;`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Aliasing Undefined Type**:
   - `alias Bad = nonexistent.Type;`  
     *Error*: `Unknown type: nonexistent.Type`
2. **Duplicate Alias Name in Scope**:
   - `alias Item = int32; alias Item = float64;`  
     *Error*: `Duplicate global symbol: Item`
3. **Cyclic Alias Reference**:
   - `alias A = B; alias B = A;`  
     *Error*: `Cyclic type alias definition detected: A -> B -> A`
4. **Missing Semicolon**:
   - `alias ID = uint64`  
     *Error*: `Expected ';' after alias declaration`
