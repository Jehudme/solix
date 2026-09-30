# `internal`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `internal` |
| Category | Modifier |
| Context | Before class, interface, enum, field, method, or constructor declarations |
| Related | [`public`](public.md), [`private`](private.md), [`protected`](protected.md), [`package`](../declarations/package.md) |

`internal` restricts access to the current package. A member or type declared `internal` is visible to all classes and interfaces within the same package, but is inaccessible from other packages, even through inheritance. `internal` is the implicit default access level when no modifier is specified, making it a useful modifier for package-level implementation utilities that are shared across files but should not leak into a library's public API.

## 2. Permitted Contexts (Syntax & Grammar)

```
InternalDeclaration
    : 'internal' MemberDeclaration
    | 'internal' TypeDeclaration
    ;
```

- May be applied to top-level types, fields, methods, and constructors.
- Only one access modifier per declaration is permitted.
- If no access modifier is written, `internal` is assumed.

## 3. Semantics & Compiler Rules

- Members or types declared `internal` (or with no modifier) are accessible from any class within the same package.
- Access from outside the package generates **E0530** (`internal member not accessible`).
- `internal` cannot be combined with `protected`; a member is either package-scoped or subclass-scoped, not both.
- Subclasses in a different package cannot access `internal` members of their parent class.

## 4. Code Examples

### Basic Usage

```solix
package solix.mylib;

// 'internal' is explicit here; omitting the modifier would be equivalent.
internal class ConnectionPool {
    private int32 maxConnections;

    internal ConnectionPool(int32 max) {
        this.maxConnections = max;
    }

    internal solix.io.Connection acquire() {
        // implementation
        return null;
    }
}
```

### Idiomatic Usage

```solix
package solix.mylib;

import solix.systems.Console;

// Public facade — this is part of the exported API.
public class DatabaseClient {
    private ConnectionPool pool;

    public DatabaseClient(int32 poolSize) {
        // ConnectionPool is internal; consumers can only use DatabaseClient.
        this.pool = new ConnectionPool(poolSize);
    }

    public string query(string sql) {
        solix.io.Connection conn = pool.acquire();
        try {
            return conn.execute(sql);
        } finally {
            conn.close();
        }
    }
}

// Also internal — implementation detail shared across the package.
internal class QueryCache {
    internal void cache(string key, string result) {
        // ...
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Accessing an `internal` type from another package | **E0530** `'ConnectionPool' is internal to package 'solix.mylib'` |
| Combining `internal` with another access modifier | **E0501** `only one access modifier is permitted per declaration` |
| Expecting `internal` members to be accessible in a subclass from a different package | **E0530** `'acquire()' is internal in 'ConnectionPool' and not accessible from 'ExtendedPool'` |

## 6. Related Keywords & Guides

- [`public`](public.md) — accessible from all packages
- [`private`](private.md) — accessible only within the declaring class
- [`protected`](protected.md) — accessible within the class and subclasses
- [`package`](../declarations/package.md) — defines the package boundary that `internal` respects
