# Solix Standard Library API Reference

This directory contains complete, production-grade documentation for the Solix Standard Library.

The standard library is organized into three primary package hierarchies:

---

## 1. [`solix.core`](core/README.md)
The foundational types, primitive wrappers, string operations, exception classes, and core algorithms.

- [`String`](core/String.md) — Immutable UTF-8 string type
- [`StringBuilder`](core/StringBuilder.md) — Mutable string buffer for dynamic formatting
- [`Exceptions`](core/Exceptions.md) — Built-in exception hierarchy
- [`Optional<T>`](core/Optional.md) — Null-safe monadic value wrapper
- [`Result<T, E>`](core/Result.md) — Functional error/success container
- [`Arrays`](core/Arrays.md) — Static array algorithms (sort, search, fill, copy)
- [`Objects`](core/Objects.md) — Null-safety and universal hashing utilities

---

## 2. [`solix.collections`](collections/README.md)
Dynamic containers and data structures.

- [`List<T>`](collections/List.md) — Dynamic resizing array list
- [`LinkedList<T>`](collections/LinkedList.md) — Doubly-linked list with $O(1)$ head/tail operations
- [`Stack<T>`](collections/Stack.md) — LIFO stack container
- [`Queue<T>`](collections/Queue.md) — FIFO queue container
- [`Deque<T>`](collections/Deque.md) — Double-ended queue
- [`HashMap<K, V>`](collections/HashMap.md) — Bucketed hash table key-value map
- [`ArrayMap<K, V>`](collections/ArrayMap.md) — Compact array-backed map
- [`HashSet<T>`](collections/HashSet.md) — Bucketed unique element set
- [`ArraySet<T>`](collections/ArraySet.md) — Compact array-backed unique element set
- [`Pair<TFirst, TSecond>`](collections/Pair.md) — Generic 2-tuple utility container

---

## 3. [`solix.systems`](systems/README.md)
System, OS, and platform runtime interfaces.

- [`Console`](systems/Console.md) — Terminal input, output, and coloring
