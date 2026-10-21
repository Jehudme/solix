# Solix Standard Library: Maps & Associative Dictionaries (`solix.collections`)

## Overview

The `solix.collections` map module provides associative key-value mapping containers:
1. `KeyValuePair<K, V>`: Encapsulates a single key-value association, implementing `IStringable`.
2. `HashMap<K, V>`: High-performance hash table with bucket chaining, dynamic load-factor monitoring, functional `for_each(void(*)(K, V) action)`, and automatic capacity expansion.
3. `TreeMap<K, V>`: Self-balancing ordered binary search tree maintaining keys in sorted order according to a comparator lambda `int32(*)(K, K)`, supporting min/max key queries, ordered key traversal, and functional `for_each(void(*)(K, V) action)`.

Both map implementations implement `IMap<K, V>` and `IStringable`, supporting polymorphic dictionary operations and canonical string rendering (`"[HashMap]"`, `"[TreeMap]"`). Internal node structures are strictly encapsulated.

---

## 1. Class: `solix.collections.KeyValuePair<K, V>`

Encapsulates an association between a key of type `K` and a value of type `V`.

### Constructors
- `KeyValuePair(K key, V value)`: Creates an entry pair with given key and value.

### Properties / Methods
- `get_key() -> K`: Returns the entry key.
- `get_value() -> V`: Returns the entry value.
- `to_string() -> String`: Returns `"[KeyValuePair]"`.

---

## 2. Class: `solix.collections.HashMap<K, V>`

Hash table implementation using bucket chaining. Implements `IMap<K, V>` and `IStringable`.

### Constructors
- `HashMap()`: Initializes empty hash map with default capacity of 16 and default load factor threshold.
- `HashMap(int32 initial_capacity)`: Initializes empty hash map with specified bucket count.
- `HashMap(int32(*)(K) hasher)`: Initializes hash map with custom key hasher function pointer.
- `HashMap(int32 initial_capacity, int32(*)(K) hasher)`: Initializes hash map with initial capacity and custom hasher.

### Methods
- `size() -> int32`: Returns number of key-value pairs stored.
- `is_empty() -> bool`: Returns `true` if `size() == 0`.
- `clear() -> void`: Removes all entries from map.
- `contains_key(K key) -> bool`: Returns `true` if key exists in map.
- `contains_value(V value) -> bool`: Returns `true` if one or more keys map to specified value.
- `get(K key) -> V`: Returns value associated with key. Throws `KeyNotFoundException` if key is not found.
- `get_or_default(K key, V default_val) -> V`: Returns value associated with key, or `default_val` if absent.
- `put(K key, V value) -> void`: Associates specified value with specified key. If key exists, updates value.
- `remove(K key) -> V`: Removes entry with specified key and returns previous value.
- `keys() -> List<K>`: Returns a list containing all keys in the map.
- `values() -> List<V>`: Returns a list containing all values in the map.
- `entries() -> List<KeyValuePair<K, V>>`: Returns a list of `KeyValuePair<K, V>` entries.
- `for_each(void(*)(K, V) action) -> void`: Executes callback action for every key-value pair.
- `to_string() -> String`: Returns `"[HashMap]"`.

---

## 3. Class: `solix.collections.TreeMap<K, V>`

Ordered binary search tree implementation. Implements `IMap<K, V>` and `IStringable`.

### Constructors
- `TreeMap()`: Initializes an empty tree map with default comparator.
- `TreeMap(int32(*)(K, K) comparator)`: Initializes tree map with explicit key comparison function pointer.

### Methods
- `first_key() -> K`: Returns minimum key in the map according to ordering. Throws `NoSuchElementException` if empty.
- `last_key() -> K`: Returns maximum key in the map according to ordering. Throws `NoSuchElementException` if empty.
- `get(K key) -> V`: Returns value associated with key. Throws `KeyNotFoundException` if key is absent.
- `get_or_default(K key, V default_val) -> V`: Returns value associated with key, or default if absent.
- `put(K key, V value) -> void`: Inserts or updates key-value pair in tree.
- `remove(K key) -> V`: Removes key from tree and returns old value.
- `keys() -> List<K>`: In-order traversal returning keys in ascending order.
- `values() -> List<V>`: In-order traversal returning values ordered by key.
- `entries() -> List<KeyValuePair<K, V>>`: In-order traversal returning key-value pairs.
- `for_each(void(*)(K, V) action) -> void`: In-order iteration executing action on each pair.
- `to_string() -> String`: Returns `"[TreeMap]"`.
