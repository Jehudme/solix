# `Deque<T>`

## 1. Class Overview

`solix.collections.Deque<T>` is a double-ended queue supporting element insertion and removal at both endpoints in amortized $O(1)$ time. It functions as both a FIFO queue and a LIFO stack using an internal circular buffer.

- **Package**: `solix.collections`
- **Import**: `import solix.collections.Deque;` or `import solix.collections.*;`

---

## 2. Constructor Reference

| Constructor | Description |
|-------------|-------------|
| `public Deque()` | Constructs an empty deque with default initial capacity of 16. |
| `public Deque(int32 initial_capacity)` | Constructs an empty deque with the specified initial capacity. |

---

## 3. Method Reference

| Method Signature | Return Type | Description |
|------------------|-------------|-------------|
| `size()` | `int32` | Returns the number of elements in the deque. |
| `is_empty()` | `bool` | Returns `true` if `size() == 0`. |
| `push_front(T item)` | `void` | Inserts an element at the front ($O(1)$ amortized). |
| `push_back(T item)` | `void` | Inserts an element at the back ($O(1)$ amortized). |
| `pop_front()` | `T` | Removes and returns the element at the front. Throws `UnderflowException` if empty. |
| `pop_back()` | `T` | Removes and returns the element at the back. Throws `UnderflowException` if empty. |
| `peek_front()` | `T` | Returns the element at the front without removing it. |
| `peek_back()` | `T` | Returns the element at the back without removing it. |
| `clear()` | `void` | Removes all elements from the deque. |
| `to_array()` | `T[]` | Copies the elements from front to back into a newly allocated array. |

---

## 4. Code Examples

```solix
package solix.example;

import solix.collections.Deque;
import solix.systems.Console;

public class Main {
    public static void main(char[][] args) {
        Deque<int32> d = new Deque<int32>();
        d.push_back(10);
        d.push_front(5);
        d.push_back(20);

        Console.println(d.pop_front()); // 5
        Console.println(d.pop_back());  // 20
        Console.println(d.pop_front()); // 10
    }
}
```
