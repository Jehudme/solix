# `HashSet<T>`

## 1. Class Overview

`solix.collections.HashSet<T>` is a set collection that stores unique elements with no duplicates, backed by a bucketed hash table. Elements are hashed using `Objects.hash_code<T>()` and checked for equality using `Objects.equals<T>()`.

- **Package**: `solix.collections`
- **Import**: `import solix.collections.HashSet;` or `import solix.collections.*;`

---

## 2. Constructor Reference

| Constructor | Description |
|-------------|-------------|
| `public HashSet()` | Initializes an empty hash set with 16 buckets. |
| `public HashSet(int32 initial_capacity)` | Initializes an empty hash set with the specified bucket capacity. |

---

## 3. Method Reference

| Method Signature | Return Type | Description |
|------------------|-------------|-------------|
| `size()` | `int32` | Returns the number of unique elements in the set. |
| `is_empty()` | `bool` | Returns `true` if `size() == 0`. |
| `add(T item)` | `bool` | Adds `item` to the set. Returns `true` if added, `false` if already present. |
| `contains(T item)` | `bool` | Returns `true` if `item` exists in the set ($O(1)$ average). |
| `remove(T item)` | `bool` | Removes `item` from the set. Returns `true` if removed. |
| `clear()` | `void` | Removes all elements from the set. |
| `to_list()` | `List<T>` | Returns all unique elements as a newly created `List<T>`. |

---

## 4. Code Examples

```solix
package solix.example;

import solix.collections.HashSet;
import solix.systems.Console;

public class Main {
    public static void main(char[][] args) {
        HashSet<string> tags = new HashSet<string>();
        tags.add("compiler");
        tags.add("vm");
        tags.add("compiler"); // Duplicate, ignored

        Console.println("Set size: " + tags.size()); // 2
        Console.println(tags.contains("vm"));       // true
    }
}
```
