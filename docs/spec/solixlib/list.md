# Solix Standard Library: Lists & Linear Sequences (`solix.collections`)

## Overview

The `solix.collections` list module provides two primary sequential data structures:
1. `List`: Dynamic array list providing amortized O(1) appending and O(1) random index-based lookups and updates.
2. `LinkedList`: Doubly-linked node list implementing both `IList` and `IDeque`, optimized for O(1) insertions and removals at both ends (head and tail).
3. `Algorithms`: Static utility functions providing in-place reversing, swapping, filling, sorting, and binary searching.

Both collection types implement `IList`, `ICollection`, `IIterable`, and `IStringable`, supporting canonical string rendering (`[item1, item2, ...]`) and direct terminal output via `Console.println()`.

---

## 1. Class: `solix.collections.List`

Resizable array implementation of `IList`.

### Constructors
- `List()`: Initializes list with default initial capacity of 16.
- `List(int32 initial_capacity)`: Initializes list with specified capacity. Throws `IllegalArgumentException` if `initial_capacity < 0`.

### Capacity Management
- `size() -> int32`: Returns current element count.
- `capacity() -> int32`: Returns current internal array buffer length.
- `is_empty() -> bool`: Returns `true` if `size() == 0`.
- `ensure_capacity(int32 min_capacity) -> void`: Expands buffer to accommodate at least `min_capacity` elements using doubling strategy.
- `shrink_to_fit() -> void`: Trims capacity down to match current size (minimum 4).

### Element Access & Mutation
- `get(int32 index) -> Any`: Returns element at index. Throws `IndexOutOfBoundsException`.
- `set(int32 index, Any item) -> Any`: Replaces element at index and returns prior element. Throws `IndexOutOfBoundsException`.
- `add(Any item) -> void`: Appends item to tail of list.
- `insert(int32 index, Any item) -> void`: Shifts elements right and inserts item at index. Throws `IndexOutOfBoundsException`.
- `remove_at(int32 index) -> Any`: Removes and returns item at index, shifting subsequent elements left. Throws `IndexOutOfBoundsException`.
- `remove(Any item) -> bool`: Searches for first matching item; removes it and returns `true`, or returns `false` if not found.
- `clear() -> void`: Resets size to 0 and clears element references.

### Queries, Slicing & Iteration
- `index_of(Any item) -> int32`: Returns index of first occurrence, or `-1` if absent.
- `contains(Any item) -> bool`: Returns `true` if item is present.
- `add_all(IIterable items) -> void`: Iterates source iterable and appends all items.
- `sub_list(int32 start, int32 count) -> List`: Returns new list containing shallow copy of sub-range. Throws `IndexOutOfBoundsException`.
- `reverse() -> void`: Reverses list elements in place.
- `to_array() -> Any[]`: Returns snapshot copy of elements as an array.
- `iterator() -> IIterator`: Returns forward iterator. Throws `NoSuchElementException` when exhausted.
- `to_string() -> String`: Canonical format `"[e1, e2, ...]"` via `Collections.to_string(this)`.

---

## 2. Class: `solix.collections.LinkedList`

Doubly-linked list implementing both `IList` and `IDeque`.

### Constructors
- `LinkedList()`: Initializes empty doubly-linked list.

### Double-Ended Operations (`IDeque`)
- `add_first(Any item) -> void`: Prepends item to head in O(1).
- `add_last(Any item) -> void`: Appends item to tail in O(1).
- `remove_first() -> Any`: Removes and returns head element in O(1). Throws `NoSuchElementException` if empty.
- `remove_last() -> Any`: Removes and returns tail element in O(1). Throws `NoSuchElementException` if empty.
- `peek_first() -> Any`: Queries head element without removal in O(1). Throws `NoSuchElementException` if empty.
- `peek_last() -> Any`: Queries tail element without removal in O(1). Throws `NoSuchElementException` if empty.

### List Operations (`IList`)
- `size() -> int32`: Returns current node count.
- `is_empty() -> bool`: Returns `true` if count is 0.
- `clear() -> void`: Unlinks all nodes.
- `to_array() -> Any[]`: Allocates array and copies values in order.
- `add(Any item) -> void`: Appends item to tail (alias for `add_last`).
- `get(int32 index) -> Any`: Traverses from closest end (head if index < count/2, else tail) and returns value. Throws `IndexOutOfBoundsException`.
- `set(int32 index, Any item) -> Any`: Updates node value at index and returns old value. Throws `IndexOutOfBoundsException`.
- `insert(int32 index, Any item) -> void`: Inserts new node at index. Throws `IndexOutOfBoundsException`.
- `remove_at(int32 index) -> Any`: Unlinks node at index and returns its value. Throws `IndexOutOfBoundsException`.
- `remove(Any item) -> bool`: Searches from head for matching value and unlinks node.
- `index_of(Any item) -> int32`: Returns 0-based index or `-1`.
- `contains(Any item) -> bool`: Returns `true` if item exists in list.
- `iterator() -> IIterator`: Returns forward iterator.
- `to_string() -> String`: Formatted bracketed string representation.

---

## 3. Class: `solix.collections.Algorithms`

Static utilities for collection operations.

### Methods
- `Algorithms.reverse(IList list) -> void`: Reverses list elements in place using two-pointer swap.
- `Algorithms.swap(IList list, int32 i, int32 j) -> void`: Swaps elements at index `i` and `j`.
- `Algorithms.fill(IList list, Any val) -> void`: Overwrites every position in list with `val`.
- `Algorithms.sort_int32(List list) -> void`: In-place insertion sort for lists containing integer values.
- `Algorithms.binary_search_int32(List list, int32 target) -> int32`: Binary search on sorted integer list. Returns 0-based index or `-1` if target is not found.
