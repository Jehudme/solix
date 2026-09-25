# §6 EnumDeclaration

## 1. Overview & Scope

An `EnumDeclaration` defines a strongly typed, discrete enumeration containing a fixed set of named constant identifiers. Each member maps to an underlying integral ordinal value (assigned sequentially starting from 0 unless explicitly specified).

Enums in Solix are value types. They provide type-safe alternatives to magic numbers, preventing accidental assignment of arbitrary integers, and integrate directly with `SwitchStatement` constructs.

### Syntactic Placement
An `EnumDeclaration` is legally permitted at global package scope or nested directly inside a class.

---

## 2. Syntax & Production Rules

### Production Rules
```solix
EnumDeclaration   ::= 'enum' Identifier '{' EnumMemberList '}'
EnumMemberList    ::= EnumMember (',' EnumMember)* ','?
EnumMember        ::= Identifier ('=' IntegerLiteral)?
```

### Canonical Code Patterns
```solix
// 1. Sequential Enum (0, 1, 2)
enum Direction {
    NORTH,
    SOUTH,
    EAST,
    WEST
}

// 2. Explicit Value Enum
enum StatusCode {
    OK = 200,
    NOT_FOUND = 404,
    SERVER_ERROR = 500
}
```

---

## 3. Scope & Declaration Space (Static Semantics)

### 3.1 Member Resolution & Type Safety
- Members are accessed via qualified syntax: `Direction.NORTH`.
- An enum type is distinct from raw `int32`. Assigning an arbitrary integer to an enum variable without an explicit cast is rejected.

---

## 4. Operational Semantics (Dynamic Execution)

### 4.1 Value Representation
- At runtime, enum members are represented as 64-bit integer scalars (`int64`).
- Evaluation emits immediate integer load opcodes (`PUSH_INT`).

---

## 5. Memory Model & ARC Invariants

### 5.1 Value Type Invariant
- Enums are non-reference types. They incur **zero ARC overhead** and require no heap allocations.

---

## 6. Compile-Time Constraints & Diagnostic Errors

### Rule 6.1: Duplicate Member Identifier
```solix
enum Status {
    ACTIVE,
    ACTIVE // Error: duplicate member
}
```
*Diagnostic Message*:
```text
[ERROR] binder.cpp: Duplicate enum member 'ACTIVE' in enum 'Status'
```

---

## 7. Runtime Fault Conditions

Enums generate no runtime faults; all validations are static.

---

## 8. Conformance & Verification Examples

### Example 8.1: Enum Equality & Switch Matching
```solix
Direction d = Direction.EAST;
if (d == Direction.EAST) {
    Console.println("Facing East");
}
```
*Verification Invariant*: Emits `EQ_I64` comparison; evaluates to `true`.
