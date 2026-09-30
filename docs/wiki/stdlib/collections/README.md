# `solix.collections` Package

The `solix.collections` package provides standard linear, associative, and utility data structures for the Solix programming language.

## Data Structures

### Linear Collections
- [`List<T>`](List.md) — Dynamic resizing array container ($O(1)$ random access)
- [`LinkedList<T>`](LinkedList.md) — Doubly-linked list with $O(1)$ insertion at head and tail
- [`Stack<T>`](Stack.md) — Last-In-First-Out (LIFO) stack container
- [`Queue<T>`](Queue.md) — First-In-First-Out (FIFO) queue container
- [`Deque<T>`](Deque.md) — Double-ended queue container

### Associative Collections & Sets
- [`HashMap<K, V>`](HashMap.md) — Bucketed hash table key-value map
- [`ArrayMap<K, V>`](ArrayMap.md) — Compact array-backed key-value map for small sizes
- [`HashSet<T>`](HashSet.md) — Bucketed hash table set of unique elements
- [`ArraySet<T>`](ArraySet.md) — Array-backed unique element set

### Utilities
- [`Pair<TFirst, TSecond>`](Pair.md) — Generic 2-tuple utility container
