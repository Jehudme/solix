# Solix Standard Library: Collections Core (`solix.collections`)

## Overview

The `solix.collections` module establishes the core structural abstractions and foundation for the Solix collection framework. It defines unified generic iteration protocols, read-only and mutable collection contracts, list/deque specifications, associative maps, sets, and collection utilities such as canonical string formatting.

All collection interfaces (`IIterator<T>`, `IIterable<T>`, `IReadOnlyCollection<T>`, `ICollection<T>`, `IList<T>`, `IDeque<T>`, `ISet<T>`, `IMap<K, V>`) are consolidated within `solix/collections/Interfaces.slx`. Concrete implementations strictly encapsulate internal node and bucket structures (e.g. `LinkedListNode`, `HashMapEntry`, `TreeMapNode`) within their implementation modules.

All collections in Solix inherit from `ICollection<T>` or `IReadOnlyCollection<T>`, which extend both `IIterable<T>` and `IStringable`.

---

## 1. Interface: `solix.collections.IIterator<T>`

Provides sequential step-through access across elements of type `T`.

### Methods
- `has_next() -> bool`: Returns `true` if iteration has more elements.
- `next() -> T`: Returns next element in iteration. Throws `NoSuchElementException` if iterator is exhausted.

---

## 2. Interface: `solix.collections.IIterable<T>`

Represents an object that provides an iterator for generic traversal.

### Methods
- `iterator() -> IIterator<T>`: Obtains an iterator over elements.

---

## 3. Interface: `solix.collections.IReadOnlyCollection<T>`

Represents a read-only queryable sequence of elements. Extends `IIterable<T>` and `IStringable`.

### Methods
- `size() -> int32`: Returns total number of elements.
- `is_empty() -> bool`: Returns `true` if `size() == 0`.
- `contains(T item) -> bool`: Returns `true` if specified item is contained.
- `to_array() -> T[]`: Returns snapshot array containing all elements.
- `iterator() -> IIterator<T>`: Returns iterator over elements.
- `to_string() -> String`: Formatted string representation.

---

## 4. Interface: `solix.collections.ICollection<T>`

Base mutable collection interface. Extends `IReadOnlyCollection<T>`.

### Methods
- All methods inherited from `IReadOnlyCollection<T>`: `size()`, `is_empty()`, `contains(T)`, `to_array()`, `iterator()`, `to_string()`.
- `add(T item) -> void`: Adds element to collection.
- `remove(T item) -> bool`: Removes element from collection.
- `clear() -> void`: Removes all elements from collection.

---

## 5. Interface: `solix.collections.IList<T>`

Ordered sequence supporting random index-based access, insertion, and deletion. Extends `ICollection<T>`.

### Methods
- `get(int32 index) -> T`: Returns element at specified 0-based index. Throws `IndexOutOfBoundsException`.
- `set(int32 index, T item) -> void`: Replaces element at specified index.
- `add(T item) -> void`: Appends element to tail of list.
- `insert(int32 index, T item) -> void`: Inserts element at specified index, shifting subsequent elements right.
- `remove_at(int32 index) -> T`: Removes and returns element at specified index.
- `remove(T item) -> bool`: Removes first occurrence of specified item; returns `true` if found and removed.
- `index_of(T item) -> int32`: Returns 0-based index of first occurrence, or `-1` if not present.

---

## 6. Interface: `solix.collections.IDeque<T>`

Double-ended sequence supporting insertion and extraction from both ends. Extends `ICollection<T>`.

### Methods
- `add_first(T item) -> void`: Inserts element at head.
- `add_last(T item) -> void`: Inserts element at tail.
- `remove_first() -> T`: Removes and returns head element. Throws `NoSuchElementException` if empty.
- `remove_last() -> T`: Removes and returns tail element. Throws `NoSuchElementException` if empty.
- `peek_first() -> T`: Returns head element without removing. Throws `NoSuchElementException` if empty.
- `peek_last() -> T`: Returns tail element without removing. Throws `NoSuchElementException` if empty.

---

## 7. Interface: `solix.collections.ISet<T>`

Mathematical set contract guaranteeing uniqueness. Extends `IReadOnlyCollection<T>`.

### Methods
- `add(T item) -> bool`: Adds element if not already present.
- `remove(T item) -> bool`: Removes element if present.
- `clear() -> void`: Removes all elements.
- `union_with(ISet<T> other) -> void`: Performs set union.
- `intersect_with(ISet<T> other) -> void`: Performs set intersection.
- `difference_with(ISet<T> other) -> void`: Performs set difference.

---

## 8. Interface: `solix.collections.IMap<K, V>`

Associative dictionary contract mapping unique keys of type `K` to values of type `V`. Extends `IStringable`.

### Methods
- `size() -> int32`: Returns number of entries.
- `is_empty() -> bool`: Returns `true` if empty.
- `contains_key(K key) -> bool`: Returns `true` if key is present.
- `get(K key) -> V`: Returns value associated with key.
- `put(K key, V value) -> void`: Inserts or updates key-value association.
- `remove(K key) -> V`: Removes key and returns previous value.
- `clear() -> void`: Removes all entries.

---

## 9. Class: `solix.collections.Collections`

Static utility class offering collection operations.

### Methods
- `Collections.to_string<T>(IIterable<T> collection) -> String`: Formats any iterable collection into canonical comma-separated notation, e.g. `"[item1, item2, item3]"`. Integrates seamlessly with `IStringable` and `Console.print(IStringable)`.
