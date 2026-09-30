# Solix Virtual Machine Instruction Set Architecture

This document specifies the complete Instruction Set Architecture (ISA) of the Solix virtual machine (VM). It covers the runtime data model, the full opcode table with encoding and stack effects, call frame layout, object memory layout, Automatic Reference Counting (ARC) semantics, and the exception-unwinding mechanism.

---

## Table of Contents

1. [VM Architecture Overview](#1-vm-architecture-overview)
2. [Memory Model](#2-memory-model)
3. [Complete Opcode Table](#3-complete-opcode-table)
   - [3.1 Stack & Constants (opcodes 0–16)](#31-stack--constants-opcodes-016)
   - [3.2 Arithmetic & Logic (opcodes 17–43)](#32-arithmetic--logic-opcodes-1743)
   - [3.3 Local & Global Variables (opcodes 44–47)](#33-local--global-variables-opcodes-4447)
   - [3.4 Control Flow (opcodes 48–50)](#34-control-flow-opcodes-4850)
   - [3.5 Memory & Objects (opcodes 51–58, 90)](#35-memory--objects-opcodes-5158-90)
   - [3.6 ARC Reference Counting (opcodes 59–60)](#36-arc-reference-counting-opcodes-5960)
   - [3.7 Type Conversions (opcodes 61–70, 87–89)](#37-type-conversions-opcodes-6170-8789)
   - [3.8 Calls & Dispatch (opcodes 71–78, 91)](#38-calls--dispatch-opcodes-7178-91)
   - [3.9 Return & Halt (opcodes 79–80)](#39-return--halt-opcodes-7980)
   - [3.10 Exception Handling (opcodes 81–86)](#310-exception-handling-opcodes-8186)
4. [Stack Frame Layout](#4-stack-frame-layout)
5. [Object Memory Layout](#5-object-memory-layout)
6. [ARC Semantics](#6-arc-semantics)
7. [Exception Unwinding](#7-exception-unwinding)

---

## 1. VM Architecture Overview

The Solix VM is a **stack-based** bytecode interpreter. It maintains three logical structures during execution:

### Operand Stack

All computation is performed by pushing operands onto and popping results off of the **operand stack**. Each stack slot holds a single 64-bit word (`uint64_t`). Floating-point values are stored as raw IEEE 754 bit patterns reinterpreted as `uint64_t`. References are stored as unsigned heap addresses.

### Call Frame Stack

Each method invocation creates a new **call frame** on the call frame stack. The VM supports a maximum call depth of **65 536 frames**. Exceeding this limit raises a stack-overflow fault. Each frame contains:

- The **return address** (program counter value to restore on `RETURN`).
- The **frame base pointer** (operand stack position at call entry, used to isolate the callee's local slots).
- A contiguous array of **local variable slots** pre-allocated for the callee.

### Heap

The heap is a flat array of `uint64_t` words. It is divided into two regions:

- **Static region** (low addresses): pre-allocated at program load time via `ALLOC_STATIC` for global/static fields.
- **Dynamic region** (high addresses): managed at runtime via `ALLOC_DYNAMIC`/`DEC_REF` for object instances and arrays.

### Program Counter

The **program counter** (`pc`) is a byte offset into the bytecode stream. Instructions are encoded as a one-byte opcode followed by zero or more immediate operand bytes (big-endian). The PC advances past the opcode and all immediates before the next instruction is decoded.

---

## 2. Memory Model

### Stack Words

Every operand stack slot is a `uint64_t`. Narrower primitive types (`int8`, `int16`, `int32`, `float32`, `bool`, `char`) are zero-extended or sign-extended to 64 bits when pushed; the VM truncates on demand via explicit `CONV_*` instructions.

### Heap Words

The heap is a flat `std::vector<uint64_t>` indexed by `Address` (a `uint64_t` byte-word index). Field offsets within objects are expressed in units of **words** (8 bytes each).

### Object Header

Every heap-allocated object (whether a class instance or an array) is preceded by exactly one header word at `address − 1`:

```
 63                    32 31                     0
┌────────────────────────┬────────────────────────┐
│    size (word count)   │    ref_count (uint32)  │
└────────────────────────┴────────────────────────┘
```

- **`ref_count`** (bits 31:0) — the current ARC strong reference count. An object is live as long as `ref_count > 0`.
- **`size`** (bits 63:32) — the allocation size in words (not counting the header word itself). Used by the allocator to coalesce freed blocks.

> [!NOTE]
> The "address" of an object as held in references and on the stack points **past** the header word, to the first payload word. The header is at `address − 1`.

### VTable ID

For class instances, the first payload word (at `address + 0`) stores the **vtable ID** — a 32-bit identifier linking the object to its class's virtual dispatch table. The vtable ID occupies the lower 32 bits of the first field word; the upper 32 bits are reserved and zero.

---

## 3. Complete Opcode Table

Notation:
- **`imm8`** — 1-byte immediate (signed or unsigned as noted)
- **`imm16`** — 2-byte big-endian immediate
- **`imm32`** — 4-byte big-endian immediate
- **`imm64`** — 8-byte big-endian immediate
- **Stack effect** — `( before -- after )` using top-of-stack on the right

### 3.1 Stack & Constants (opcodes 0–16)

| Op | Mnemonic            | Immediate | Stack Effect              | Description |
|----|---------------------|-----------|---------------------------|-------------|
| 0  | `PUSH_CONST_I8`     | `imm8`    | `( -- i64 )`              | Sign-extend the 1-byte signed immediate to 64 bits and push. |
| 1  | `PUSH_CONST_I16`    | `imm16`   | `( -- i64 )`              | Sign-extend the 2-byte signed immediate to 64 bits and push. |
| 2  | `PUSH_CONST_I32`    | `imm32`   | `( -- i64 )`              | Sign-extend the 4-byte signed immediate to 64 bits and push. |
| 3  | `PUSH_CONST_I64`    | `imm64`   | `( -- i64 )`              | Push the literal 8-byte signed integer value. |
| 4  | `PUSH_CONST_U8`     | `imm8`    | `( -- u64 )`              | Zero-extend the 1-byte unsigned immediate to 64 bits and push. |
| 5  | `PUSH_CONST_U16`    | `imm16`   | `( -- u64 )`              | Zero-extend the 2-byte unsigned immediate to 64 bits and push. |
| 6  | `PUSH_CONST_U32`    | `imm32`   | `( -- u64 )`              | Zero-extend the 4-byte unsigned immediate to 64 bits and push. |
| 7  | `PUSH_CONST_U64`    | `imm64`   | `( -- u64 )`              | Push the literal 8-byte unsigned integer value. |
| 8  | `PUSH_CONST_F32`    | `imm32`   | `( -- f64 )`              | Read 4-byte IEEE 754 float, widen to double, push as 64-bit bit pattern. |
| 9  | `PUSH_CONST_F64`    | `imm64`   | `( -- f64 )`              | Push the 8-byte IEEE 754 double literal as a raw 64-bit bit pattern. |
| 10 | `PUSH_CONST_STRING` | `imm32` (length) + bytes | `( -- ref )` | Allocate a heap string object from the inline UTF-8 bytes (4-byte length prefix followed by that many bytes); push its address. |
| 11 | `PUSH_TRUE`         | —         | `( -- 1 )`                | Push the boolean value `true` (integer `1`). |
| 12 | `PUSH_FALSE`        | —         | `( -- 0 )`                | Push the boolean value `false` (integer `0`). |
| 13 | `PUSH_NULL`         | —         | `( -- 0 )`                | Push the null reference (`0`). |
| 14 | `POP`               | —         | `( val -- )`              | Discard the top stack word. |
| 15 | `DUP`               | —         | `( val -- val val )`      | Duplicate the top stack word. |
| 16 | `DUP2`              | —         | `( a b -- a b a b )`      | Duplicate the top two stack words, preserving order. |

### 3.2 Arithmetic & Logic (opcodes 17–43)

All integer arithmetic operates on the top value(s) as `int64` (signed 64-bit). All float arithmetic operates on the top value(s) as `float64` (IEEE 754 double). Results are pushed as 64-bit words.

| Op | Mnemonic         | Immediate | Stack Effect              | Description |
|----|------------------|-----------|---------------------------|-------------|
| 17 | `ADD_I64`        | —         | `( a b -- a+b )`          | Integer addition. |
| 18 | `ADD_F64`        | —         | `( a b -- a+b )`          | Float addition. |
| 19 | `SUB_I64`        | —         | `( a b -- a-b )`          | Integer subtraction. |
| 20 | `SUB_F64`        | —         | `( a b -- a-b )`          | Float subtraction. |
| 21 | `MUL_I64`        | —         | `( a b -- a*b )`          | Integer multiplication. |
| 22 | `MUL_F64`        | —         | `( a b -- a*b )`          | Float multiplication. |
| 23 | `DIV_I64`        | —         | `( a b -- a/b )`          | Integer division (truncates toward zero). Division by zero raises a runtime fault. |
| 24 | `DIV_F64`        | —         | `( a b -- a/b )`          | Float division (IEEE 754; division by zero yields ±Infinity). |
| 25 | `MOD_I64`        | —         | `( a b -- a%b )`          | Integer modulo (sign follows dividend). Modulo by zero raises a runtime fault. |
| 26 | `EQ_I64`         | —         | `( a b -- bool )`         | Push `1` if `a == b` (integer), else `0`. |
| 27 | `EQ_F64`         | —         | `( a b -- bool )`         | Push `1` if `a == b` (float), else `0`. |
| 28 | `NEQ_I64`        | —         | `( a b -- bool )`         | Push `1` if `a != b` (integer), else `0`. |
| 29 | `NEQ_F64`        | —         | `( a b -- bool )`         | Push `1` if `a != b` (float), else `0`. |
| 30 | `GREATER_I64`    | —         | `( a b -- bool )`         | Push `1` if `a > b` (integer), else `0`. |
| 31 | `GREATER_F64`    | —         | `( a b -- bool )`         | Push `1` if `a > b` (float), else `0`. |
| 32 | `GREATER_EQ_I64` | —         | `( a b -- bool )`         | Push `1` if `a >= b` (integer), else `0`. |
| 33 | `GREATER_EQ_F64` | —         | `( a b -- bool )`         | Push `1` if `a >= b` (float), else `0`. |
| 34 | `LESS_I64`       | —         | `( a b -- bool )`         | Push `1` if `a < b` (integer), else `0`. |
| 35 | `LESS_F64`       | —         | `( a b -- bool )`         | Push `1` if `a < b` (float), else `0`. |
| 36 | `LESS_EQ_I64`    | —         | `( a b -- bool )`         | Push `1` if `a <= b` (integer), else `0`. |
| 37 | `LESS_EQ_F64`    | —         | `( a b -- bool )`         | Push `1` if `a <= b` (float), else `0`. |
| 38 | `LOGICAL_NOT`    | —         | `( val -- bool )`         | Push `1` if `val == 0`, else push `0`. Implements boolean `!`. |
| 39 | `NEGATE`         | —         | `( f -- -f )`             | Negate the top value as a `float64`. |
| 40 | `INC_I64`        | —         | `( n -- n+1 )`            | Increment the top integer value by 1. |
| 41 | `INC_F64`        | —         | `( f -- f+1.0 )`          | Increment the top float value by `1.0`. |
| 42 | `DEC_I64`        | —         | `( n -- n-1 )`            | Decrement the top integer value by 1. |
| 43 | `DEC_F64`        | —         | `( f -- f-1.0 )`          | Decrement the top float value by `1.0`. |

> [!NOTE]
> Bitwise operations (`&`, `|`, `^`, `~`, `<<`, `>>`) on integer types are lowered by the compiler to the corresponding integer arithmetic opcodes at the 64-bit level. There are no dedicated bitwise opcodes; the compiler performs source-level type checking and emits the appropriate integer operation.

### 3.3 Local & Global Variables (opcodes 44–47)

| Op | Mnemonic      | Immediate | Stack Effect          | Description |
|----|---------------|-----------|-----------------------|-------------|
| 44 | `GET_LOCAL`   | `imm32`   | `( -- val )`          | Load the value from local variable slot `imm32` in the current frame and push it. |
| 45 | `SET_LOCAL`   | `imm32`   | `( val -- )`          | Pop the top value and store it into local variable slot `imm32`. |
| 46 | `GET_GLOBAL`  | `imm32`   | `( -- val )`          | Load the word at static heap address `imm32` and push it. |
| 47 | `SET_GLOBAL`  | `imm32`   | `( val -- )`          | Pop the top value and store it at static heap address `imm32`. |

Local slot indices are assigned by the compiler starting at slot 0. Slot 0 is conventionally `this` for instance methods.

### 3.4 Control Flow (opcodes 48–50)

All jump targets are **absolute byte offsets** into the bytecode stream, encoded as a 4-byte big-endian unsigned integer.

| Op | Mnemonic         | Immediate | Stack Effect    | Description |
|----|------------------|-----------|-----------------|-------------|
| 48 | `JUMP`           | `imm32`   | `( -- )`        | Unconditionally set `pc` to `imm32`. |
| 49 | `JUMP_IF_FALSE`  | `imm32`   | `( val -- )`    | Pop `val`; if `val == 0` (false), jump to `imm32`. Otherwise continue. |
| 50 | `JUMP_IF_TRUE`   | `imm32`   | `( val -- )`    | Pop `val`; if `val != 0` (true), jump to `imm32`. Otherwise continue. |

### 3.5 Memory & Objects (opcodes 51–58, 90)

| Op | Mnemonic            | Immediate | Stack Effect                    | Description |
|----|---------------------|-----------|---------------------------------|-------------|
| 51 | `ALLOC_STATIC`      | `imm32`   | `( -- addr )`                   | Reserve `imm32` words in the static heap region. Push the base address. Used for static/global field blocks. |
| 52 | `ALLOC_DYNAMIC`     | `imm32`   | `( -- addr )`                   | Allocate `imm32` payload words on the dynamic heap (best-fit, with an implicit header word). Push the address of the first payload word. Initial ref count = 1. All payload words zeroed. |
| 53 | `GET_PROPERTY`      | `imm32`   | `( obj -- val )`                | Pop object reference `obj`; push the word at `heap[obj + imm32]`. |
| 54 | `SET_PROPERTY`      | `imm32`   | `( obj val -- )`                | Pop `val` and object reference `obj`; store `val` at `heap[obj + imm32]`. |
| 55 | `WEAK_SET_PROPERTY` | `imm32`   | `( obj val -- )`                | Like `SET_PROPERTY` but registers `obj + imm32` as a weak slot for the object pointed to by `val`. If `val`'s ref count later reaches 0, the slot is automatically zeroed. Does not increment `val`'s ref count. |
| 56 | `GET_ARRAY`         | —         | `( arr idx -- val )`            | Pop index `idx` and array address `arr`. Bounds-check `idx` against `heap[arr]` (element count); throw `IndexOutOfBoundsException` if out of range. Push `heap[arr + 1 + idx]`. |
| 57 | `SET_ARRAY`         | —         | `( arr idx val -- )`            | Pop `val`, `idx`, and `arr`. Bounds-check as above. Store `val` at `heap[arr + 1 + idx]`. |
| 58 | `ARRAY_LENGTH`      | —         | `( arr -- len )`                | Pop array address `arr`; push `heap[arr]` (the element count stored in the first payload word). |
| 90 | `SIZEOF`            | —         | `( ref -- size_bytes )`         | Pop heap object reference `ref`. Look up block header at `heap[ref - 1]` to retrieve allocated block size in words, multiply by 8, and push the byte size. If `ref == 0`, push `0`. |

### 3.6 ARC Reference Counting (opcodes 59–60)

| Op | Mnemonic   | Immediate | Stack Effect    | Description |
|----|------------|-----------|-----------------|-------------|
| 59 | `INC_REF`  | —         | `( ref -- ref )`| Increment the ref count of the object at address `ref` (i.e., increment the low 32 bits of the header word at `ref − 1`). If `ref == 0` (null), no-op. The reference remains on the stack. |
| 60 | `DEC_REF`  | —         | `( ref -- )`    | Decrement the ref count of the object at address `ref`. If the count reaches 0, deallocate the object (add its header address to the free-block list, zero any registered weak slots). If `ref == 0`, no-op. |

### 3.7 Type Conversions (opcodes 61–70, 87–89)

Conversion instructions pop the top stack word, reinterpret or convert its value, and push the result. All results are 64-bit words.

| Op | Mnemonic       | Stack Effect        | Description |
|----|----------------|---------------------|-------------|
| 61 | `CONV_I8`      | `( val -- i8_ext )` | Truncate to the low 8 bits; sign-extend to 64 bits. |
| 62 | `CONV_I16`     | `( val -- i16_ext )`| Truncate to the low 16 bits; sign-extend to 64 bits. |
| 63 | `CONV_I32`     | `( val -- i32_ext )`| Truncate to the low 32 bits; sign-extend to 64 bits. |
| 64 | `CONV_I64`     | `( val -- i64 )`    | Reinterpret as a signed 64-bit integer (no-op at the word level; ensures semantic sign interpretation). |
| 65 | `CONV_U8`      | `( val -- u8_ext )` | Mask to the low 8 bits; zero-extend to 64 bits. |
| 66 | `CONV_U16`     | `( val -- u16_ext )`| Mask to the low 16 bits; zero-extend to 64 bits. |
| 67 | `CONV_U32`     | `( val -- u32_ext )`| Mask to the low 32 bits; zero-extend to 64 bits. |
| 68 | `CONV_U64`     | `( val -- u64 )`    | Reinterpret as an unsigned 64-bit integer (no-op at the word level). |
| 69 | `CONV_F32`     | `( val -- f64 )`    | Convert integer (treated as `int64`) to `float32`, then widen to `float64`; push the 64-bit bit pattern. |
| 70 | `CONV_F64`     | `( val -- f64 )`    | Convert integer (treated as `int64`) to `float64`; push the 64-bit bit pattern. |
| 87 | `CONV_I_TO_F`  | `( bits -- f64 )`   | Reinterpret the raw 64-bit word as an IEEE 754 `double` without numeric conversion (bit-cast). Used for float literal loading. |
| 88 | `CONV_F_TO_I`  | `( f64 -- bits )`   | Reinterpret the IEEE 754 `double` bit pattern as a raw `uint64_t` (bit-cast). |
| 89 | `NEGATE_I64`   | `( n -- -n )`       | Negate the top value as a signed 64-bit integer (two's complement). |

### 3.8 Calls & Dispatch (opcodes 71–78, 91)

| Op | Mnemonic             | Immediate | Stack Effect                                 | Description |
|----|----------------------|-----------|----------------------------------------------|-------------|
| 71 | `CALL`               | —         | `( target_ip arg_count -- retval )`          | Pop `arg_count` and `target_ip`. Create return frame recording caller's `pc`, `frame_pointer = (sp - arg_count)`, and `arg_count`. Jump to `target_ip`. Arguments reside in slots `0..arg_count-1`. Callee reserves local slots via `ALLOC_FRAME`. |
| 72 | `CALL_NATIVE`        | `imm32`   | `( args... -- retval )`                      | Invoke the C++ native function registered under ID `imm32`. Arguments are popped from the stack; the native function may push a return value. |
| 73 | `DEFINE_NATIVE`      | `imm32`   | `( -- )`                                     | Register a binding entry for native ID `imm32`. This is an initialization-time instruction emitted once per native method. |
| 74 | `CALL_VIRTUAL`       | `imm32(slot) imm32(args)` | `( this args... -- retval )` | Read object address at `stack[sp - args]`; look up `slot` in `heap[this].vtable_id`; push return frame; jump to target. Callee allocates local slots via `ALLOC_FRAME`. |
| 75 | `DEFINE_VTABLE`      | `imm32`   | `( -- )`                                     | Define a new vtable with ID `imm32`. Subsequent `SET_VTABLE` + `DEFINE_VTABLE` pairs populate its slots. Initialization-time instruction. |
| 76 | `SET_VTABLE`         | `imm32`   | `( obj -- obj )`                             | Set the vtable ID of the object at the top of the stack (stored in `heap[obj + 0]`) to `imm32`. Does not pop the reference. |
| 77 | `CAST_CHECK`         | `imm32`   | `( ref -- ref )`                             | Verify that the object at `ref` is an instance of the type with vtable ID `imm32` (traverses the vtable hierarchy). If the check fails, throw a `TypeCastException`. If `ref == null`, pass through (null is assignable to any reference type). |
| 78 | `INSTANCEOF`         | `imm32`   | `( ref -- bool )`                            | Pop `ref`; push `1` if the object's vtable hierarchy includes vtable ID `imm32`, else push `0`. If `ref == null`, push `0`. Does not throw. |
| 91 | `ALLOC_FRAME`        | `imm32`   | `( -- )`                                     | Callee function prologue instruction. Given `frame_size` (`imm32`), compares with caller `arg_count`. If `frame_size > arg_count`, expands the operand stack by `(frame_size - arg_count)` zeroed slots to reserve local variable storage. |

### 3.9 Return & Halt (opcodes 79–80)

| Op | Mnemonic  | Immediate | Stack Effect         | Description |
|----|-----------|-----------|----------------------|-------------|
| 79 | `RETURN`  | —         | `( retval -- )`      | Restore the previous call frame (`sp = stack + frame.frame_pointer`). If the callee left a return value on the stack, it is restored and pushed as the top word in the caller's frame. For `void` methods, `null` (0) is returned. |
| 80 | `HALT`    | —         | `( -- )`             | Terminate VM execution immediately. Any value remaining on the stack is ignored. |

### 3.10 Exception Handling (opcodes 81–86)

| Op | Mnemonic                    | Immediate | Stack Effect        | Description |
|----|-----------------------------|-----------|---------------------|-------------|
| 81 | `THROW_ABSTRACT`            | —         | `( -- )`            | Throw a fatal "Abstract method not implemented" error. Used as the body of abstract method stubs. |
| 82 | `REGISTER_RETURN_CLEANUP`   | `imm32`   | `( -- )`            | Register `imm32` as the cleanup trampoline address for the current frame. When the frame is unwound due to an exception, execution jumps to this address before leaving the frame. |
| 83 | `JMP_TO_OUTER_CLEANUP`      | —         | `( -- )`            | Jump to the cleanup trampoline registered by the enclosing frame's `REGISTER_RETURN_CLEANUP`. Used to chain `finally` blocks across nested try scopes. |
| 84 | `THROW_EXCEPTION`           | —         | `( exobj -- )`      | Pop the exception object reference `exobj` from the stack, store it as the active exception, and begin stack unwinding (see §7). |
| 85 | `GET_EXCEPTION`             | —         | `( -- exobj )`      | Push the currently active exception object reference onto the stack. Used inside `catch` handlers to access the thrown object. |
| 86 | `CLEAR_EXCEPTION`           | —         | `( -- )`            | Clear the active exception state. Emitted at the end of a `catch` or `finally` block to signal that the exception has been handled. |

---

## 4. Stack Frame Layout

Solix uses a callee-allocated stack frame architecture. When a `CALL` or `CALL_VIRTUAL` instruction fires, caller arguments reside on the unified operand stack. The call instruction establishes a `Frame` record:

```
┌─────────────────────────────────────────────┐
│  return_ip      (uint32_t) — caller's pc    │  ← saved when CALL executes
│  frame_pointer  (uint32_t) — caller's sp    │  ← start of arguments/locals
│  arg_count      (uint32_t) — parameter count│
└─────────────────────────────────────────────┘
```

In the callee's prologue, the `ALLOC_FRAME <frame_size>` instruction executes:
1. `slot[0] .. slot[arg_count - 1]` contain the incoming arguments (or `this` instance reference followed by arguments).
2. If `frame_size > arg_count`, the VM reserves `(frame_size - arg_count)` additional zeroed stack slots above the arguments for local variables.
3. Subsequent statements access locals using `GET_LOCAL <idx>` and `SET_LOCAL <idx>` relative to `frame_pointer`.
4. When `RETURN` executes, `sp` is restored directly to `stack + frame.frame_pointer`, automatically collapsing all arguments and local variables before pushing the return value.

---

## 5. Object Memory Layout

### Class Instance

A class instance occupies `1 + N` heap words, where `N` is the number of declared fields (including inherited fields, ordered superclass-first):

```
heap[addr − 1]  ─── header: size(32-bit) | ref_count(32-bit)
heap[addr + 0]  ─── vtable_id (uint32, stored in low 32 bits; upper 32 reserved)
heap[addr + 1]  ─── field[0]   (first declared field of root superclass)
heap[addr + 2]  ─── field[1]
...
heap[addr + N]  ─── field[N-1] (last field of concrete class)
```

Field offsets are assigned by the compiler in declaration order, superclass fields first, and encoded directly in `GET_PROPERTY`/`SET_PROPERTY` immediates.

### Array

An array of `K` elements occupies `1 + 1 + K` heap words:

```
heap[addr − 1]  ─── header: size(= K + 1) | ref_count
heap[addr + 0]  ─── element_count  (uint64_t = K)
heap[addr + 1]  ─── element[0]
heap[addr + 2]  ─── element[1]
...
heap[addr + K]  ─── element[K-1]
```

The element count word at `addr + 0` is what `ARRAY_LENGTH` reads and what `GET_ARRAY`/`SET_ARRAY` use for bounds checking.

---

## 6. ARC Semantics

Solix uses **Automatic Reference Counting (ARC)** for heap memory management. There is no tracing garbage collector; objects are freed synchronously when their reference count drops to zero.

### Reference Count Invariants

- **Initial count**: Every newly allocated object (via `ALLOC_DYNAMIC`) starts with `ref_count = 1`, representing the reference held by the allocating expression.
- **`INC_REF`**: Emitted whenever a second live reference to an object is created (e.g., storing a reference into a field or local variable while retaining the original). Increments the low 32 bits of the header word.
- **`DEC_REF`**: Emitted whenever a live reference goes out of scope or is overwritten. Decrements the low 32 bits of the header word. If the result is **0**, the object is immediately deallocated:
  1. Any slots registered as weak references pointing to this object are zeroed.
  2. The header address is added to the allocator's free-block list for reuse.
  3. No recursive deallocation of referenced fields occurs at this level — the compiler emits `DEC_REF` for each owned child reference before emitting `DEC_REF` for the parent.

### Weak References

A field declared `weak` is stored via `WEAK_SET_PROPERTY`. The memory subsystem records the field slot address in a per-object weak-reference set. When the target object is deallocated, all registered weak slots are automatically set to `0` (null). Weak references do **not** increment the target's ref count and do not prevent deallocation. They are used to break reference cycles (e.g., a child holding a `weak` reference back to its parent).

### ARC and Exceptions

When a stack frame is unwound due to an exception, the compiler-generated cleanup trampoline (registered via `REGISTER_RETURN_CLEANUP`) is responsible for emitting `DEC_REF` on all local reference-type variables that were in scope at the throw point. This ensures that no heap object is leaked during exception propagation.

---

## 7. Exception Unwinding

The Solix VM uses a **trampoline-based** unwinding model rather than a separate exception table. Each method that contains any reference-type local variables or `try`/`finally` blocks registers a cleanup entry point using `REGISTER_RETURN_CLEANUP`.

### Normal Execution Path

```
CALL methodA
  │
  ├── REGISTER_RETURN_CLEANUP  <cleanup_A>    ; register cleanup
  │
  ├── ... body of methodA ...
  │
  ├── DEC_REF all locals                      ; explicit pre-return cleanup
  └── RETURN                                  ; return normally
```

### Exception Unwinding Path

When `THROW_EXCEPTION` is executed:

1. The active exception reference is stored in the VM's exception register.
2. The VM walks the call frame stack upward, frame by frame.
3. For each frame that has a registered cleanup trampoline, execution jumps to that trampoline address.
4. The trampoline emits the necessary `DEC_REF` operations for local variables and then executes `JMP_TO_OUTER_CLEANUP` to continue propagation to the next frame.
5. If a `catch` block matching the thrown type is found, execution transfers to the handler. The handler calls `GET_EXCEPTION` to retrieve the exception object and `CLEAR_EXCEPTION` upon completion.
6. If a `finally` block is present, the compiler emits its body at the trampoline site, ensuring it executes on both normal and exceptional paths.
7. If no matching handler is found after exhausting all frames, the VM terminates with an unhandled exception report.

### Try/Catch/Finally Compilation Pattern

```solix
try {
    riskyOperation();
} catch (IOException e) {
    handleError(e);
} finally {
    cleanup();
}
```

Compiles to the following logical structure:

```
REGISTER_RETURN_CLEANUP  <trampoline>

; --- try body ---
CALL riskyOperation
JUMP <after_catch>

; --- catch handler ---
<catch_entry>:
GET_EXCEPTION
CAST_CHECK  <IOException_vtable_id>
SET_LOCAL   <e_slot>
CALL handleError
CLEAR_EXCEPTION
JUMP <finally_entry>

; --- finally (normal path) ---
<after_catch>:
<finally_entry>:
CALL cleanup
JUMP <end>

; --- trampoline (exception path) ---
<trampoline>:
CALL cleanup              ; finally body runs on exception path too
JMP_TO_OUTER_CLEANUP      ; continue unwinding

<end>:
```
