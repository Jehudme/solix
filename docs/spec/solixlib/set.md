# Solix Standard Library: Sets & Distinct Containers (`solix.collections`)

## Overview

The `solix.collections` set module provides mathematical set abstractions guaranteeing distinct, duplicate-free elements:
1. `ISet`: Generic set contract extending `ICollection`, declaring membership checks, distinct additions, element removals, and set algebra operations (`union_with`, `intersect_with`, `difference_with`, `is_subset_of`, `is_superset_of`).
2. `HashSet`: High-performance hash table backed set providing average $O(1)$ insertions, lookups, and deletions.
3. `TreeSet`: Self-balancing binary search tree backed set maintaining elements in strict ascending order, supporting boundary element queries (`first()`, `last()`) and ordered iteration in $O(\log N)$ time.

Both implementations implement `ISet`, `ICollection`, `IIterable`, and `IStringable`, supporting canonical string formatting (`"{item1, item2, item3}"`) and direct output through `Console.println()`.

---

## 1. Interface: `solix.collections.ISet`

Contract for set collections. Extends `ICollection`.

### Methods
- `bool add(Any item)`: Adds the specified element to this set if it is not already present. Returns `true` if the element was added, or `false` if the element already existed.
- `bool remove(Any item)`: Removes the specified element from this set if present. Returns `true` if removed, or `false` if not found.
- `void union_with(ICollection other)`: Modifies the current set to contain all elements present in this set, in the specified collection, or in both.
- `void intersect_with(ICollection other)`: Modifies the current set to contain only elements that are present in both this set and the specified collection.
- `void difference_with(ICollection other)`: Removes all elements in the specified collection from the current set.
- `bool is_subset_of(ICollection other)`: Returns `true` if every element of this set is present in the specified collection.
- `bool is_superset_of(ICollection other)`: Returns `true` if this set contains all elements of the specified collection.

---

## 2. Class: `solix.collections.HashSet`

Hash-table backed implementation of `ISet`.

### Constructors
- `HashSet()`: Initializes an empty hash set with default initial capacity (16).
- `HashSet(int32 initial_capacity)`: Initializes an empty hash set with specified initial bucket capacity.

### Complexity & Characteristics
- **Lookup & Insertion**: Average $O(1)$ for `add()`, `remove()`, and `contains()`.
- **Iteration Order**: Unspecified; dependent on bucket hash distribution.
- **Set Algebra**:
  - `union_with(other)`: $O(M)$ where $M$ is the size of `other`.
  - `intersect_with(other)`: $O(N)$ where $N$ is the size of `this`.
  - `difference_with(other)`: $O(M)$ where $M$ is the size of `other`.

---

## 3. Class: `solix.collections.TreeSet`

Red-Black binary search tree backed implementation of `ISet`.

### Constructors
- `TreeSet()`: Initializes an empty tree set maintaining elements in natural ascending order.

### Methods
- `Any first()`: Retrieves the lowest (minimum) element in the set. Throws `NoSuchElementException` if the set is empty.
- `Any last()`: Retrieves the highest (maximum) element in the set. Throws `NoSuchElementException` if the set is empty.

### Complexity & Characteristics
- **Lookup & Insertion**: Guaranteed $O(\log N)$ for `add()`, `remove()`, and `contains()`.
- **Iteration Order**: Ascending sorted order.
- **String Rendering**: Formats elements in strictly sorted order: `"{10, 20, 30, 40, 50}"`.
