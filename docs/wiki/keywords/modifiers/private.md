# `private`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `private` |
| Category | Modifier |
| Context | Before field, method, constructor, or nested class declarations |
| Related | [`public`](public.md), [`protected`](protected.md), [`internal`](internal.md) |

`private` is the most restrictive access modifier in Solix. A `private` member is visible only within the body of the class (or nested type) that declares it. It is inaccessible from subclasses, sibling classes in the same package, and all external code. Using `private` aggressively for implementation details is considered best practice, as it minimises coupling and protects invariants.

## 2. Permitted Contexts (Syntax & Grammar)

```
PrivateDeclaration
    : 'private' MemberDeclaration
    ;
```

- `private` may be applied to fields, methods, constructors, and nested classes.
- `private` may not be applied to top-level classes or interfaces — a top-level type cannot be `private`.
- Only one access modifier per declaration is permitted.

## 3. Semantics & Compiler Rules

- Access to a `private` member from outside the declaring class generates **E0510** (`private member not accessible`).
- `private` members are not inherited; a subclass cannot access the `private` members of its parent class directly. It must use `protected` or `public` accessor methods.
- `private` constructors prevent instantiation from outside the class, enabling the Singleton and Factory patterns.
- The compiler issues **W0311** if a `private` member is declared but never read or called within the class.

## 4. Code Examples

### Basic Usage

```solix
package solix.example;

public class BankAccount {
    private float64 balance;

    public BankAccount(float64 initial) {
        this.balance = initial;
    }

    public void deposit(float64 amount) {
        if (amount > 0.0) {
            balance = balance + amount;
        }
    }

    public float64 getBalance() {
        return balance;
    }
}
```

### Idiomatic Usage

```solix
package solix.example;

// Singleton via private constructor
public class AppConfig {
    private static AppConfig instance = null;
    private string configPath;
    private bool   debugMode;

    private AppConfig() {
        this.configPath = "/etc/myapp/config.slx";
        this.debugMode  = false;
    }

    public static AppConfig getInstance() {
        if (instance == null) {
            instance = new AppConfig();
        }
        return instance;
    }

    public bool isDebug() {
        return debugMode;
    }

    public string getConfigPath() {
        return configPath;
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| Accessing a `private` member from a subclass | **E0510** `'balance' is private in 'BankAccount' and not accessible from 'SavingsAccount'` |
| Applying `private` to a top-level class | **E0511** `access modifier 'private' cannot be applied to a top-level type` |
| Unused `private` member | **W0311** `private field 'cache' is never read` |

## 6. Related Keywords & Guides

- [`public`](public.md) — broadest access; accessible from everywhere
- [`protected`](protected.md) — accessible from subclasses
- [`internal`](internal.md) — accessible within the same package
