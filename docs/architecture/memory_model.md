# Byte-Addressed Memory Model & Alignment Architecture

## 1. Overview

Starting in Phase 4, the Solix Virtual Machine transitioned from a word-addressed (`std::vector<uint64_t>`) heap to a **byte-addressed byte buffer (`std::vector<uint8_t>`)**. This transition eliminates memory bloat for narrow primitive types (`bool`, `char`, `int8`, `uint8`, `int16`, `uint16`, `int32`, `uint32`), provides zero-copy C-ABI interop buffers, and enforces standard natural field alignment across all user-defined class instances.

---

## 2. Heap Block & Header Layout

Every dynamically allocated heap block (objects, arrays, closures, strings) resides within `Memory::heap` with an 8-byte header positioned directly before the object base address:

```text
Byte offset [address - 8 ... address - 5]:  [ 4 bits BlockTag | 28 bits Allocation Size in Bytes ]
Byte offset [address - 4 ... address - 1]:  [ 32 bits Reference Count                            ]
Byte offset [address + 0 ...           ]:  [ Object Payload / VTable ID / Array Length / Data    ]
```

### 2.1. Block Tags (`BlockTag`)
The top 4 bits of the 32-bit size field encode the runtime payload type:
- `RAW (0)`: Untyped raw byte buffer or string payload
- `OBJECT (1)`: User class instance (with 8-byte VTable ID at `address + 0`)
- `ARRAY_U8 (2)`: Contiguous unsigned 8-bit array (`uint8[]`, `bool[]`)
- `ARRAY_I8 (3)`: Contiguous signed 8-bit array (`int8[]`, `char[]`)
- `ARRAY_U16 (4)`: Contiguous unsigned 16-bit array (`uint16[]`)
- `ARRAY_I16 (5)`: Contiguous signed 16-bit array (`int16[]`)
- `ARRAY_U32 (6)`: Contiguous unsigned 32-bit array (`uint32[]`)
- `ARRAY_I32 (7)`: Contiguous signed 32-bit array (`int32[]`)
- `ARRAY_U64 (8)`: Contiguous 64-bit array (`int64[]`, `uint64[]`, `float64[]`)
- `ARRAY_REF (9)`: Array of reference pointers requiring child ARC tracking
- `CLOSURE (10)`: Closure instance containing captured variable slots

---

## 3. Natural Field Alignment in Class Instances

In class layout calculation (`Binder::calculate_layout`), fields are packed contiguously while respecting standard natural alignment boundaries:

| Type Category | Types | Width | Alignment |
|---|---|---|---|
| 1-byte | `bool`, `char`, `int8`, `uint8` | 1 byte | 1-byte boundary |
| 2-byte | `int16`, `uint16` | 2 bytes | 2-byte boundary |
| 4-byte | `int32`, `uint32`, `float32`, `enum` | 4 bytes | 4-byte boundary |
| 8-byte | `int64`, `uint64`, `float64`, Class references, Weak references, Arrays, Function pointers | 8 bytes | 8-byte boundary |

Field offsets start after the 8-byte VTable ID slot at `address + 8`. Padding bytes are inserted whenever a field's current offset is not an integer multiple of its natural alignment requirement. Total instance allocation size is rounded up to the nearest multiple of 8 bytes.

---

## 4. Property Opcode Width Encoding

To read and write fields with the correct stride and sign-extension without dedicated per-type opcodes, `GET_PROPERTY` and `SET_PROPERTY` pack both the field's byte offset and its width code into their 32-bit immediate operand (`imm32`):

```text
Bits [27:0]:  Raw byte offset from object base address
Bits [31:28]: Width / Sign-extension code
```

### Width Codes:
- `0`: 64-bit word (`int64`, `uint64`, references, arrays, pointers)
- `1`: Unsigned 8-bit (`uint8`, `bool`) -> zero-extended to 64-bit stack slot
- `2`: Unsigned 16-bit (`uint16`) -> zero-extended to 64-bit stack slot
- `3`: Unsigned 32-bit (`uint32`) -> zero-extended to 64-bit stack slot
- `4`: 64-bit floating point (`float64`)
- `5`: Signed 8-bit (`int8`, `char`) -> sign-extended to 64-bit stack slot
- `6`: Signed 16-bit (`int16`) -> sign-extended to 64-bit stack slot
- `7`: Signed 32-bit (`int32`) -> sign-extended to 64-bit stack slot

---

## 5. Array Representation & Stride

Arrays in the byte-addressed heap have the following structure:
- `address + 0`: 8-byte element count (`uint64_t length`)
- `address + 8`: Contiguous payload elements packed according to element size:
  - Stride 1: `uint8[]`, `int8[]`, `bool[]`, `char[]`
  - Stride 2: `uint16[]`, `int16[]`
  - Stride 4: `uint32[]`, `int32[]`, `float32[]`
  - Stride 8: `uint64[]`, `int64[]`, `float64[]`, object references (`ARRAY_REF`)

Opcodes `GET_ARRAY` and `SET_ARRAY` inspect the array's `BlockTag` at `address - 8` to determine element stride and apply appropriate sign extension when pushing values onto the 64-bit operand stack.

---

## 6. Safe Memory Primitives

All memory reads and writes in `RuntimeContext` and native standard library modules utilize inline, safe accessor primitives that use `std::memcpy`:
- `read_u8(addr)`, `write_u8(addr, val)`
- `read_u16(addr)`, `write_u16(addr, val)`
- `read_u32(addr)`, `write_u32(addr, val)`
- `read_u64(addr)`, `write_u64(addr, val)`
- `read_f64(addr)`, `write_f64(addr, val)`
- `read_char(addr)`, `write_char(addr, val)`
- `get_array_length(addr)`
- `get_block_tag(addr)`
- `get_block_ref_count(addr)`

This guarantees clean, undefined-behavior-free memory access across architectures regardless of platform pointer alignment constraints.
