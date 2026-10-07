# Solix Standard Library: Sets & Distinct Containers (`solix.collections`)

## Overview

The `solix.collections` set module provides mathematical set abstractions guaranteeing distinct, duplicate-free elements:
1. `HashSet<T>`: High-performance hash table backed set providing average O(1) insertions, lookups, and deletions, with functional `for_each(void(*)(T) action)`.
2. `TreeSet<T>`: Self-balancing binary search tree backed set maintaining elements in sorted order according to a comparator lambda `int32(*)(T, T)`, supporting boundary element queries (`first()`, `last()`), ordered iteration, and functional `for_each(void(*)(T) action)`.

Both implementations implement `ISet<T>`, `IReadOnlyCollection<T>`, `IIterable<T>`, and `IStringable`, supporting polymorphic set processing and canonical string formatting (`"[HashSet]"`, `"[TreeSet]"`).

---

## 1. Class: `solix.collections.HashSet<T>`

Hash-table backed distinct set implementation. Implements `ISet<T>`, `IReadOnlyCollection<T>`, `IIterable<T>`, and `IStringable`.

### Constructors
- `HashSet()`: Initializes an empty hash set with default initial capacity (16).
- `HashSet(int32 initial_capacity)`: Initializes an empty hash set with specified initial bucket capacity.
- `HashSet(int32(*)(T) hasher)`: Initializes hash set with custom element hasher function pointer.
- `HashSet(int32 initial_capacity, int32(*)(T) hasher)`: Initializes hash set with initial capacity and custom hasher.

### Methods
- `bool add(T item)`: Adds element if not already present. Returns `true` if added, `false` if duplicate.
- `bool remove(T item)`: Removes element if present. Returns `true` if removed, `false` otherwise.
- `bool contains(T item)`: Checks if element is present.
- `int32 size()`: Returns count of unique elements.
- `bool is_empty()`: Returns true if set is empty.
- `void clear()`: Removes all elements.
- `T[] to_array()`: Exports elements into an array.
- `List<T> to_list()`: Exports elements into a generic `List<T>`.
- `void for_each(void(*)(T) action)`: Executes action on every element.
- `void union_with(HashSet<T> other)`: Adds all elements from `other`.
- `void union_with(ISet<T> other)`: Adds all elements from another set implementing `ISet<T>`.
- `void intersect_with(HashSet<T> other)`: Retains only elements present in both sets.
- `void intersect_with(ISet<T> other)`: Retains only elements present in both sets.
- `void difference_with(HashSet<T> other)`: Removes all elements present in `other`.
- `void difference_with(ISet<T> other)`: Removes all elements present in `other`.
- `bool is_subset_of(HashSet<T> other)`: Returns true if all elements are contained in `other`.
- `bool is_superset_of(HashSet<T> other)`: Returns true if all elements of `other` are contained in this set.
- `IIterator<T> iterator()`: Obtains forward iterator implementing `IIterator<T>`.
- `String to_string()`: Returns `"[HashSet]"`.

---

## 2. Class: `solix.collections.TreeSet<T>`

Binary search tree backed ordered distinct set implementation. Implements `ISet<T>`, `IReadOnlyCollection<T>`, `IIterable<T>`, and `IStringable`. Node structures are strictly encapsulated.

### Constructors
- `TreeSet()`: Initializes an empty tree set with natural ordering.
- `TreeSet(int32(*)(T, T) comparator)`: Initializes tree set with explicit element comparison function pointer.

### Methods
- `T first()`: Retrieves the lowest (minimum) element in the set. Throws `NoSuchElementException` if empty.
- `T last()`: Retrieves the highest (maximum) element in the set. Throws `NoSuchElementException` if empty.
- `bool add(T item)`: Inserts element in sorted position if distinct.
- `bool remove(T item)`: Removes element from tree.
- `bool contains(T item)`: Searches for element in O(log N) time.
- `int32 size()`: Returns element count.
- `bool is_empty()`: Returns true if empty.
- `void clear()`: Clears all elements.
- `T[] to_array()`: In-order sorted array representation.
- `List<T> to_list()`: In-order sorted `List<T>`.
- `TreeSetIterator<T> iterator()`: Returns in-order iterator.
- `void for_each(void(*)(T) action)`: In-order traversal executing action.
- `void union_with(TreeSet<T> other)`: Merges elements from `other`.
- `void intersect_with(TreeSet<T> other)`: Retains elements present in both trees.
- `void difference_with(TreeSet<T> other)`: Removes elements present in `other`.
- `bool is_subset_of(TreeSet<T> other)`: Returns true if subset.
- `bool is_superset_of(TreeSet<T> other)`: Returns true if superset.
- `String to_string()`: Returns `"[TreeSet]"`.
