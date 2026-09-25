# EnumDeclaration (`NodeType::ENUM_DECL`)

## 1. Description & Purpose

The `enum` declaration introduces a strongly typed, discrete enumeration containing a fixed set of named constant identifiers. Each enumeration member maps to an underlying integral value (assigned sequentially starting from 0 unless explicitly specified). Enums provide type-safe alternatives to magic numbers and can be used in pattern matching, switch statements, and conditional expressions.

## 2. Syntax & Grammar

```solix
[access-modifier] enum <identifier> '{' <identifier> (',' <identifier>)* [','] '}'
```

## 3. Underlying Systems & Mechanics

- Registered in `global_scope.symbols` as an `EnumDeclaration`.
- Members assigned integer ordinal indices (0, 1, 2, ...).
- Emitted in bytecode as integer constants.
- Supports switch dispatch and equality comparisons.

## 4. Positive Test Scenarios (Valid Variations)

1. **Standard Enum**: `enum State { IDLE, RUNNING, PAUSED, STOPPED }`
2. **Trailing Comma Enum**: `enum Level { LOW, HIGH, }`
3. **Qualified Member Access**: `State s = State.RUNNING;`
4. **Enum In Switch**: `switch (s) { case State.IDLE: ... }`

## 5. Negative Test Scenarios (Invalid Variations)

1. **Duplicate Enum Constants**:
   - `enum Mode { ON, OFF, ON }`  
     *Error*: `Duplicate enum constant 'ON'`
2. **Accessing Non-Existent Member**:
   - `State s = State.UNKNOWN;`  
     *Error*: `Undefined enum member: UNKNOWN`
3. **Assigning Integer to Enum Without Cast**:
   - `State s = 0;`  
     *Error*: `Type mismatch in variable declaration: expected 'State', got 'int32'`
