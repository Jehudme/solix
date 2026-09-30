# `alias`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `alias` |
| Category | Declaration |
| Context | Top-level in a source file, after `package` and `import` declarations |
| Related | [`import`](import.md), [`class`](class.md) |

`alias` introduces a local shorthand name for an existing, fully qualified type. It is particularly useful for shortening long generic or deeply nested type names, resolving naming conflicts between two imports that share a simple name, or establishing domain-meaningful synonyms that make code self-documenting. An alias is purely a compile-time substitution; it does not create a new type and has no runtime footprint.

## 2. Permitted Contexts (Syntax & Grammar)

```
AliasDeclaration
    : 'alias' Identifier '=' QualifiedName ';'
    ;
```

- `alias` declarations appear at the top level of a source file, after `package` and `import`.
- The right-hand side must be an existing, resolvable fully qualified type name.
- The alias name must not conflict with any imported or locally declared type name.

## 3. Semantics & Compiler Rules

- Within the file, every use of the alias name is treated exactly as if the full qualified name had been written.
- Aliases do not participate in type identity; `alias Str = solix.core.String;` means `Str` and `solix.core.String` are the same type.
- An alias for a generic type must include type parameters if the type is parameterised.
- Circular aliases (where the right-hand side resolves back to the alias) are a compile error **E0450**.
- **W0310** is issued for an alias that is declared but never used within the file.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

alias MyString = solix.core.String;

public class Greeter {
    public static MyString greet(MyString name) {
        return "Hello, " + name + "!";
    }
}
```

### Idiomatic Usage

```solix
package solix.example.network;

import solix.collections.Map;
import solix.net.HttpHeaders;

// Long type name shortened for readability
alias HeaderMap = solix.collections.Map;

// Disambiguate two packages that both export "Logger"
alias SysLog  = solix.systems.Logger;
alias AppLog  = solix.myapp.logging.Logger;

public class RequestHandler {
    private HeaderMap defaultHeaders;
    private SysLog    sysLogger;
    private AppLog    appLogger;

    public RequestHandler() {
        this.defaultHeaders = new HeaderMap();
        this.sysLogger      = new SysLog();
        this.appLogger      = new AppLog();
    }

    public void handle(string path) {
        appLogger.info("Handling: " + path);
        sysLogger.debug("RequestHandler.handle called");
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Alias name conflicts with an imported type | **E0451** `alias 'List' conflicts with imported type 'solix.collections.List'` |
| Right-hand side type does not exist | **E0452** `cannot resolve type 'solix.bogus.Foo' in alias declaration` |
| Circular alias | **E0450** `circular alias: 'MyType' resolves back to itself` |
| Unused alias | **W0310** `alias 'HeaderMap' is declared but never used` |

## 6. Related Keywords & Guides

- [`import`](import.md) — alternative for bringing a type into scope by its simple name
- [`class`](class.md) — use `alias` when a class name is too verbose or ambiguous
