# SwitchStatement (`NodeType::SWITCH_STMT`) & CaseStatement (`NodeType::CASE_STMT`)

## 1. Description & Purpose

The `switch` statement provides multi-way branching based on the value of an integer or enum selector expression. Control transfers to the `case` label whose constant literal value matches the selector. If no match occurs and a `default` label is present, execution jumps to the default block. In Solix, cases can end with explicit `break` statements to exit the switch construct, or support fall-through semantics where control proceeds to adjacent cases.

## 2. Syntax & Grammar

```solix
'switch' '(' <expr> ')' '{' ('case' <constant-literal> ':' <statement...>*)* ['default' ':' <statement...>*] '}'
```

## 3. Underlying Systems & Mechanics

- Evaluates switch expression (must be `int8`..`int64`, `uint8`..`uint64`, `char`, or `enum`).
- Compiles jump table comparing expression value with each constant case.
- Supports fallthrough: if a `case` does not end with `break;` or `return;`, execution flows into the next case instructions.
- `default` branch executes if no cases match.

## 4. Positive Test Scenarios (Valid Variations)

1. **Integer Switching**: `switch (code) { case 200: ... break; default: ... break; }`
2. **Enum Switching**: `switch (state) { case State.IDLE: ... break; }`
3. **Fallthrough Cascading**:
   ```solix
   switch (val) {
       case 1:
       case 2: log_low(); break;
       case 3: log_high(); break;
   }
   ```

## 5. Negative Test Scenarios (Invalid Variations)

1. **Switching on Float, String, or Object Reference**:
   - `switch (3.14) { ... }`  
     *Error*: `Switch expression must evaluate to integer, char, or enum`
   - `switch (new Object()) { ... }`  
     *Error*: `Switch expression must evaluate to integer, char, or enum`
2. **Non-Constant Expression in Case**:
   - `case variable_x: ...`  
     *Error*: `Case value must be a constant literal expression`
3. **Duplicate Case Values**:
   - `case 1: ... case 1: ...`  
     *Error*: `Duplicate case value '1'`
4. **Multiple Default Blocks**:
   - `default: ... default: ...`  
     *Error*: `Switch statement can only have one default branch`
