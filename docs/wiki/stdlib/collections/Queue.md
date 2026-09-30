# `Queue<T>`

## 1. Class Overview

`solix.collections.Queue<T>` represents a First-In-First-Out (FIFO) queue of objects. Elements are inserted at the back and removed from the front, backed by a circular array buffer for $O(1)$ enqueue and dequeue operations.

- **Package**: `solix.collections`
- **Import**: `import solix.collections.Queue;` or `import solix.collections.*;`

---

## 2. Constructor Reference

| Constructor | Description |
|-------------|-------------|
| `public Queue()` | Creates an empty queue with default circular capacity of 16. |
| `public Queue(int32 initial_capacity)` | Creates an empty queue with the specified initial capacity. |

---

## 3. Method Reference

| Method Signature | Return Type | Description |
|------------------|-------------|-------------|
| `size()` | `int32` | Returns the number of elements in the queue. |
| `is_empty()` | `bool` | Returns `true` if the queue is empty. |
| `enqueue(T item)` | `void` | Inserts an item at the back of the queue ($O(1)$). |
| `dequeue()` | `T` | Removes and returns the item at the front of the queue ($O(1)$). Throws `UnderflowException` if empty. |
| `peek()` | `T` | Returns the front item without removing it. Throws `UnderflowException` if empty. |
| `clear()` | `void` | Clears all elements from the queue. |
| `to_array()` | `T[]` | Copies the elements from front to back into a newly allocated raw array. |

---

## 4. Code Examples

```solix
package solix.example;

import solix.collections.Queue;
import solix.systems.Console;

public class Main {
    public static void main(char[][] args) {
        Queue<string> q = new Queue<string>();
        q.enqueue("Customer 1");
        q.enqueue("Customer 2");
        q.enqueue("Customer 3");

        while (!q.is_empty()) {
            Console.println(q.dequeue()); // Prints Customer 1, 2, 3
        }
    }
}
```
