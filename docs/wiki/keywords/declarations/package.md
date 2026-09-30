# `package`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `package` |
| Category | Declaration |
| Context | First non-comment statement in a source file |
| Related | [`import`](import.md) |

`package` declares the namespace to which all top-level types in the current source file belong. Packages form a hierarchical dot-separated naming scheme that mirrors the file-system directory structure, preventing naming collisions across large projects and third-party libraries. A source file without a `package` declaration belongs to the unnamed default package, which is reserved for small scripts and is inaccessible from named packages.

## 2. Permitted Contexts (Syntax & Grammar)

```
PackageDeclaration
    : 'package' QualifiedName ';'
    ;

QualifiedName
    : Identifier ( '.' Identifier )*
    ;
```

- The `package` declaration must be the first statement in the file (preceding any `import` declarations and type declarations).
- Only one `package` declaration is permitted per source file.
- The qualified name should correspond to the directory path relative to the source root.

## 3. Semantics & Compiler Rules

- All types declared in a file belong to the package named in that file's `package` declaration.
- The `internal` access modifier restricts visibility to members of the same package.
- Types in the same package may reference each other by simple name without an `import`.
- Types in different packages require either a fully qualified name or an `import` declaration.
- Package names are case-sensitive; convention uses lowercase identifiers separated by dots.

## 4. Code Examples

### Basic Usage

```solix
package solix.myapp;

public class AppMain {
    public static void main(string[] args) {
        // entry point
    }
}
```

### Idiomatic Usage

```solix
package solix.myapp.data;

import solix.collections.List;
import solix.systems.Console;

// Both UserRepository and UserRecord are in package solix.myapp.data.
// They can refer to each other by simple name.

public class UserRecord {
    public string name;
    public string email;

    public UserRecord(string name, string email) {
        this.name = name;
        this.email = email;
    }
}

public class UserRepository {
    private List records;

    public UserRepository() {
        this.records = new List();
    }

    public void add(UserRecord r) {
        records.add(r);
    }

    public int32 count() {
        return records.size();
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| `package` declaration not at start of file | **E0430** `'package' declaration must be the first statement in the file` |
| Multiple `package` declarations in one file | **E0431** `only one 'package' declaration is allowed per source file` |
| Package name contains uppercase letters (convention warning) | **W0307** `package name segment 'MyApp' should be lowercase` |
| File path does not match package declaration | **W0308** `file is in directory 'myapp/ui/' but declares package 'solix.myapp.data'` |

## 6. Related Keywords & Guides

- [`import`](import.md) — brings types from other packages into scope
- [`internal`](../modifiers/internal.md) — restricts visibility to the same package
- [`public`](../modifiers/public.md) — makes types accessible from other packages
