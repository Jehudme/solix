# `LinkedList<T>`

## 1. Class Overview

`solix.collections.LinkedList<T>` is a doubly-linked list collection providing $O(1)$ insertion and deletion at both the head and tail. Unlike `List<T>`, elements are not stored contiguously, making `LinkedList<T>` ideal for FIFO queues, LIFO stacks, or workloads where frequent insertions and removals occur at container extremities.

- **Package**: `solix.collections`
- **Import**: `import solix.collections.LinkedList;` or `import solix.collections.*;`

---

## 2. Constructor Reference

| Constructor | Description |
|-------------|-------------|
| `public LinkedList()` | Constructs an empty doubly-linked list. |
| `public LinkedList(T[] initial_elements)` | Populates a new linked list with elements from a raw array. |

---

## 3. Method Reference

| Method Signature | Return Type | Description |
|------------------|-------------|-------------|
| `size()` | `int32` | Returns the number of nodes in the linked list. |
| `is_empty()` | `bool` | Returns `true` if `size() == 0`. |
| `clear()` | `void` | Deallocates all nodes and resets head/tail to `null`. |
| `add_first(T item)` | `void` | Prepends `item` to the head of the list ($O(1)$). |
| `add_last(T item)` | `void` | Appends `item` to the tail of the list ($O(1)$). |
| `add(T item)` | `void` | Alias for `add_last(item)`. |
| `get_first()` | `T` | Returns the first element without removing it. |
| `get_last()` | `T` | Returns the last element without removing it. |
| `remove_first()` | `T` | Removes and returns the first element ($O(1)$). |
| `remove_last()` | `T` | Removes and returns the last element ($O(1)$). |
| `contains(T item)` | `bool` | Returns `true` if `item` exists in the list ($O(N)$). |
| `to_array()` | `T[]` | Copies all elements sequentially into a newly allocated raw array. |

---

## 4. Code Examples

```solix
package solix.example;

import solix.collections.LinkedList;
import solix.systems.Console;

public class Main {
    public static void main(char[][] args) {
        LinkedList<string> deque = new LinkedList<string>();
        deque.add_last("middle");
        deque.add_first("front");
        deque.add_last("back");

        Console.println(deque.remove_first()); // "front"
        Console.println(deque.remove_last());  // "back"
        Console.println(deque.get_first());    // "middle"
    }
}
```
