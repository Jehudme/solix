# `Arrays`

## 1. Class Overview

`solix.core.Arrays` provides static utility algorithms for manipulating primitive arrays and generic reference arrays. It includes operations for filling array contents, copying contiguous slices between buffers, sorting elements in ascending order, binary searching across sorted arrays, reversing elements in-place, and comparing arrays for element-wise equality.

- **Package**: `solix.core`
- **Import**: `import solix.core.Arrays;` or `import solix.core.*;`

---

## 2. Method Reference

### Filling Operations

| Method Signature | Description |
|------------------|-------------|
| `public static void fill(int32[] target_array, int32 fill_value)` | Fills all slots in the integer array with `fill_value`. |
| `public static void fill(int64[] target_array, int64 fill_value)` | Fills all slots in the 64-bit integer array. |
| `public static void fill(char[] target_array, char fill_value)` | Fills all slots in the character array. |
| `public static void fill(bool[] target_array, bool fill_value)` | Fills all slots in the boolean array. |
| `public static void fill(float64[] target_array, float64 fill_value)` | Fills all slots in the 64-bit float array. |
| `public static void fill_i32(int32[] target_array, int32 fill_value)` | Suffix-variant alias for `fill(int32[], int32)`. |
| `public static void fill_char(char[] target_array, char fill_value)` | Suffix-variant alias for `fill(char[], char)`. |
| `public static void fill_bool(bool[] target_array, bool fill_value)` | Suffix-variant alias for `fill(bool[], bool)`. |
| `public static void fill_f64(float64[] target_array, float64 fill_value)` | Suffix-variant alias for `fill(float64[], float64)`. |

### Copy Operations

| Method Signature | Description |
|------------------|-------------|
| `public static void copy<T>(T[] src, int32 src_off, T[] dst, int32 dst_off, int32 len)` | Generic bounds-checked copy of `len` elements from `src` to `dst`. |
| `public static void copy_i32(int32[] src, int32 src_off, int32[] dst, int32 dst_off, int32 len)` | Type-specific copy for `int32[]`. |
| `public static void copy_char(char[] src, int32 src_off, char[] dst, int32 dst_off, int32 len)` | Type-specific copy for `char[]`. |

### Sorting & Searching

| Method Signature | Description |
|------------------|-------------|
| `public static void sort(int32[] target_array)` | Sorts an integer array in ascending order using in-place Quicksort ($O(N \log N)$ average). |
| `public static void sort(int64[] target_array)` | Sorts an `int64[]` array in ascending order. |
| `public static void sort(float64[] target_array)` | Sorts a `float64[]` array in ascending order. |
| `public static void sort(char[] target_array)` | Sorts a `char[]` array in ascending order. |
| `public static void sort_i32(int32[] target_array)` | Suffix-variant alias for `sort(int32[])`. |
| `public static int32 binary_search(int32[] sorted_array, int32 key)` | Binary search returning index if found, or `-1` if absent ($O(\log N)$). |
| `public static int32 binary_search(int64[] sorted_array, int64 key)` | Binary search on `int64[]`. |
| `public static int32 binary_search(float64[] sorted_array, float64 key)` | Binary search on `float64[]`. |
| `public static int32 binary_search(char[] sorted_array, char key)` | Binary search on `char[]`. |
| `public static int32 binary_search_i32(int32[] sorted_array, int32 key)` | Suffix-variant alias for `binary_search(int32[], int32)`. |

### Utility Operations

| Method Signature | Description |
|------------------|-------------|
| `public static void swap<T>(T[] array, int32 i, int32 j)` | Swaps elements at index `i` and `j` in a generic array. |
| `public static void reverse<T>(T[] array)` | In-place reversal of all elements in the generic array. |
| `public static bool equals<T>(T[] first, T[] second)` | Tests if two arrays have equal length and matching elements via `Objects.equals`. |

---

## 3. Code Examples

```solix
package solix.example;

import solix.core.Arrays;
import solix.systems.Console;

public class Main {
    public static void main(char[][] args) {
        int32[] nums = new int32[5];
        Arrays.fill(nums, 42);

        int32[] values = new int32[6];
        values[0] = 50; values[1] = 10; values[2] = 40;
        values[3] = 20; values[4] = 60; values[5] = 30;

        Arrays.sort(values);
        int32 idx = Arrays.binary_search(values, 40);
        Console.println(idx); // Prints: 3
    }
}
```
