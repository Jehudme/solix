# Control Flow

Solix provides a complete set of structured control-flow statements covering branching, looping, and early exits. This guide covers each construct with realistic examples.

---

## Table of Contents

1. [if / else](#if--else)
2. [while Loop](#while-loop)
3. [do-while Loop](#do-while-loop)
4. [for Loop](#for-loop)
5. [switch / case / default / break](#switch--case--default--break)
6. [break](#break)
7. [continue](#continue)
8. [return](#return)

---

## if / else

The `if` statement evaluates a `bool` condition and executes the body if the condition is `true`. An optional `else` branch executes when the condition is `false`. Multiple branches can be chained with `else if`.

```solix
import solix.systems.Console;

public void classify_score(int32 score) {
    if (score >= 90) {
        Console.println("Grade: A");
    } else if (score >= 80) {
        Console.println("Grade: B");
    } else if (score >= 70) {
        Console.println("Grade: C");
    } else if (score >= 60) {
        Console.println("Grade: D");
    } else {
        Console.println("Grade: F");
    }
}
```

Conditions must be of type `bool`; there is no implicit conversion from integers or pointers.

```solix
int32 x = 5;
if (x)         { }   // Compile error — int32 is not bool
if (x != 0)   { }   // Correct
```

Braces are required even for single-statement bodies.

---

## while Loop

A `while` loop evaluates its condition **before** each iteration. The loop body executes zero or more times.

```solix
import solix.systems.Console;

public void count_down(int32 from) {
    int32 n = from;
    while (n > 0) {
        Console.println(n);
        n--;
    }
    Console.println("Lift off!");
}
```

**Practical example — read until a sentinel value:**

```solix
import solix.systems.Console;

public int32 sum_until_zero() {
    int32 total = 0;
    int32 input = Console.input_int32();
    while (input != 0) {
        total = total + input;
        input = Console.input_int32();
    }
    return total;
}
```

---

## do-while Loop

A `do-while` loop evaluates its condition **after** each iteration, guaranteeing the body runs **at least once**.

```solix
import solix.systems.Console;

public int32 read_positive() {
    int32 value;
    do {
        Console.print("Enter a positive number: ");
        value = Console.input_int32();
    } while (value <= 0);
    return value;
}
```

The `while` condition is checked after the body, so the prompt is always shown at least once.

---

## for Loop

The `for` loop has three clauses separated by semicolons:

```
for (<init>; <condition>; <step>) {
    <body>
}
```

- **`<init>`** — executed once before the loop begins; typically declares a loop variable.
- **`<condition>`** — a `bool` expression evaluated before each iteration.
- **`<step>`** — executed after each iteration; typically increments or decrements the loop variable.

```solix
import solix.systems.Console;

public void print_squares(int32 n) {
    for (int32 i = 1; i <= n; i++) {
        Console.print(i * i);
        Console.print(" ");
    }
    Console.println();
}
```

**Iterating an array:**

```solix
int32[] data = {10, 20, 30, 40, 50};
int32 sum = 0;
for (int32 i = 0; i < data.length; i++) {
    sum = sum + data[i];
}
Console.println(sum);   // 150
```

**Nested loops — matrix fill:**

```solix
int32[][] grid = new int32[][3];
for (int32 row = 0; row < 3; row++) {
    grid[row] = new int32[3];
    for (int32 col = 0; col < 3; col++) {
        grid[row][col] = row * 3 + col;
    }
}
```

---

## switch / case / default / break

The `switch` statement dispatches on an integer or enumeration value and jumps to the matching `case` label. **Fall-through does not occur automatically** — each case must explicitly use `break` to exit the switch, or execution continues into the next case.

```solix
import solix.systems.Console;

public void print_day(int32 day) {
    switch (day) {
        case 0:
            Console.println("Sunday");
            break;
        case 1:
            Console.println("Monday");
            break;
        case 2:
            Console.println("Tuesday");
            break;
        case 3:
            Console.println("Wednesday");
            break;
        case 4:
            Console.println("Thursday");
            break;
        case 5:
            Console.println("Friday");
            break;
        case 6:
            Console.println("Saturday");
            break;
        default:
            Console.println("Unknown day");
            break;
    }
}
```

**Intentional fall-through** is achieved by omitting `break` between cases:

```solix
public bool is_weekday(int32 day) {
    switch (day) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            return true;
        default:
            return false;
    }
}
```

> [!IMPORTANT]
> A `switch` statement in Solix operates on `int32` and enum types. Switching on `String` or floating-point values is not supported.

---

## break

`break` immediately exits the **nearest enclosing** `while`, `do-while`, `for`, or `switch` statement.

```solix
import solix.systems.Console;

public int32 find_first(int32[] arr, int32 target) {
    int32 result = -1;
    for (int32 i = 0; i < arr.length; i++) {
        if (arr[i] == target) {
            result = i;
            break;   // Stop searching once found
        }
    }
    return result;
}
```

`break` only exits one level. To exit nested loops, use a flag variable or restructure as a method with an early `return`.

---

## continue

`continue` skips the remainder of the current loop body and proceeds to the **next iteration** (re-evaluating the condition).

```solix
import solix.systems.Console;

public void print_odd(int32 limit) {
    for (int32 i = 1; i <= limit; i++) {
        if (i % 2 == 0) {
            continue;   // Skip even numbers
        }
        Console.println(i);
    }
}
```

In a `for` loop, `continue` causes the **step expression** to execute before re-checking the condition.

---

## return

`return` exits the current method and optionally provides a return value. For methods declared `void`, `return` may appear with no argument or be omitted entirely (the method exits at its closing brace).

```solix
public int32 factorial(int32 n) {
    if (n <= 1) {
        return 1;              // Early exit with base case
    }
    return n * factorial(n - 1);
}

public void greet(String name) {
    if (name == null) {
        return;                // Early exit — no return value for void
    }
    Console.print("Hello, ");
    Console.println(name);
}
```

> [!IMPORTANT]
> Every non-`void` code path in a method must end with a `return` statement that provides a value of the declared return type. The compiler enforces this.
