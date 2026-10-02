# Solix Standard Library: Linear Collections & Buffers (`solix.collections`)

## Overview

The `solix.collections` linear collections module provides specialized, high-performance linear data structures, queues, buffers, and bit vectors:
1. `Stack`: Last-In-First-Out (LIFO) stack container backed by dynamic array.
2. `Queue`: First-In-First-Out (FIFO) queue container backed by doubly linked nodes.
3. `Deque`: Double-ended queue (`IDeque`) supporting $O(1)$ insertion and removal at both extremities.
4. `PriorityQueue`: Binary heap priority container supporting min-heap and max-heap extraction orders with $O(\log N)$ updates.
5. `CircularBuffer`: Fixed-capacity FIFO ring buffer supporting bounded streaming with optional overwrite semantics.
6. `BitSet`: Dynamically expandable bit vector providing dense binary flags, bitwise operations (`and`, `or`, `xor`), and popcount cardinality tracking.

---

## 1. Class: `solix.collections.Stack`

LIFO stack data structure implementing `ICollection`.

### Constructors
- `Stack()`: Initializes an empty stack with default initial capacity.
- `Stack(int32 initial_capacity)`: Initializes an empty stack with specified initial capacity.

### Methods
- `void push(Any item)`: Pushes an element onto the top of the stack.
- `Any pop()`: Removes and returns the element at the top of the stack. Throws `InvalidOperationException` if the stack is empty.
- `Any peek()`: Returns the element at the top of the stack without removing it. Throws `InvalidOperationException` if the stack is empty.
- `int32 size()`: Returns the number of elements in the stack.
- `bool is_empty()`: Returns `true` if the stack contains no elements.
- `bool contains(Any item)`: Returns `true` if the specified element is in the stack.
- `void clear()`: Removes all elements from the stack.
- `Any[] to_array()`: Returns an array containing all elements in insertion order.
- `IIterator iterator()`: Returns an iterator over elements.
- `String to_string()`: Formats the stack as a bracketed list `"[item1, item2, ...]"`.

### Complexity
- **Push**: Amortized $O(1)$.
- **Pop / Peek**: $O(1)$.

---

## 2. Class: `solix.collections.Queue`

FIFO queue data structure implementing `ICollection`.

### Constructors
- `Queue()`: Initializes an empty FIFO queue.

### Methods
- `void enqueue(Any item)`: Inserts an element at the end of the queue.
- `Any dequeue()`: Removes and returns the element at the beginning of the queue. Throws `InvalidOperationException` if the queue is empty.
- `Any peek()`: Returns the element at the beginning of the queue without removing it. Throws `InvalidOperationException` if the queue is empty.
- `int32 size()`: Returns the number of elements in the queue.
- `bool is_empty()`: Returns `true` if the queue is empty.
- `bool contains(Any item)`: Returns `true` if the specified element is in the queue.
- `void clear()`: Removes all elements from the queue.
- `Any[] to_array()`: Returns an array of elements in FIFO order.
- `IIterator iterator()`: Returns an iterator over elements in FIFO order.
- `String to_string()`: Formats the queue as `"[item1, item2, ...]"`.

### Complexity
- **Enqueue / Dequeue / Peek**: Guaranteed $O(1)$.

---

## 3. Class: `solix.collections.Deque`

Double-ended queue implementing `IDeque` and `ICollection`.

### Constructors
- `Deque()`: Initializes an empty double-ended queue.

### Methods
- `void push_front(Any item)` / `void add_first(Any item)`: Inserts an element at the front.
- `void push_back(Any item)` / `void add_last(Any item)`: Inserts an element at the back.
- `Any pop_front()` / `Any remove_first()`: Removes and returns the front element. Throws `InvalidOperationException` if empty.
- `Any pop_back()` / `Any remove_last()`: Removes and returns the back element. Throws `InvalidOperationException` if empty.
- `Any peek_front()` / `Any peek_first()`: Returns the front element without removing it. Throws `InvalidOperationException` if empty.
- `Any peek_back()` / `Any peek_last()`: Returns the back element without removing it. Throws `InvalidOperationException` if empty.
- `int32 size()`, `bool is_empty()`, `void clear()`, `Any[] to_array()`, `IIterator iterator()`.

