# `List<T>`

## 1. Class Overview

`solix.collections.List<T>` is a dynamically resizing array-backed sequential container. It provides $O(1)$ amortized append operations, random access by 0-based index ($O(1)$), bounds checking on element access, and dynamic array reallocation when capacity is exceeded.

- **Package**: `solix.collections`
- **Import**: `import solix.collections.List;` or `import solix.collections.*;`

---

## 2. Constructor Reference

| Constructor | Description |
|-------------|-------------|
| `public List()` | Initializes an empty list with a default capacity of 16. |
| `public List(int32 initial_capacity)` | Initializes an empty list with the specified initial capacity. |
| `public List(T[] initial_elements)` | Initializes the list by copying elements from an existing raw array. |

---

## 3. Method Reference

| Method Signature | Return Type | Description |
|------------------|-------------|-------------|
| `size()` | `int32` | Returns the number of elements currently stored in the list. |
| `capacity()` | `int32` | Returns the allocated buffer capacity. |
| `is_empty()` | `bool` | Returns `true` if the list contains 0 elements. |
| `clear()` | `void` | Removes all elements and resets size to 0. |
| `reserve(int32 min_capacity)` | `void` | Expands capacity if less than `min_capacity`. |
| `get(int32 index)` | `T` | Returns the element at `index`. Throws `IndexOutOfBoundsException` if out of bounds. |
| `set(int32 index, T value)` | `void` | Overwrites the element at `index`. Throws `IndexOutOfBoundsException` if out of bounds. |
| `first()` | `T` | Returns the element at index 0. |
| `last()` | `T` | Returns the element at `size() - 1`. |
| `add(T item)` | `void` | Appends `item` to the end of the list ($O(1)$ amortized). |
| `insert(int32 index, T item)` | `void` | Shifts subsequent elements and inserts `item` at `index` ($O(N)$). |
| `add_all(List<T> items)` | `void` | Appends all elements from another list. |
| `remove_at(int32 index)` | `T` | Removes and returns the element at `index`, shifting subsequent elements left ($O(N)$). |
| `pop()` | `T` | Removes and returns the last element ($O(1)$). |
| `remove(T item)` | `bool` | Finds and removes the first occurrence of `item`. Returns `true` if found. |
| `index_of(T item)` | `int32` | Returns the index of first occurrence of `item`, or `-1` if not found. |
| `last_index_of(T item)` | `int32` | Returns the index of last occurrence of `item`, or `-1`. |
| `contains(T item)` | `bool` | Returns `true` if `item` exists in the list. |
| `reverse()` | `void` | In-place reversal of the list elements. |
| `slice(int32 start, int32 end)` | `List<T>` | Returns a new sublist slice from `start` to `end` (exclusive). |
| `to_array()` | `T[]` | Copies the elements into a newly allocated raw array. |
| `clone()` | `List<T>` | Creates a shallow clone of the list. |

---

## 4. Code Examples

```solix
package solix.example;

import solix.collections.List;
import solix.systems.Console;

public class Main {
    public static void main(char[][] args) {
        List<int32> numbers = new List<int32>();
        numbers.add(10);
        numbers.add(20);
        numbers.add(30);

        for (int32 i = 0; i < numbers.size(); i++) {
            Console.println(numbers.get(i));
        }

        numbers.remove_at(1); // Removes 20
        Console.println("Remaining size: " + numbers.size());
    }
}
```
