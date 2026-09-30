# `HashMap<K, V>`

## 1. Class Overview

`solix.collections.HashMap<K, V>` is a hash-table-based associative mapping from keys of type `K` to values of type `V`. It resolves hash collisions via separate chaining using linked bucket nodes and computes key hashes via `Objects.hash_code<K>(key)`.

- **Package**: `solix.collections`
- **Import**: `import solix.collections.HashMap;` or `import solix.collections.*;`

---

## 2. Constructor Reference

| Constructor | Description |
|-------------|-------------|
| `public HashMap()` | Initializes an empty hash map with 16 buckets. |
| `public HashMap(int32 initial_capacity)` | Initializes an empty hash map with the given bucket count (minimum 4). |

---

## 3. Method Reference

| Method Signature | Return Type | Description |
|------------------|-------------|-------------|
| `size()` | `int32` | Returns the total count of key-value pairs stored in the map. |
| `capacity()` | `int32` | Returns the number of hash buckets. |
| `is_empty()` | `bool` | Returns `true` if `size() == 0`. |
| `put(K key, V value)` | `void` | Inserts or updates the association for `key`. |
| `get(K key)` | `V` | Retrieves the value associated with `key`, or `null` if not found. |
| `contains_key(K key)` | `bool` | Returns `true` if `key` exists in the map. |
| `remove(K key)` | `bool` | Removes the mapping for `key`. Returns `true` if found and removed. |
| `clear()` | `void` | Removes all key-value mappings. |
| `keys()` | `List<K>` | Returns a list of all keys currently in the map. |
| `values()` | `List<V>` | Returns a list of all values currently in the map. |

---

## 4. Code Examples

```solix
package solix.example;

import solix.collections.HashMap;
import solix.systems.Console;

public class Main {
    public static void main(char[][] args) {
        HashMap<string, int32> scores = new HashMap<string, int32>();
        scores.put("Alice", 95);
        scores.put("Bob", 88);
        scores.put("Charlie", 72);

        if (scores.contains_key("Alice")) {
            Console.println("Alice's score: " + scores.get("Alice"));
        }

        scores.remove("Bob");
        Console.println("Total players: " + scores.size());
    }
}
```