### Complexity
- **All front/back insertions, removals, and peeks**: Guaranteed $O(1)$.

---

## 4. Class: `solix.collections.PriorityQueue`

Binary heap priority queue implementing `ICollection`.

### Constructors
- `PriorityQueue()`: Initializes an empty min-heap priority queue (lowest value dequeued first).
- `PriorityQueue(bool is_max_heap)`: Initializes an empty priority queue with specified heap mode (`true` for max-heap, `false` for min-heap).

### Methods
- `void enqueue(Any item)`: Inserts an element into the priority queue and sifts it up into place.
- `Any dequeue()`: Removes and returns the highest priority element (root). Throws `InvalidOperationException` if empty.
- `Any peek()`: Returns the highest priority element without removing it. Throws `InvalidOperationException` if empty.
- `int32 size()`, `bool is_empty()`, `void clear()`, `Any[] to_array()`, `IIterator iterator()`.

### Complexity
- **Enqueue**: $O(\log N)$.
- **Dequeue**: $O(\log N)$.
- **Peek**: $O(1)$.

---

## 5. Class: `solix.collections.CircularBuffer`

Fixed-capacity FIFO ring buffer implementing `ICollection`.

### Constructors
- `CircularBuffer(int32 capacity)`: Initializes a circular buffer with specified fixed capacity in non-overwrite mode. Throws `IllegalArgumentException` if capacity $\le 0$.
- `CircularBuffer(int32 capacity, bool overwrite)`: Initializes a circular buffer with specified capacity and overwrite policy.

### Methods
- `int32 capacity()`: Returns the buffer's maximum capacity.
- `int32 size()`: Returns current element count.
- `bool is_full()`: Returns `true` when element count equals capacity.
- `bool is_overwrite()`: Returns `true` if overwrite mode is enabled.
- `bool write(Any item)`: Writes an item. If full and overwrite is false, returns `false`. If full and overwrite is true, advances read head, overwrites oldest element, and returns `true`.
- `void enqueue(Any item)`: Writes an item. Throws `InvalidOperationException` if full and overwrite is false.
- `Any read()` / `Any dequeue()`: Reads and removes the oldest item. Throws `InvalidOperationException` if empty.
- `Any peek()`: Returns oldest item without removing it. Throws `InvalidOperationException` if empty.
- `void clear()`, `Any[] to_array()`, `IIterator iterator()`.

### Complexity
- **Read / Write / Peek**: Guaranteed $O(1)$ without dynamic allocations.

---

## 6. Class: `solix.collections.BitSet`

Dense dynamically-sized bit vector implementing `IStringable`.

### Constructors
- `BitSet()`: Initializes a bit set with initial capacity of 64 bits (all clear).
- `BitSet(int32 nbits)`: Initializes a bit set with capacity for at least `nbits`. Throws `IllegalArgumentException` if `nbits < 0`.

### Methods
- `bool get(int32 bit_index)`: Returns value of the bit at specified index. Throws `IndexOutOfBoundsException` if `bit_index < 0`.
- `void set(int32 bit_index)`: Sets bit at `bit_index` to `true`. Automatically expands capacity if needed.
- `void set_value(int32 bit_index, bool value)`: Sets bit at `bit_index` to specified boolean value.
- `void clear(int32 bit_index)`: Clears bit at `bit_index` (`false`).
- `void clear_all()`: Clears all bits.
- `void flip(int32 bit_index)`: Inverts bit at `bit_index`.
- `void and(BitSet other)`: Performs bitwise logical AND with `other`. Throws `IllegalArgumentException` if `other` is `null`.
- `void or(BitSet other)`: Performs bitwise logical OR with `other`.
- `void xor(BitSet other)`: Performs bitwise logical XOR with `other`.
- `int32 cardinality()`: Returns number of bits set to `true`.
- `int32 length()`: Returns index of highest set bit + 1 (or 0 if empty).
- `int32 size()`: Returns allocated bit capacity.
- `bool is_empty()`: Returns `true` if no bits are set.
- `String to_string()`: Formats set bit indices as `"{0, 5, 63}"`.
