# Function Pointer Expressions

## 1. Overview & Purpose

Solix supports C-compatible, strongly-typed first-class function pointers. A function pointer variable stores the code entry address of a static function or static method without any heap overhead or closure environment allocation.

Function pointer types use the syntax:
```solix
<ReturnType>(*)(<ParamType1>, <ParamType2>, ...)
```

Function pointers can be:
- Declared as local variables, method parameters, or class fields.
- Assigned from static method names (e.g., `int32(*)(int32) fn = MathUtils.square;`).
- Invoked directly using standard function call parentheses `fn(arg1, arg2)`.
- Reassigned at runtime or passed as callbacks to higher-order algorithms.

Unlike lambdas with captures, pure function pointers require zero runtime heap memory and do not trigger reference counting operations (`INC_REF` / `DEC_REF`).

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Assignment and Invocation
When referencing a static method as a function pointer, the compiler emits `PUSH_CONST_I32 <target_ip>` containing the resolved bytecode address of the function.

Invocation uses the standard `CALL` instruction, popping the arguments and target address from the stack.

### Bytecode Disassembly Example
```solix
// Solix Code
public class MathUtils {
    public static int32 doubleVal(int32 x) {
        return x * 2;
    }
}

public class Main {
    public static int32 main() {
        int32(*)(int32) op = MathUtils.doubleVal;
        return op(21);
    }
}
```

```bytecode
// Compiled VM Bytecode for Main.main()
PUSH_CONST_I32 <MathUtils.doubleVal_ip>  // Address of target static method
SET_LOCAL 0                             // Store into function pointer variable 'op'

// Invocation
PUSH_CONST_I32 21                       // Push argument 1
GET_LOCAL 0                             // Load target IP from 'op'
PUSH_CONST_I32 1                        // Argument count
CALL                                    // Invoke function at target address
RETURN
```

---

## 3. Type Checking & Safety
- **Strict Signature Matching**: The return type and all parameter types of the assigned static method must match the function pointer type exactly.
- **Instance Method Restriction**: Instance methods require a `this` receiver and cannot be assigned to simple function pointers without a lambda wrapper or closure binding.
