# `Pair<TFirst, TSecond>`

## 1. Class Overview

`solix.collections.Pair<TFirst, TSecond>` is a generic 2-tuple utility container holding two strongly typed values (`first` and `second`). It is useful for returning multiple values from a function, storing key-value pairs, and aggregating related data without creating dedicated classes.

- **Package**: `solix.collections`
- **Import**: `import solix.collections.Pair;` or `import solix.collections.*;`

---

## 2. Constructor Reference

| Constructor | Description |
|-------------|-------------|
| `public Pair()` | Initializes a pair with both fields set to `null`. |
| `public Pair(TFirst first, TSecond second)` | Initializes a pair with the provided values. |
| `public Pair(Pair<TFirst, TSecond> source)` | Copy constructor cloning values from an existing pair. |

---

## 3. Fields & Method Reference

### Public Fields

- `public TFirst first` — The first element.
- `public TSecond second` — The second element.

### Methods

| Method Signature | Return Type | Description |
|------------------|-------------|-------------|
| `get_first()` | `TFirst` | Accessor returning the first element. |
| `set_first(TFirst val)` | `void` | Mutator setting the first element. |
| `get_second()` | `TSecond` | Accessor returning the second element. |
| `set_second(TSecond val)` | `void` | Mutator setting the second element. |
| `equals(Pair<TFirst, TSecond> other)` | `bool` | Returns `true` if both `first` and `second` match `other`. |
| `clone()` | `Pair<TFirst, TSecond>` | Creates a new shallow copy of this pair. |
| `operator=(Pair<TFirst, TSecond> other)` | `Pair<TFirst, TSecond>` | Assignment operator overload. |

---

## 4. Code Examples

```solix
package solix.example;

import solix.collections.Pair;
import solix.systems.Console;

public class Coordinates {
    public static Pair<int32, int32> get_origin() {
        return new Pair<int32, int32>(0, 0);
    }

    public static void main(char[][] args) {
        Pair<int32, int32> pt = Coordinates.get_origin();
        Console.println("X: " + pt.first + ", Y: " + pt.second);
    }
}
```
