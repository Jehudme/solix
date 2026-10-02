# 08 - Generics & Template Metaprogramming

Generics allow you to write reusable, type-safe data structures and algorithms that operate across any data type without sacrificing performance or type safety.

In Solix, generics are implemented via compile-time **monomorphization** (similar to C++ templates and Rust generics). When you instantiate a generic type with concrete types (such as `int32` or `String`), the Solix compiler emits specialized bytecode specifically tailored for those types, eliminating boxing, unboxing, and runtime type casts.

---

## 1. Generic Classes

Declare a generic class by specifying type parameters in angle brackets `<...>`:

```solix
public class Box<T> {
    public T value;

    public Box(T value) {
        this.value = value;
    }

    public T getValue() {
        return this.value;
    }
}
```

### Instantiation
Instantiate generic classes with concrete type arguments:
```solix
Box<int32> intBox = new Box<int32>(42);
int32 num = intBox.getValue(); // 42

Box<String> strBox = new Box<String>("Hello, Solix!");
String text = strBox.getValue(); // "Hello, Solix!"
```

---

## 2. Multiple Type Parameters

Classes can declare multiple type parameters separated by commas:

```solix
public class Pair<K, V> {
    public K key;
    public V value;

    public Pair(K key, V value) {
        this.key = key;
        this.value = value;
    }
}

// In main():
Pair<int32, int32> point = new Pair<int32, int32>(10, 20);
Pair<String, int32> score = new Pair<String, int32>("Alice", 100);
```

---

## 3. Generic Methods

Methods can declare their own template parameters independently of class-level parameters:

```solix
public class MathAlgorithms {
    public static T identity<T>(T value) {
        return value;
    }

    public static T chooseFirst<T>(T first, T second) {
        return first;
    }
}
```

### Explicit Type Arguments
You can explicitly supply the type arguments when calling the method:
```solix
int32 a = MathAlgorithms.identity<int32>(55);
```

### Implicit Template Argument Deduction
Solix automatically deduces template arguments from the passed argument types when unambiguous:
```solix
int32 val = MathAlgorithms.identity(99);              // Inferred as <int32>
int32 chosen = MathAlgorithms.chooseFirst(100, 200);   // Inferred as <int32>
```

---

## 4. Generic Interfaces & Polymorphism

Generic classes can implement non-generic or generic interfaces:

```solix
public interface IContainer {
    int32 getVal();
}

public class Holder<T> implements IContainer {
    public T item;

    public Holder(T item) {
        this.item = item;
    }

    public int32 getVal() {
        return 99;
    }
}

public class Main {
    public static int32 main() {
        IContainer c = new Holder<int32>(42);
        return c.getVal(); // Dispatches dynamically via interface vtable
    }
}
```

---

## 5. Generic Classes with Function Pointers

Type parameters can be used seamlessly in function pointer fields and method signatures:

```solix
public class Processor<T> {
    public T(*)(T) transform;

    public Processor(T(*)(T) fn) {
        this.transform = fn;
    }

    public T execute(T input) {
        return this.transform(input);
    }
}

public class Main {
    public static int32 main() {
        Processor<int32> doubler = new Processor<int32>((x) => x * 2);
        return doubler.execute(21); // 42
    }
}
```

---

## 6. Nested Generic Types

Generic types can be nested to build complex composite data structures:

```solix
public class Cell<T> {
    public T content;
    public Cell(T c) {
        this.content = c;
    }
}

// In main():
Cell<Cell<int32>> matrix = new Cell<Cell<int32>>(new Cell<int32>(777));
int32 value = matrix.content.content; // 777
```

---

## 7. Compilation & Monomorphization Details

Understanding how Solix compiles generics helps you write more efficient code:

1. **No Runtime Overhead**:
   - `Box<int32>` allocates memory sized precisely for an `int32` field.
   - No heap boxing or object wrappers are introduced for primitive types.

2. **Deduplication**:
   - Multiple uses of `Box<int32>` across different files share the same monomorphized class and method implementations.

3. **Separate VTables**:
   - Each unique specialization (e.g., `Holder<int32>` vs `Holder<float64>`) receives its own virtual method table and interface dispatch table (`itable`), ensuring correct polymorphic dispatch.
