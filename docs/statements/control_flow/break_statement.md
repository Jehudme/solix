# BreakStatement (`NodeType::BREAK_STMT`)

## 1. Description & Purpose

The `break` statement performs an unconditional jump that terminates the innermost enclosing loop (`for`, `while`, `do-while`) or `switch` statement. In addition to emitting jump instructions to the loop/switch exit label, the compiler must emit ARC scope cleanup instructions for all intermediate lexical blocks traversed between the `break` location and the target loop/switch construct.

## 2. Syntax & Grammar

```solix
'break' ';'
```

## 3. Underlying Systems & Mechanics

- Scans up AST for the innermost enclosing `for`, `while`, `do-while`, or `switch`.
- Emits ARC `DEC_REF` cleanups for all local variables active in the scopes between the `break` statement and the enclosing loop boundary.
- Emits unconditional `JUMP` to the loop/switch exit label.

## 4. Positive Test Scenarios (Valid Variations)

1. **Exiting Loop**: `for (int32 i = 0; i < 10; i++) { if (i == 5) break; }`
2. **Exiting Switch**: `case 1: do_work(); break;`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Break Outside Loop or Switch**:
   - `void test() { break; }`  
     *Error*: `Break statement outside of loop or switch`
