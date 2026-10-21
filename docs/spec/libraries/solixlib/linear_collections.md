# Solix Standard Library: Linear Collections & Buffers (`solix.collections`)

## Overview

The `solix.collections` linear collections module provides specialized, high-performance generic linear data structures, queues, buffers, and bit vectors:
1. `Stack<T>`: Last-In-First-Out (LIFO) stack container backed by dynamic array with `for_each` lambda iteration.
2. `Queue<T>`: First-In-First-Out (FIFO) queue container backed by doubly linked nodes with `for_each` lambda iteration.
3. `Deque<T>`: Double-ended queue supporting $O(1)$ insertion and removal at both extremities with `for_each` lambda iteration.
4. `PriorityQueue<T>`: Binary heap priority container supporting comparator lambdas (`int32(*)(T, T)`) with $O(\log N)$ updates and `for_each`.
5. `CircularBuffer<T>`: Fixed-capacity FIFO ring buffer supporting bounded streaming with optional overwrite semantics, `enqueue`/`dequeue`, and `for_each`.
6. `BitSet`: Dynamically expandable bit vector providing dense binary flags, bitwise operations (`and`, `or`, `xor`), and popcount cardinality tracking.

---

## 1. Class: `solix.collections.Stack<T>`

Compile-time generic LIFO stack data structure.

### Constructors
- `Stack()`: Initializes an empty stack with default initial capacity.
- `Stack(int32 initial_capacity)`: Initializes an empty stack with specified initial capacity.

### Methods
- `void push(T item)`: Pushes an element onto the top of the stack.
- `T pop()`: Removes and returns the element at the top of the stack. Throws `InvalidOperationException` if the stack is empty.
- `T peek()`: Returns the element at the top of the stack without removing it. Throws `InvalidOperationException` if the stack is empty.
- `int32 size()`: Returns the number of elements in the stack.
- `bool is_empty()`: Returns `true` if the stack contains no elements.
- `bool contains(T item)`: Returns `true` if the specified element is in the stack.
- `void clear()`: Removes all elements from the stack.
- `T[] to_array()`: Returns an array containing all elements in insertion order.
- `void for_each(void(*)(T) action)`: Sequentially executes `action(item)` on all elements.
- `StackIterator<T> iterator()`: Returns an iterator over elements.
- `String to_string()`: Returns `"[Stack]"`.

---

## 2. Class: `solix.collections.Queue<T>`

Compile-time generic FIFO queue data structure backed by doubly linked nodes.

### Constructors
- `Queue()`: Initializes an empty FIFO queue.

### Methods
- `void enqueue(T item)`: Inserts an element at the end of the queue.
- `T dequeue()`: Removes and returns the element at the beginning of the queue. Throws `InvalidOperationException` if the queue is empty.
- `T peek()`: Returns the element at the beginning of the queue without removing it. Throws `InvalidOperationException` if the queue is empty.
- `int32 size()`: Returns the number of elements in the queue.
- `bool is_empty()`: Returns `true` if the queue is empty.
- `bool contains(T item)`: Returns `true` if the specified element is in the queue.
- `void clear()`: Removes all elements from the queue.
- `T[] to_array()`: Returns an array of elements in FIFO order.
- `void for_each(void(*)(T) action)`: Sequentially executes `action(item)` on all elements.
- `QueueIterator<T> iterator()`: Returns an iterator over elements in FIFO order.
- `String to_string()`: Returns `"[Queue]"`.

---

## 3. Class: `solix.collections.Deque<T>`

Compile-time generic double-ended queue supporting $O(1)$ operations at both ends.

### Constructors
- `Deque()`: Initializes an empty double-ended queue.

