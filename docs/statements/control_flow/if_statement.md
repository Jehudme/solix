# IfStatement

## 1. Overview & Scope

An `IfStatement` provides conditional branching based on the evaluation of a boolean predicate expression. If the condition evaluates to `true`, the `then` branch executes. If `false`, control transfers to the optional `else` branch (or bypasses the statement).

In Solix:
- The condition must be strictly of type `bool`. Numeric zero/non-zero truthiness is rejected.
- The compiler lowers the if-statement using conditional jump instructions (`JUMP_IF_FALSE`) with forward jump target patching.

---

## 2. Compilation & Runtime Mechanics (With Real Bytecode)

### Bytecode Disassembly Example
```solix
// Solix Code
if (score >= 50) {
    Console.println("Pass");
} else {
    Console.println("Fail");
}
```

```bytecode
// Compiled VM Bytecode
GET_LOCAL 1                 // Load score
PUSH_CONST_I32 50
GREATER_EQ_I64              // Pushes bool (true/false)
JUMP_IF_FALSE <else_ip>     // Jump to else branch if false

// --- Then Branch ---
PUSH_CONST_STRING 0         // "Pass"
CALL_NATIVE Console.println
JUMP <end_ip>               // Jump over else branch

// --- Else Branch (<else_ip>) ---
PUSH_CONST_STRING 1         // "Fail"
CALL_NATIVE Console.println

// --- End (<end_ip>) ---
```

---

> [!NOTE]
> For all positive test scenarios and negative failure cases for this construct, see [IfStatement in TEST_SPECIFICATION.md](../../TEST_SPECIFICATION.md#ifstatement).
