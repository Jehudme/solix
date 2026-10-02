# Generic Class & Method Declarations

## 1. Overview & Purpose

Solix supports compile-time generics (template metaprogramming) for classes, interfaces, and methods. Generics allow writing reusable, type-safe data structures and algorithms without runtime type erasure or boxing overhead.

Solix uses compile-time **monomorphization**: each unique specialization of a template class or method (e.g., `Box<int32>` vs `Box<String>`) generates specialized, optimal bytecode.

### Supported Features
- **Generic Classes**: `public class Box<T> { public T item; }`
- **Multiple Type Parameters**: `public class Pair<K, V> { public K key; public V value; }`
- **Generic Interfaces & Implementations**:
  ```solix
  public interface IContainer { int32 getVal(); }
  public class Holder<T> implements IContainer {
      public T item;
      public int32 getVal() { return 99; }
  }
  ```
- **Generic Methods**:
  ```solix
  public static T convert<T>(T val) { return val; }
  ```
- **Implicit Template Argument Deduction**: Type arguments for generic methods are inferred automatically from call arguments when unambiguous:
  ```solix
  int32 v = Deduce.identity(123); // Deduced as Deduce.identity<int32>(123)
  ```
- **Template Function Pointer Fields**:
  ```solix
  public class Processor<T> {
      public T(*)(T) transform;
  }
  ```
- **Nested Generic Types**: `Cell<Cell<int32>>`

---

## 2. Compilation & Monomorphization Mechanics

### Blueprint Registration & AST Cloning
1. **Pass 1 (Registration)**:
   - Any class or method containing template parameters (e.g., `<T>`, `<K, V>`) is registered in the binder's `template_registry`.
   - Blueprints are not emitted into the final bytecode until instantiated.

2. **Template Instantiation (`instantiate_template`)**:
   - When a type with type arguments is resolved (e.g., `Box<int32>`), the compiler checks if an instantiation with mangled name `Box<int32>` already exists.
   - If not cached, the blueprint AST is deep-cloned.
   - `TemplateSubstitutionVisitor` recursively replaces occurrences of type parameters (`T`) with the concrete argument (`int32`), including inside function pointers (`T(*)(T)` -> `int32(*)(int32)`), nested templates, and method return/parameter signatures.
   - Cloned classes are assigned distinct `vtable_id`s, have their memory layouts and reference field offsets computed, and generate interface dispatch tables (`itables`).

### Bytecode Efficiency
Because Solix monomorphizes templates at compile time:
- Primitive instantiations (`Box<int32>`, `Box<float64>`) avoid any object boxing or dynamic type tag inspection.
- Operations on `T` (such as field accesses or arithmetic) compile directly into primitive instructions (`ADD_I64`, `SET_PROPERTY`).
