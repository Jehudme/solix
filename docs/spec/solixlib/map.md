# Solix Standard Library: Maps & Associative Dictionaries (`solix.collections`)

## Overview

The `solix.collections` map module provides associative key-value mapping containers:
1. `KeyValuePair`: Encapsulates a single key-value association, implementing `IStringable` formatted as `key: value`.
2. `IMap`: Generic mapping contract specifying key-value insertions, lookups, removals, key/value list extraction, and clear operations.
3. `HashMap`: High-performance hash table with bucket chaining, dynamic load-factor monitoring, and automatic capacity expansion.
4. `TreeMap`: Self-balancing ordered binary search tree maintaining keys in strict sorted order according to `IComparable<K>` (or natural ordering), supporting min/max key queries and ordered key traversal.

Both map implementations implement `IMap`, `IReadOnlyCollection`, `IIterable`, and `IStringable`, supporting canonical string rendering (`"{k1: v1, k2: v2}"`) and direct terminal output via `Console.println()`.

---

## 1. Class: `solix.collections.KeyValuePair`

Encapsulates an immutable association between a key and a value.

### Constructors
- `KeyValuePair(Any key, Any value)`: Creates an entry pair with given key and value.

### Properties / Methods
- `key() -> Any`: Returns the entry key.
- `value() -> Any`: Returns the entry value.
- `to_string() -> String`: Returns `"key: value"`.

---

## 2. Interface: `solix.collections.IMap`

Contract for map collections. Extends `IReadOnlyCollection`, `IIterable`, and `IStringable`.

### Methods
- `size() -> int32`: Returns number of key-value pairs stored.
- `is_empty() -> bool`: Returns `true` if `size() == 0`.
- `clear() -> void`: Removes all entries from map.
- `contains_key(Any key) -> bool`: Returns `true` if key exists in map.
- `contains_value(Any value) -> bool`: Returns `true` if one or more keys map to specified value.
- `get(Any key) -> Any`: Returns value associated with key. Throws `KeyNotFoundException` if key is not found.
- `get_or_default(Any key, Any default_val) -> Any`: Returns value associated with key, or `default_val` if absent.
- `put(Any key, Any value) -> void`: Associates specified value with specified key. If key exists, updates value.
- `remove(Any key) -> Any`: Removes entry with specified key and returns previous value. Throws `KeyNotFoundException` if key is absent.
- `keys() -> List`: Returns a list containing all keys in the map.
- `values() -> List`: Returns a list containing all values in the map.
- `entries() -> List`: Returns a list of `KeyValuePair` entries.

---

## 3. Class: `solix.collections.HashMap`

Hash table implementation of `IMap` using bucket chaining.

### Constructors
- `HashMap()`: Initializes empty hash map with default capacity of 16 and default load factor threshold.
- `HashMap(int32 initial_capacity)`: Initializes empty hash map with specified bucket count. Throws `IllegalArgumentException` if `initial_capacity <= 0`.

### Characteristics & Performance
- **Time Complexity**: Average O(1) for `get`, `put`, `remove`, and `contains_key`.
- **Key Hashing**: Employs `Collections.hash_code(key)` with positive bitmasking (`hash & 0x7FFFFFFF`) to ensure valid table indices.
- **Rehashing**: Automatically expands bucket array and redistributes entries when `size >= capacity * 3 / 4`.
- **String Representation**: Canonical format `"{k1: v1, k2: v2}"`.

---

## 4. Class: `solix.collections.TreeMap`

Ordered binary search tree implementation of `IMap`.

### Constructors
- `TreeMap()`: Initializes an empty tree map.

### Ordered Operations
- `first_key() -> Any`: Returns minimum key in the map according to ordering. Throws `NoSuchElementException` if empty.
- `last_key() -> Any`: Returns maximum key in the map according to ordering. Throws `NoSuchElementException` if empty.

### Characteristics & Performance
- **Time Complexity**: Guaranteed O(log N) for `get`, `put`, `remove`, and `contains_key`.
- **Ordering**: Keys are compared using `IComparable.compare_to()` if implemented, or fallback comparison logic.
- **Ordered Traversal**: In-order traversal produces keys and entries in ascending sorted order.
- **String Representation**: Canonical format `"{k1: v1, k2: v2}"` sorted by key.
