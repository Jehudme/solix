# Error Handling

Solix handles exceptional conditions through a structured **try-catch-finally** mechanism backed by a hierarchy of exception classes. This guide covers the exception class tree, try-catch-finally semantics, multiple catch blocks, re-throwing, custom exceptions, and how exceptions propagate up the call stack.

---

## Table of Contents

1. [Exception Class Hierarchy](#exception-class-hierarchy)
2. [The `throw` Statement](#the-throw-statement)
3. [try-catch-finally](#try-catch-finally)
4. [Multiple catch Blocks](#multiple-catch-blocks)
5. [Re-Throwing Exceptions](#re-throwing-exceptions)
6. [Custom Exception Classes](#custom-exception-classes)
7. [The `finally` Guarantee](#the-finally-guarantee)
8. [Exception Propagation](#exception-propagation)

---

## Exception Class Hierarchy

All exception types in Solix extend from the base `Exception` class in `solix.core`. The full hierarchy is:

```
Exception
├── RuntimeException
│   ├── NullPointerException
│   ├── IndexOutOfBoundsException
│   ├── TypeCastException
│   ├── InvalidArgumentException
│   ├── IllegalStateException
│   ├── NoSuchElementException
│   ├── EmptyCollectionException
│   ├── KeyNotFoundException
│   ├── DuplicateKeyException
│   ├── UnsupportedOperationException
│   └── ArithmeticException
│       ├── DivideByZeroException
│       ├── OverflowException
│       └── UnderflowException
│   └── FormatException
└── IOException
```

Every exception carries three fields (accessible via virtual getters):

| Field | Getter | Description |
|-------|--------|-------------|
| `detailed_message` | `get_message()` | Human-readable description |
| `underlying_cause` | `get_cause()` | The exception that caused this one, or `null` |
| `error_code` | `get_error_code()` | Optional numeric error code |

Additionally, `print_stack_trace()` writes the exception and its cause chain to `stderr`.

---

## The `throw` Statement

`throw` raises an exception. The expression must evaluate to a reference type that extends `Exception`.

```solix
import solix.core.Exceptions;

public void divide(int32 a, int32 b) {
    if (b == 0) {
        throw new DivideByZeroException("Cannot divide by zero.");
    }
    Console.println(a / b);
}
```

After `throw`, execution of the current function ceases immediately. Control transfers to the nearest enclosing `catch` block that matches the exception type, or propagates up the call stack if no handler exists.

---

## try-catch-finally

The `try` block wraps code that may throw. A `catch` block handles a specific exception type. A `finally` block always executes, regardless of whether an exception was thrown or caught.

```solix
import solix.core.Exceptions;
import solix.systems.Console;

public void safe_parse(String input) {
    try {
        int32 value = input.to_int32();
        Console.println(value);
    } catch (FormatException ex) {
        Console.error("Invalid number format: ");
        Console.errorln(ex.get_message());
    } finally {
        Console.println("Parsing attempt complete.");
    }
}
```

**Execution flow:**

1. Statements in `try` execute sequentially.
2. If a statement throws and a matching `catch` exists → `catch` body runs.
3. If no matching `catch` exists → exception propagates to the caller.
4. The `finally` block always runs: after the `try` completes normally, after a matched `catch` runs, and before a re-throw or propagation exits the frame.

---

## Multiple catch Blocks

A single `try` block can have multiple `catch` clauses. The runtime matches the thrown exception against each handler **in order** using the vtable hierarchy. The **first** matching handler wins.

```solix
import solix.core.Exceptions;
import solix.systems.Console;

public void read_data(int32[] arr, int32 index) {
    try {
        int32 value = arr[index];
        Console.println(value);
    } catch (NullPointerException ex) {
        Console.errorln("Array reference was null.");
    } catch (IndexOutOfBoundsException ex) {
        Console.error("Index out of range: ");
        Console.errorln(ex.get_message());
    } catch (RuntimeException ex) {
        Console.error("Unexpected runtime error: ");
        Console.errorln(ex.get_message());
    } catch (Exception ex) {
        Console.error("Unknown error: ");
        Console.errorln(ex.get_message());
    }
}
```

> [!IMPORTANT]
> Order matters. Always place more specific exception types **before** broader ones. If `RuntimeException` appeared before `NullPointerException`, the null-pointer case would be caught by `RuntimeException` and the specific handler would be unreachable.

---

## Re-Throwing Exceptions

To catch an exception for partial handling (e.g., logging) and then let it continue propagating, use `throw` with the caught exception variable:

```solix
import solix.core.Exceptions;
import solix.systems.Console;

public void process_file(String path) {
    try {
        open_and_process(path);
    } catch (IOException ex) {
        Console.errorln("IO failure — logging and re-throwing.");
        ex.print_stack_trace();
        throw ex;   // Re-throw the original exception
    }
}
```

You can also throw a **new** exception that wraps the original as its cause:

```solix
} catch (IOException ex) {
    throw new RuntimeException("File processing failed.", ex);
}
```

---

## Custom Exception Classes

Define application-specific exceptions by extending `Exception` or any of its subclasses. Override `to_string()` to provide a descriptive representation.

```solix
package com.example.banking;

import solix.core.Exceptions;
import solix.core.String;

public class InsufficientFundsException extends RuntimeException {
    private float64 required_amount;
    private float64 available_amount;

    public InsufficientFundsException(float64 required, float64 available) {
        this.required_amount  = required;
        this.available_amount = available;
        this.detailed_message = new String("Insufficient funds.");
        this.underlying_cause = (Exception)null;
        this.error_code = 0;
    }

    public float64 get_required()  { return this.required_amount;  }
    public float64 get_available() { return this.available_amount; }

    public override String to_string() {
        return new String("InsufficientFundsException: required=")
            .concat(String.from_float64(this.required_amount, 2))
            .concat(new String(", available="))
            .concat(String.from_float64(this.available_amount, 2));
    }
}
```

Usage:

```solix
public void withdraw(float64 amount) {
    if (amount > this.balance) {
        throw new InsufficientFundsException(amount, this.balance);
    }
    this.balance = this.balance - amount;
}
```

Catching the custom type:

```solix
try {
    account.withdraw(1000.0);
} catch (InsufficientFundsException ex) {
    Console.println("Not enough money.");
    Console.println(ex.to_string());
} catch (RuntimeException ex) {
    Console.println("Other error.");
}
```

---

## The `finally` Guarantee

A `finally` block **always** executes — even when:

- The `try` block completes normally (no exception).
- A `catch` block runs.
- A `catch` block itself throws.
- The exception is re-thrown.
- A `return` statement is reached inside `try` or `catch`.

This makes `finally` ideal for releasing resources that must be cleaned up unconditionally:

```solix
public void use_resource() {
    Resource res = acquire_resource();
    try {
        process(res);
    } catch (RuntimeException ex) {
        Console.errorln("Error during processing.");
    } finally {
        res.release();   // Always called, even if process() throws
    }
}
```

> [!CAUTION]
> Avoid `throw` or `return` inside a `finally` block. Such statements suppress the original exception and make the control flow extremely difficult to reason about.

---

## Exception Propagation

If an exception is thrown and no matching `catch` is found in the current frame, the runtime **unwinds the call stack**:

1. The VM executes the `JMP_TO_OUTER_CLEANUP` opcode, which pops the current frame.
2. Any `finally` blocks registered for that frame execute via the **return-to-cleanup table**.
3. The process repeats for each frame until a matching `catch` is found.
4. If the exception reaches the top-level `main` without being caught, the runtime prints the exception's `to_string()` to `stderr` and terminates with exit code `1`.

```
main()
  └─ processOrder()      ← no matching catch → unwind
       └─ validateInput() ← throws InvalidArgumentException
```

```solix
public void main(char[][] args) {
    try {
        process_order(args);
    } catch (InvalidArgumentException ex) {
        Console.errorln("Invalid order data: ");
        Console.errorln(ex.get_message());
    }
}

public void process_order(char[][] args) {
    validate_input(args);   // May throw — propagates up to main's catch
}

public void validate_input(char[][] args) {
    if (args.length == 0) {
        throw new InvalidArgumentException("No order data provided.");
    }
}
```
