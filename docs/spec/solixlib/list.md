# Solix Standard Library: Lists & Linear Sequences (`solix.collections`)

## Overview

The `solix.collections` list module provides compile-time generic sequential data structures and utility algorithms:
1. `List<T>`: Dynamic array list providing amortized O(1) appending, O(1) random index-based lookups/updates, first-class `for_each` lambda iteration, and functional `filter` transformation.
2. `LinkedList<T>`: Doubly-linked node list implementing bidirectional node traversal, O(1) head/tail operations, `for_each` lambda iteration, and functional `filter` transformation.
3. `Algorithms`: Generic static utility functions providing in-place reversing, swapping, filling, comparator-based sorting, and binary searching.

Both collection types implement `IList<T>`, `ICollection<T>`, `IIterable<T>`, and `IStringable`, supporting polymorphic collection processing and canonical bracketed string representations.

---

## 1. Class: `solix.collections.List<T>`

Compile-time generic resizable array container. Implements `IList<T>`, `ICollection<T>`, `IIterable<T>`, and `IStringable`.

### Constructors
- `List()`: Initializes list with default initial capacity of 16.
- `List(int32 initial_capacity)`: Initializes list with specified capacity. Throws `IllegalArgumentException` if `initial_capacity < 0`.

### Capacity Management
- `size() -> int32`: Returns current element count.
- `capacity() -> int32`: Returns current internal array buffer length.
- `is_empty() -> bool`: Returns `true` if `size() == 0`.
- `ensure_capacity(int32 min_capacity) -> void`: Expands buffer to accommodate at least `min_capacity` elements using a doubling strategy.
- `shrink_to_fit() -> void`: Trims capacity down to match current size (minimum 4).

### Element Access & Mutation
- `get(int32 index) -> T`: Returns element at index. Throws `IndexOutOfBoundsException`.
- `set(int32 index, T item) -> T`: Replaces element at index and returns prior element. Throws `IndexOutOfBoundsException`.
- `add(T item) -> void`: Appends item to tail of list.
- `insert(int32 index, T item) -> void`: Shifts elements right and inserts item at index. Throws `IndexOutOfBoundsException`.
- `remove_at(int32 index) -> T`: Removes and returns item at index, shifting subsequent elements left. Throws `IndexOutOfBoundsException`.
- `remove(T item) -> bool`: Searches for first matching item; removes it and returns `true`, or returns `false` if not found.
- `clear() -> void`: Resets size to 0 and clears element references.

### Queries, Slicing & Functional Iteration
- `index_of(T item) -> int32`: Returns index of first occurrence, or `-1` if absent.
- `contains(T item) -> bool`: Returns `true` if item is present.
- `sub_list(int32 start, int32 count) -> List<T>`: Returns new list containing shallow copy of sub-range. Throws `IndexOutOfBoundsException`.
- `reverse() -> void`: Reverses list elements in place.
- `to_array() -> T[]`: Returns snapshot copy of elements as a typed array.
- `for_each(void(*)(T) action) -> void`: Iterates sequentially across all elements, invoking `action(item)` on each.
- `filter(bool(*)(T) predicate) -> List<T>`: Returns a new `List<T>` containing all elements satisfying `predicate(item) == true`.
- `iterator() -> IIterator<T>`: Returns forward iterator implementing `IIterator<T>`.
- `to_string() -> String`: Standard bracketed representation `"[List]"`.

---

## 2. Class: `solix.collections.LinkedList<T>`

Compile-time generic doubly-linked list supporting efficient node insertion and removal at both ends. Implements `IList<T>`, `IDeque<T>`, `ICollection<T>`, `IIterable<T>`, and `IStringable`. Node structures are strictly encapsulated.

### Constructors
- `LinkedList()`: Initializes empty doubly-linked list.

### Double-Ended Operations
- `add_first(T item) -> void`: Prepends item to head in O(1).
- `add_last(T item) -> void`: Appends item to tail in O(1).
- `remove_first() -> T`: Removes and returns head element in O(1). Throws `NoSuchElementException` if empty.
- `remove_last() -> T`: Removes and returns tail element in O(1). Throws `NoSuchElementException` if empty.
- `peek_first() -> T`: Queries head element without removal in O(1). Throws `NoSuchElementException` if empty.
- `peek_last() -> T`: Queries tail element without removal in O(1). Throws `NoSuchElementException` if empty.

### List Operations
- `size() -> int32`: Returns current node count.
- `is_empty() -> bool`: Returns `true` if count is 0.
- `clear() -> void`: Unlinks all nodes.
- `to_array() -> T[]`: Allocates typed array and copies values in order.
- `add(T item) -> void`: Appends item to tail (alias for `add_last`).
- `get(int32 index) -> T`: Traverses from closest end (head if index < count/2, else tail) and returns value. Throws `IndexOutOfBoundsException`.
- `set(int32 index, T item) -> T`: Updates node value at index and returns old value. Throws `IndexOutOfBoundsException`.
- `insert(int32 index, T item) -> void`: Inserts new node at index. Throws `IndexOutOfBoundsException`.
- `remove_at(int32 index) -> T`: Unlinks node at index and returns its value. Throws `IndexOutOfBoundsException`.
- `remove(T item) -> bool`: Searches from head for matching value and unlinks node.
- `index_of(T item) -> int32`: Returns 0-based index or `-1`.
- `contains(T item) -> bool`: Returns `true` if item exists in list.
- `for_each(void(*)(T) action) -> void`: Iterates across all nodes from head to tail, invoking `action(item)` on each.
- `filter(bool(*)(T) predicate) -> LinkedList<T>`: Returns a new `LinkedList<T>` containing all elements satisfying `predicate(item) == true`.
- `iterator() -> LinkedListIterator<T>`: Returns forward node iterator.
- `to_string() -> String`: Standard bracketed representation `"[LinkedList]"`.

---

## 3. Class: `solix.collections.Algorithms`

Generic static utilities for linear sequences.

### Methods
- `Algorithms.reverse<T>(List<T> list) -> void`: Reverses list elements in place using two-pointer swap.
- `Algorithms.swap<T>(List<T> list, int32 i, int32 j) -> void`: Swaps elements at index `i` and `j`.
- `Algorithms.fill<T>(List<T> list, T val) -> void`: Overwrites every position in list with `val`.
- `Algorithms.sort<T>(List<T> list, int32(*)(T, T) comparator) -> void`: In-place insertion sort for lists using the specified comparator lambda.
- `Algorithms.binary_search<T>(List<T> list, T target, int32(*)(T, T) comparator) -> int32`: Binary search on sorted list using the comparator. Returns 0-based index or `-1` if target is not found.
