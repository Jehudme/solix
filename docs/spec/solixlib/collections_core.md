# Solix Standard Library: Collections Core (`solix.collections`)

## Overview

The `solix.collections` module establishes the core structural abstractions and foundation for the Solix collection framework. It defines unified iteration protocols, read-only and mutable collection contracts, list/deque specifications, and collection utilities such as canonical string formatting.

All collections in Solix inherit from `ICollection`, which extends both `IIterable` and `IStringable`.

---

## 1. Interface: `solix.collections.IIterator`

Provides sequential step-through access across elements of a collection.

### Methods
- `has_next() -> bool`: Returns `true` if iteration has more elements.
- `next() -> Any`: Returns next element in iteration. Throws `NoSuchElementException` if iterator is exhausted.

---

## 2. Interface: `solix.collections.IIterable`

Represents an object that provides an iterator for traversal.

### Methods
- `iterator() -> IIterator`: Obtains an iterator over elements.

---

## 3. Interface: `solix.collections.IReadOnlyCollection`

Represents a read-only queryable sequence of elements. Extends `IIterable` and `IStringable`.

### Methods
- `size() -> int32`: Returns total number of elements.
- `is_empty() -> bool`: Returns `true` if `size() == 0`.
- `contains(Any item) -> bool`: Returns `true` if specified item is contained.
- `to_array() -> Any[]`: Returns snapshot array containing all elements.

---

## 4. Interface: `solix.collections.ICollection`

Base mutable collection interface. Extends `IReadOnlyCollection`.

### Methods
- All methods inherited from `IReadOnlyCollection`: `size()`, `is_empty()`, `contains(Any)`, `to_array()`, `iterator()`, `to_string()`.
- `clear() -> void`: Removes all elements from collection.

---

## 5. Interface: `solix.collections.IList`

Ordered sequence supporting random index-based access, insertion, and deletion. Extends `ICollection`.

### Methods
- `get(int32 index) -> Any`: Returns element at specified 0-based index. Throws `IndexOutOfBoundsException`.
- `set(int32 index, Any item) -> Any`: Replaces element at specified index and returns replaced element.
- `add(Any item) -> void`: Appends element to tail of list.
- `insert(int32 index, Any item) -> void`: Inserts element at specified index, shifting subsequent elements right.
- `remove_at(int32 index) -> Any`: Removes and returns element at specified index.
- `remove(Any item) -> bool`: Removes first occurrence of specified item; returns `true` if found and removed.
- `index_of(Any item) -> int32`: Returns 0-based index of first occurrence, or `-1` if not present.

---

## 6. Interface: `solix.collections.IDeque`

Double-ended sequence supporting insertion and extraction from both ends. Extends `ICollection`.

### Methods
- `add_first(Any item) -> void`: Inserts element at head.
- `add_last(Any item) -> void`: Inserts element at tail.
- `remove_first() -> Any`: Removes and returns head element. Throws `NoSuchElementException` if empty.
- `remove_last() -> Any`: Removes and returns tail element. Throws `NoSuchElementException` if empty.
- `peek_first() -> Any`: Returns head element without removing. Throws `NoSuchElementException` if empty.
- `peek_last() -> Any`: Returns tail element without removing. Throws `NoSuchElementException` if empty.

---

## 7. Class: `solix.collections.Collections`

Static utility class offering collection operations.

### Methods
- `Collections.to_string(IIterable collection) -> String`: Formats any iterable collection into canonical comma-separated notation, e.g. `"[item1, item2, item3]"`. Integrates seamlessly with `IStringable` and `Console.print(IStringable)`.