### Methods
- `void push_front(T item)` / `void add_first(T item)`: Inserts an element at the front.
- `void push_back(T item)` / `void add_last(T item)`: Inserts an element at the back.
- `T pop_front()` / `T remove_first()`: Removes and returns the front element. Throws `InvalidOperationException` if empty.
- `T pop_back()` / `T remove_last()`: Removes and returns the back element. Throws `InvalidOperationException` if empty.
- `T peek_front()` / `T peek_first()`: Returns the front element without removing it. Throws `InvalidOperationException` if empty.
- `T peek_back()` / `T peek_last()`: Returns the back element without removing it. Throws `InvalidOperationException` if empty.
- `int32 size()`, `bool is_empty()`, `void clear()`, `T[] to_array()`.
- `void for_each(void(*)(T) action)`: Sequentially executes `action(item)` on all elements.
- `DequeIterator<T> iterator()`: Returns iterator over deque elements.
- `String to_string()`: Returns `"[Deque]"`.

---

## 4. Class: `solix.collections.PriorityQueue<T>`

Compile-time generic binary heap priority queue parameterized by a comparator lambda.

### Constructors
- `PriorityQueue(int32(*)(T, T) comparator)`: Initializes an empty priority queue with the specified comparator lambda.

### Methods
- `void enqueue(T item)`: Inserts an element into the priority queue and sifts it up into place.
- `T dequeue()`: Removes and returns the highest priority element (root). Throws `InvalidOperationException` if empty.
- `T peek()`: Returns the highest priority element without removing it. Throws `InvalidOperationException` if empty.
- `int32 size()`, `bool is_empty()`, `void clear()`, `T[] to_array()`.
- `void for_each(void(*)(T) action)`: Sequentially executes `action(item)` across heap elements.
- `PriorityQueueIterator<T> iterator()`: Returns iterator over heap elements.
- `String to_string()`: Returns `"[PriorityQueue]"`.

---

## 5. Class: `solix.collections.CircularBuffer<T>`

Compile-time generic fixed-capacity circular ring buffer with optional overwrite support.

### Constructors
- `CircularBuffer(int32 capacity)`: Initializes a non-overwrite ring buffer with specified capacity.
- `CircularBuffer(int32 capacity, bool overwrite)`: Initializes a ring buffer with specified capacity and overwrite policy.

### Methods
- `int32 capacity()`: Returns total buffer capacity.
- `int32 size()`: Returns current stored element count.
- `bool is_empty()`, `bool is_full()`, `bool is_overwrite()`.
- `bool write(T item)`: Writes item to buffer. In non-overwrite mode, returns `false` if full. In overwrite mode, drops oldest element and returns `true`.
- `void enqueue(T item)`: Writes item to buffer; throws `InvalidOperationException` if full in non-overwrite mode.
- `T read()` / `T dequeue()`: Reads and removes the oldest element. Throws `InvalidOperationException` if empty.
- `T peek()`: Reads the oldest element without removal. Throws `InvalidOperationException` if empty.
- `T get(int32 index)`: Direct access relative to head. Throws `InvalidOperationException` if out of bounds.
- `void clear()`, `T[] to_array()`.
- `void for_each(void(*)(T) action)`: Sequentially executes `action(item)` on all elements from head to tail.
- `CircularBufferIterator<T> iterator()`.
- `String to_string()`: Returns `"[CircularBuffer]"`.

---

## 6. Class: `solix.collections.BitSet`

Dynamically expandable bit vector with bitwise operators.

### Constructors
- `BitSet()`: Initializes a 64-bit vector.
- `BitSet(int32 nbits)`: Initializes a vector with at least `nbits` capacity.

### Methods
- `void set(int32 bit_index)`: Sets bit at `bit_index` to `true`.
- `void set_value(int32 bit_index, bool value)`: Sets bit at `bit_index` to `value`.
- `bool get(int32 bit_index)`: Returns bit state at `bit_index`.
- `void clear_bit(int32 bit_index)`: Clears bit at `bit_index`.
- `void flip(int32 bit_index)`: Inverts bit at `bit_index`.
- `void clear()`: Resets all bits to `false`.
- `int32 cardinality()`: Returns the number of bits set to `true`.
- `int32 length()`: Returns the logical size (index of highest set bit + 1).
- `bool is_empty()`: Returns `true` if no bits are set.
- `void and(BitSet other)`, `void or(BitSet other)`, `void xor(BitSet other)`, `void and_not(BitSet other)`.
