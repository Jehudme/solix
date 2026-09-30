# `import`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `import` |
| Category | Declaration |
| Context | After the `package` declaration, before type declarations |
| Related | [`package`](package.md), [`alias`](alias.md) |

`import` brings an external package member or an entire package into the current compilation unit so that it can be referenced by its simple name rather than its fully qualified name. Solix supports single-type imports (`import solix.systems.Console;`) and wildcard imports (`import solix.collections.*;`) that bring all public types from a package into scope. `import` has no runtime cost; it is purely a compile-time name resolution directive.

## 2. Permitted Contexts (Syntax & Grammar)

```
ImportDeclaration
    : 'import' QualifiedName ';'               // single-type import
    | 'import' QualifiedName '.' '*' ';'       // wildcard import
    ;
```

- `import` declarations must appear after the `package` declaration and before any type declarations.
- Multiple `import` statements are permitted in a single file.
- Wildcard imports bring all public types in the named package into scope, but not sub-packages.

## 3. Semantics & Compiler Rules

- After `import solix.systems.Console;`, the name `Console` resolves to `solix.systems.Console` within the file.
- If two imports introduce the same simple name, the compiler raises **E0440** (`ambiguous type name`); use fully qualified names to disambiguate.
- Unused imports generate **W0309** (`unused import`) by default.
- Wildcard imports never cause conflicts by themselves; a conflict only arises when a simple name is ambiguous between two or more in-scope types.
- Types in the current package and types in `solix.core` are always in scope without an explicit import.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

import solix.systems.Console;

public class HelloWorld {
    public static void main(string[] args) {
        Console.println("Hello, world!");
    }
}
```

### Idiomatic Usage

```solix
package solix.example.data;

import solix.collections.List;
import solix.collections.Map;
import solix.systems.Console;
import solix.io.*;

public class DataStore {
    private Map store;

    public DataStore() {
        this.store = new Map();
    }

    public void put(string key, string value) {
        store.set(key, value);
    }

    public string get(string key) {
        return (string) store.get(key);
    }

    public void dumpAll() {
        List keys = store.keys();
        for (int32 i = 0; i < keys.size(); i = i + 1) {
            string k = (string) keys.get(i);
            Console.println(k + " => " + get(k));
        }
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| `import` placed after a type declaration | **E0441** `'import' declarations must appear before any type declarations` |
| Two imports resolve to the same simple name | **E0440** `ambiguous type name 'List': imported from 'solix.collections' and 'solix.util'` |
| Unused import | **W0309** `unused import 'solix.io.FileReader'` |
| Importing a non-existent type or package | **E0442** `cannot resolve import 'solix.bogus.Foo'` |

## 6. Related Keywords & Guides

- [`package`](package.md) — declares the current file's namespace
- [`alias`](alias.md) — creates a local shorthand for a type name
