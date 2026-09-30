# `Stack<T>`

## 1. Class Overview

`solix.collections.Stack<T>` represents a Last-In-First-Out (LIFO) stack of objects. It provides standard push, pop, and peek operations backed by a dynamic resizing array buffer.

- **Package**: `solix.collections`
- **Import**: `import solix.collections.Stack;` or `import solix.collections.*;`

---

## 2. Constructor Reference

| Constructor | Description |
|-------------|-------------|
| `public Stack()` | Creates an empty stack with default capacity of 16. |
| `public Stack(int32 initial_capacity)` | Creates an empty stack with the specified initial capacity. |

---

## 3. Method Reference

| Method Signature | Return Type | Description |
|------------------|-------------|-------------|
| `size()` | `int32` | Returns the number of items currently on the stack. |
| `is_empty()` | `bool` | Returns `true` if the stack contains no elements. |
| `push(T item)` | `void` | Pushes an item onto the top of the stack ($O(1)$ amortized). |
| `pop()` | `T` | Removes and returns the top item. Throws `UnderflowException` if empty. |
| `peek()` | `T` | Looks at the top item without removing it. Throws `UnderflowException` if empty. |
| `clear()` | `void` | Clears all items from the stack. |
| `to_array()` | `T[]` | Copies the stack elements from bottom to top into a raw array. |

---

## 4. Code Examples

```solix
package solix.example;

import solix.collections.Stack;
import solix.systems.Console;

public class Main {
    public static void main(char[][] args) {
        Stack<int32> s = new Stack<int32>();
        s.push(1);
        s.push(2);
        s.push(3);

        while (!s.is_empty()) {
            Console.println(s.pop()); // Prints 3, 2, 1
        }
    }
}
```
