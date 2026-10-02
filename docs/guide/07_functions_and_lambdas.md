# 07 - Functions, Function Pointers & Lambdas

Functions are central to programming in Solix. Beyond standard class methods, Solix features modern functional programming capabilities including C-compatible function pointers, first-class lambdas, and automatic reference counted (ARC) closures.

---

## 1. Static Methods as Functions

Static methods belong to the class namespace and do not require an instance receiver (`this`):

```solix
public class MathUtils {
    public static int32 add(int32 a, int32 b) {
        return a + b;
    }

    public static int32 factorial(int32 n) {
        if (n <= 1) return 1;
        return n * factorial(n - 1);
    }
}
```

You invoke them using class name qualification:
```solix
int32 sum = MathUtils.add(10, 20);
```

---

## 2. Function Pointers

A function pointer is a variable that stores the code address of a function. Function pointers in Solix have zero runtime heap overhead and execute with direct CPU call efficiency.

### Declaring Function Pointer Types
Function pointer types declare their return type and parameter types:
```solix
<ReturnType>(*)(<ParamType1>, <ParamType2>, ...)
```

For example:
```solix
// Takes an int32, returns an int32
int32(*)(int32) op;

// Takes two int32 values, returns an int32
int32(*)(int32, int32) binaryOp;

// Takes no parameters, returns void
void(*)() callback;
```

### Assigning and Invoking Function Pointers
Assign any compatible static method:
```solix
public class Transformer {
    public static int32 square(int32 x) {
        return x * x;
    }
}

public class Main {
    public static int32 main() {
        int32(*)(int32) fn = Transformer.square;
        int32 result = fn(7); // Invokes Transformer.square(7) -> 49
        return result == 49 ? 0 : 1;
    }
}
```

### Passing Function Pointers as Parameters (Higher-Order Functions)
```solix
public class Algorithm {
    public static int32 apply(int32 value, int32(*)(int32) transform) {
        return transform(value);
    }
}

// In main():
int32 res = Algorithm.apply(5, Transformer.square); // 25
```

---

## 3. Lambda Expressions

Lambdas provide concise syntax for inline anonymous functions.

### Expression Lambdas
When a lambda consists of a single expression, the arrow `=>` returns that expression directly:
```solix
int32(*)(int32) square = (x) => x * x;
int32(*)(int32, int32) add = (a, b) => a + b;

int32 v = square(6); // 36
```

### Block Lambdas
For multi-line logic, use block braces `{ ... }`:
```solix
int32(*)(int32, int32) max = (a, b) => {
    if (a > b) {
        return a;
    }
    return b;
};
```

---

## 4. Closures & Variable Captures

When a lambda references local variables from outside its body, it becomes a **closure**:

```solix
public class Main {
    public static int32 main() {
        int32 factor = 10;
        int32(*)(int32) multiplier = (x) => x * factor;

        return multiplier(5); // 50
    }
}
```

### Capture Semantics & ARC Safety
- **Environment Allocation**: The Solix compiler automatically creates an internal capture frame on the heap.
- **Reference Counting**: If a captured variable is a reference type (an object or array), Solix increments its reference count (`INC_REF`).
- **Memory Safety**: When the closure variable goes out of scope, the ARC runtime frees the capture frame and releases captured reference types (`DEC_REF_CALLABLE`), preventing dangling pointers and memory leaks.

---

## 5. Storing Functions and Lambdas in Classes

Function pointers and closures can be stored in class fields:

```solix
public class TaskRunner {
    public int32(*)(int32) task;

    public TaskRunner(int32(*)(int32) task) {
        this.task = task;
    }

    public int32 run(int32 input) {
        return this.task(input);
    }
}

public class Main {
    public static int32 main() {
        TaskRunner runner = new TaskRunner((n) => n * 3);
        return runner.run(10); // 30
    }
}
```

---

## 6. Best Practices

1. **Use pure function pointers for maximum performance**: Static method references and non-capturing lambdas require zero heap allocations.
2. **Beware of circular references in closures**: If a class instance holds a closure field that captures `this`, use weak references or clear the closure when done to prevent reference cycles.
3. **Type aliases for readable signatures**: Use `alias` to give complex function pointer types clean names:
   ```solix
   alias IntPredicate = bool(*)(int32);
   ```
