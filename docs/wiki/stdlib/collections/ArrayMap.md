# `ArrayMap<K, V>`

## 1. Class Overview

`solix.collections.ArrayMap<K, V>` is a lightweight, array-backed map optimized for small collections where the memory footprint and overhead of bucket allocation in `HashMap` are undesirable. Keys and values are stored in parallel arrays, providing $O(N)$ lookup and mutation.

- **Package**: `solix.collections`
- **Import**: `import solix.collections.ArrayMap;` or `import solix.collections.*;`

---

## 2. Constructor Reference

| Constructor | Description |
|-------------|-------------|
| `public ArrayMap()` | Initializes an empty array map with default capacity of 8. |
| `public ArrayMap(int32 initial_capacity)` | Initializes an empty array map with the specified initial capacity. |

---

## 3. Method Reference

| Method Signature | Return Type | Description |
|------------------|-------------|-------------|
| `size()` | `int32` | Returns the number of pairs stored in the map. |
| `is_empty()` | `bool` | Returns `true` if `size() == 0`. |
| `put(K key, V value)` | `void` | Inserts or updates the association for `key`. |
| `get(K key)` | `V` | Linearly scans for `key` and returns its value, or `null`. |
| `contains_key(K key)` | `bool` | Returns `true` if `key` exists. |
| `remove(K key)` | `bool` | Removes `key` and shifts array elements. Returns `true` if removed. |
| `clear()` | `void` | Clears all stored key-value pairs. |
| `keys()` | `List<K>` | Returns a list of all keys. |
| `values()` | `List<V>` | Returns a list of all values. |

---

## 4. Code Examples

```solix
package solix.example;

import solix.collections.ArrayMap;
import solix.systems.Console;

public class Main {
    public static void main(char[][] args) {
        ArrayMap<string, string> config = new ArrayMap<string, string>(4);
        config.put("host", "localhost");
        config.put("port", "8080");

        Console.println(config.get("host") + ":" + config.get("port"));
    }
}
```
