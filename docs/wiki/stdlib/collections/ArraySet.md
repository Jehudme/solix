# `ArraySet<T>`

## 1. Class Overview

`solix.collections.ArraySet<T>` is a compact, array-backed set for storing unique elements. It performs linear equality scans for insertion and lookup, making it memory-efficient for small element counts where hash table overhead is undesirable.

- **Package**: `solix.collections`
- **Import**: `import solix.collections.ArraySet;` or `import solix.collections.*;`

---

## 2. Constructor Reference

| Constructor | Description |
|-------------|-------------|
| `public ArraySet()` | Initializes an empty array set with default capacity of 8. |
| `public ArraySet(int32 initial_capacity)` | Initializes an empty array set with the specified capacity. |

---

## 3. Method Reference

| Method Signature | Return Type | Description |
|------------------|-------------|-------------|
| `size()` | `int32` | Returns the number of elements in the set. |
| `is_empty()` | `bool` | Returns `true` if `size() == 0`. |
| `add(T item)` | `bool` | Adds `item` if not present. Returns `true` if inserted, `false` if already exists ($O(N)$). |
| `contains(T item)` | `bool` | Scans array to determine if `item` is present ($O(N)$). |
| `remove(T item)` | `bool` | Removes `item` and shifts remaining elements left ($O(N)$). |
| `clear()` | `void` | Clears all elements. |
| `to_list()` | `List<T>` | Returns a new `List<T>` containing all elements. |

---

## 4. Code Examples

```solix
package solix.example;

import solix.collections.ArraySet;
import solix.systems.Console;

public class Main {
    public static void main(char[][] args) {
        ArraySet<int32> set = new ArraySet<int32>(4);
        set.add(10);
        set.add(20);
        set.add(10); // Duplicate, returns false

        Console.println(set.size()); // 2
    }
}
```
