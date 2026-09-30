# `native`

## 1. Summary & Classification

| Property | Value |
|----------|-------|
| Keyword | `native` |
| Category | Modifier |
| Context | Before method declarations |
| Related | [`static`](static.md), [`inline`](inline.md) |

`native` marks a method whose implementation is provided outside the Solix runtime — in platform-specific native code (e.g., C, assembly, or OS syscall wrappers). A `native` method has no Solix body; the compiler generates a call into the native linkage instead. `native` is the primary interoperability mechanism for accessing platform services, hardware interfaces, and performance-critical routines that cannot be written in pure Solix.

## 2. Permitted Contexts (Syntax & Grammar)

```
NativeMethod
    : Modifier* 'native' ReturnType Identifier '(' ParameterList? ')' ';'
    ;
```

- A `native` method declaration ends with a semicolon; it has no body block.
- `native` may be combined with access modifiers and `static`.
- `native` may not be combined with `abstract`, `inline`, or `virtual`.

## 3. Semantics & Compiler Rules

- The compiler does not generate a Solix body for a `native` method; it emits an external symbol reference that the native linker must resolve.
- If the native symbol is not found at link time, the linker raises a linkage error (not a compile error).
- Calling a `native` method is subject to the same type checking as any other method call.
- All parameter and return types must be representable in the native ABI; passing complex Solix objects may require marshalling.
- `native` and `abstract` are mutually exclusive: **E0560**.
- `native` and `inline` are mutually exclusive: **E0561**.

## 4. Code Examples

### Basic Usage

```solix
package solix.systems;

// These methods are implemented in the runtime's native layer.
public class Console {
    public static native void println(string s);
    public static native void println(int32 val);
    public static native void println(float64 val);
    public static native void println(bool val);
}
```

### Idiomatic Usage

```solix
package solix.platform;

// Low-level OS interface — native implementations provided by the platform layer.
public class Os {
    public static native int32 getpid();
    public static native int32 getuid();
    public static native string getenv(string name);
    public static native int32  setenv(string name, string value, bool overwrite);
    public static native void   sleep(int32 milliseconds);
    public static native int64  currentTimeMillis();
}

public class Diagnostics {
    public static void logEnvironment() {
        solix.systems.Console.println("PID: "  + Os.getpid());
        solix.systems.Console.println("UID: "  + Os.getuid());
        solix.systems.Console.println("HOME: " + Os.getenv("HOME"));
    }
}
```

## 5. Common Pitfalls & Compiler Diagnostics

| Mistake | Compiler Diagnostic |
|---------|---------------------|
| `native` method with a body | **E0562** `'native' method 'println' must not have a body` |
| Combining `native` with `abstract` | **E0560** `'native' and 'abstract' cannot be combined` |
| Combining `native` with `inline` | **E0561** `'native' and 'inline' cannot be combined` |
| Native symbol not resolved at link time | *(Linker error, not a compiler error)* `undefined symbol: _solix_Console_println` |

## 6. Related Keywords & Guides

- [`static`](static.md) — often combined with `native` for platform utility functions
- [`inline`](inline.md) — mutually exclusive; use one or the other
- [`abstract`](abstract.md) — mutually exclusive with `native`
