#include "solix/runtime.hpp"
#include "utilities/optcodes.hpp"
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace solix {

uint64_t Memory::static_allocation(size_t size_in_words, Address address) {
  if (address == 0) {
    address = next_free_static;
    next_free_static += size_in_words;
    if (next_free_static >= heap.size()) {
      throw std::runtime_error("Heap overflow during static allocation!");
    }
  }
  currently_used_words += size_in_words;
  if (currently_used_words > peak_used_words)
    peak_used_words = currently_used_words;
  return address;
}

uint64_t Memory::dynamic_allocation(size_t size_in_words, Address address) {
  if (next_free_dynamic == 1) {
    next_free_dynamic = heap.size() / 2;
  }
  Address header_addr = 0;
  if (!free_blocks.empty()) {
    header_addr = free_blocks.back();
    free_blocks.pop_back();
  } else {
    header_addr = next_free_dynamic;
    next_free_dynamic += (size_in_words + 1);
    if (next_free_dynamic >= heap.size())
      throw std::runtime_error("Heap overflow during dynamic allocation!");
  }
  currently_used_words += (size_in_words + 1);
  if (currently_used_words > peak_used_words)
    peak_used_words = currently_used_words;

  for (size_t i = 0; i < size_in_words; ++i) {
    heap[header_addr + 1 + i] = 0;
  }

  uint64_t header = (static_cast<uint64_t>(size_in_words) << 32) | 1ULL;
  heap[header_addr] = header;
  return header_addr + 1;
}

void Memory::deallocate(Address address) {
  if (address == 0)
    return;
  if (address == 50) {
    std::cout << "[DEBUG] Deallocating address 50!" << std::endl;
  }

  if (auto it = weak_references.find(address); it != weak_references.end()) {
    for (Address weak_slot : it->second) {
      heap[weak_slot] = 0;
    }
    weak_references.erase(it);
  }

  Address header_addr = address - 1;
  uint64_t header = heap[header_addr];
  uint32_t size = static_cast<uint32_t>(header >> 32);
  currently_used_words -= (size + 1);
  free_blocks.push_back(header_addr);
}

inline void Memory::write_u64(Address address, uint32_t offset,
                              uint64_t value) {
  heap[address + offset] = value;
}
inline uint64_t Memory::read_u64(Address address, uint32_t offset) const {
  return heap[address + offset];
}
inline void Memory::write_f64(Address address, uint32_t offset, double value) {
  heap[address + offset] = std::bit_cast<uint64_t>(value);
}
inline double Memory::read_f64(Address address, uint32_t offset) const {
  return std::bit_cast<double>(heap[address + offset]);
}
inline void Memory::write_char(Address address, uint32_t offset, char value) {
  heap[address + offset] = static_cast<uint64_t>(value);
}
inline char Memory::read_char(Address address, uint32_t offset) const {
  return static_cast<char>(heap[address + offset]);
}

inline void Memory::increase_reference(Address address) {
  if (address == 0)
    return;
  heap[address - 1]++;
}

inline void Memory::decrease_reference(Address address) {
  if (address == 0)
    return;
  uint64_t &header = heap[address - 1];
  uint32_t ref_count = static_cast<uint32_t>(header & 0xFFFFFFFF);
  if (ref_count > 0) {
    ref_count--;
    header = (header & 0xFFFFFFFF00000000ULL) | ref_count;
    if (ref_count == 0)
      deallocate(address);
  }
}

RuntimeContext::RuntimeContext(const RuntimeOptions &opts)
    : options(opts), memory(opts.stack_capacity, opts.heap_capacity) {

  if (std::holds_alternative<Bytecode>(opts.bytecode_source)) {
    bytecode = std::get<Bytecode>(opts.bytecode_source);
  } else {
    auto path = std::get<std::filesystem::path>(opts.bytecode_source);
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
      throw std::runtime_error("Could not open bytecode file: " +
                               path.string());

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    bytecode.resize(size);
    if (!file.read(reinterpret_cast<char *>(bytecode.data()), size)) {
      throw std::runtime_error("Failed to read bytecode file: " +
                               path.string());
    }
  }
}

void RuntimeContext::register_native(uint32_t id, NativeFunction func) {
  native_registry[id] = std::move(func);
}

void RuntimeContext::push(uint64_t val) {

  if (memory.stack_pointer >= memory.stack.size())
    throw std::runtime_error("Stack overflow");
  memory.stack[memory.stack_pointer++] = val;
}
uint64_t RuntimeContext::pop() {

  if (memory.stack_pointer == 0)
    throw std::runtime_error("Stack underflow");
  return memory.stack[--memory.stack_pointer];
}

static inline uint32_t read_u32(const Bytecode &bcode, Address &pc) {
  uint32_t val = (bcode[pc] << 24) | (bcode[pc + 1] << 16) |
                 (bcode[pc + 2] << 8) | bcode[pc + 3];
  pc += 4;
  return val;
}
static inline uint64_t read_u64(const Bytecode &bcode, Address &pc) {
  uint64_t val = (static_cast<uint64_t>(bcode[pc]) << 56) |
                 (static_cast<uint64_t>(bcode[pc + 1]) << 48) |
                 (static_cast<uint64_t>(bcode[pc + 2]) << 40) |
                 (static_cast<uint64_t>(bcode[pc + 3]) << 32) |
                 (static_cast<uint64_t>(bcode[pc + 4]) << 24) |
                 (static_cast<uint64_t>(bcode[pc + 5]) << 16) |
                 (static_cast<uint64_t>(bcode[pc + 6]) << 8) |
                 static_cast<uint64_t>(bcode[pc + 7]);
  pc += 8;
  return val;
}
static inline std::string read_string(const Bytecode &bcode, Address &pc) {
  uint32_t length = read_u32(bcode, pc);
  std::string str(reinterpret_cast<const char *>(&bcode[pc]), length);
  pc += length;
  return str;
}

template <typename T> inline uint64_t bit_cast_to_u64(T value) {
  uint64_t result = 0;
  std::memcpy(&result, &value, sizeof(T));
  return result;
}
template <typename T> inline T bit_cast_from_u64(uint64_t value) {
  T result;
  std::memcpy(&result, &value, sizeof(T));
  return result;
}

void RuntimeContext::execute() {
  const uint8_t *code = bytecode.data();
  uint64_t *stack = memory.stack.data();
  uint64_t *heap_data = memory.heap.data();

  uint64_t *sp = stack + memory.stack_pointer;

#define PUSH(val) (*sp++ = (val))
#define POP() (*--sp)
#define PEEK() (*(sp - 1))
#define SYNC_SP() (memory.stack_pointer = static_cast<uint32_t>(sp - stack))
#define RESTORE_SP() (sp = stack + memory.stack_pointer)

  // Add an initial frame so we don't underflow
  call_depth = 0;
  call_stack[call_depth++] = Frame(0, 0);

  size_t arg_count = options.program_args.size();
  Address args_array = memory.dynamic_allocation(arg_count);
  for (size_t i = 0; i < arg_count; ++i) {
    const std::string &str = options.program_args[i];
    size_t len = str.length();
    Address str_addr = memory.dynamic_allocation(len);
    for (size_t j = 0; j < len; ++j) {
      heap_data[str_addr + j] = static_cast<uint64_t>(str[j]);
    }
    heap_data[args_array + i] = str_addr;
  }
  PUSH(args_array);

#if defined(_MSC_VER) && !defined(__clang__)
#define DISPATCH() goto vm_dispatch
  goto vm_dispatch;
vm_dispatch:
  switch (code[program_counter++]) {
    case 0: goto op_PUSH_CONST_I8;
    case 1: goto op_PUSH_CONST_I16;
    case 2: goto op_PUSH_CONST_I32;
    case 3: goto op_PUSH_CONST_I64;
    case 4: goto op_PUSH_CONST_U8;
    case 5: goto op_PUSH_CONST_U16;
    case 6: goto op_PUSH_CONST_U32;
    case 7: goto op_PUSH_CONST_U64;
    case 8: goto op_PUSH_CONST_F32;
    case 9: goto op_PUSH_CONST_F64;
    case 10: goto op_PUSH_CONST_STRING;
    case 11: goto op_PUSH_TRUE;
    case 12: goto op_PUSH_FALSE;
    case 13: goto op_PUSH_NULL;
    case 14: goto op_POP;
    case 15: goto op_DUP;
    case 16: goto op_DUP2;
    case 17: goto op_ADD_I64;
    case 18: goto op_ADD_F64;
    case 19: goto op_SUB_I64;
    case 20: goto op_SUB_F64;
    case 21: goto op_MUL_I64;
    case 22: goto op_MUL_F64;
    case 23: goto op_DIV_I64;
    case 24: goto op_DIV_F64;
    case 25: goto op_MOD_I64;
    case 26: goto op_EQ_I64;
    case 27: goto op_EQ_F64;
    case 28: goto op_NEQ_I64;
    case 29: goto op_NEQ_F64;
    case 30: goto op_GREATER_I64;
    case 31: goto op_GREATER_F64;
    case 32: goto op_GREATER_EQ_I64;
    case 33: goto op_GREATER_EQ_F64;
    case 34: goto op_LESS_I64;
    case 35: goto op_LESS_F64;
    case 36: goto op_LESS_EQ_I64;
    case 37: goto op_LESS_EQ_F64;
    case 38: goto op_LOGICAL_NOT;
    case 39: goto op_NEGATE;
    case 40: goto op_INC_I64;
    case 41: goto op_INC_F64;
    case 42: goto op_DEC_I64;
    case 43: goto op_DEC_F64;
    case 44: goto op_GET_LOCAL;
    case 45: goto op_SET_LOCAL;
    case 46: goto op_GET_GLOBAL;
    case 47: goto op_SET_GLOBAL;
    case 48: goto op_JUMP;
    case 49: goto op_JUMP_IF_FALSE;
    case 50: goto op_JUMP_IF_TRUE;
    case 51: goto op_ALLOC_STATIC;
    case 52: goto op_ALLOC_DYNAMIC;
    case 53: goto op_GET_PROPERTY;
    case 54: goto op_SET_PROPERTY;
    case 55: goto op_WEAK_SET_PROPERTY;
    case 56: goto op_GET_ARRAY;
    case 57: goto op_SET_ARRAY;
    case 58: goto op_ARRAY_LENGTH;
    case 59: goto op_INC_REF;
    case 60: goto op_DEC_REF;
    case 61: goto op_CONV_I8;
    case 62: goto op_CONV_I16;
    case 63: goto op_CONV_I32;
    case 64: goto op_CONV_I64;
    case 65: goto op_CONV_U8;
    case 66: goto op_CONV_U16;
    case 67: goto op_CONV_U32;
    case 68: goto op_CONV_U64;
    case 69: goto op_CONV_F32;
    case 70: goto op_CONV_F64;
    case 71: goto op_CALL;
    case 72: goto op_CALL_NATIVE;
    case 73: goto op_DEFINE_NATIVE;
    case 74: goto op_CALL_VIRTUAL;
    case 75: goto op_DEFINE_VTABLE;
    case 76: goto op_SET_VTABLE;
    case 77: goto op_CAST_CHECK;
    case 78: goto op_INSTANCEOF;
    case 79: goto op_RETURN;
    case 80: goto op_HALT;
    case 81: goto op_THROW_ABSTRACT;
    case 82: goto op_REGISTER_RETURN_CLEANUP;
    case 83: goto op_JMP_TO_OUTER_CLEANUP;
    case 84: goto op_THROW_EXCEPTION;
    case 85: goto op_GET_EXCEPTION;
    case 86: goto op_CLEAR_EXCEPTION;
    default: goto op_HALT;
  }
#else
  static const void *dispatch_table[] = {
      &&op_PUSH_CONST_I8,
      &&op_PUSH_CONST_I16,
      &&op_PUSH_CONST_I32,
      &&op_PUSH_CONST_I64,
      &&op_PUSH_CONST_U8,
      &&op_PUSH_CONST_U16,
      &&op_PUSH_CONST_U32,
      &&op_PUSH_CONST_U64,
      &&op_PUSH_CONST_F32,
      &&op_PUSH_CONST_F64,
      &&op_PUSH_CONST_STRING,
      &&op_PUSH_TRUE,
      &&op_PUSH_FALSE,
      &&op_PUSH_NULL,
      &&op_POP,
      &&op_DUP,
      &&op_DUP2,
      &&op_ADD_I64,
      &&op_ADD_F64,
      &&op_SUB_I64,
      &&op_SUB_F64,
      &&op_MUL_I64,
      &&op_MUL_F64,
      &&op_DIV_I64,
      &&op_DIV_F64,
      &&op_MOD_I64,
      &&op_EQ_I64,
      &&op_EQ_F64,
      &&op_NEQ_I64,
      &&op_NEQ_F64,
      &&op_GREATER_I64,
      &&op_GREATER_F64,
      &&op_GREATER_EQ_I64,
      &&op_GREATER_EQ_F64,
      &&op_LESS_I64,
      &&op_LESS_F64,
      &&op_LESS_EQ_I64,
      &&op_LESS_EQ_F64,
      &&op_LOGICAL_NOT,
      &&op_NEGATE,
      &&op_INC_I64,
      &&op_INC_F64,
      &&op_DEC_I64,
      &&op_DEC_F64,
      &&op_GET_LOCAL,
      &&op_SET_LOCAL,
      &&op_GET_GLOBAL,
      &&op_SET_GLOBAL,
      &&op_JUMP,
      &&op_JUMP_IF_FALSE,
      &&op_JUMP_IF_TRUE,
      &&op_ALLOC_STATIC,
      &&op_ALLOC_DYNAMIC,
      &&op_GET_PROPERTY,
      &&op_SET_PROPERTY,
      &&op_WEAK_SET_PROPERTY,
      &&op_GET_ARRAY,
      &&op_SET_ARRAY,
      &&op_ARRAY_LENGTH,
      &&op_INC_REF,
      &&op_DEC_REF,
      &&op_CONV_I8,
      &&op_CONV_I16,
      &&op_CONV_I32,
      &&op_CONV_I64,
      &&op_CONV_U8,
      &&op_CONV_U16,
      &&op_CONV_U32,
      &&op_CONV_U64,
      &&op_CONV_F32,
      &&op_CONV_F64,
      &&op_CALL,
      &&op_CALL_NATIVE,
      &&op_DEFINE_NATIVE,
      &&op_CALL_VIRTUAL,
      &&op_DEFINE_VTABLE,
      &&op_SET_VTABLE,
      &&op_CAST_CHECK,
      &&op_INSTANCEOF,
      &&op_RETURN,
      &&op_HALT,
      &&op_THROW_ABSTRACT,
      &&op_REGISTER_RETURN_CLEANUP,
      &&op_JMP_TO_OUTER_CLEANUP,
      &&op_THROW_EXCEPTION,
      &&op_GET_EXCEPTION,
      &&op_CLEAR_EXCEPTION
  };

#define DISPATCH() goto *dispatch_table[code[program_counter++]]

  DISPATCH();
#endif
op_PUSH_CONST_I8:
op_PUSH_CONST_I16:
op_PUSH_CONST_I32:
  {
    PUSH(static_cast<uint64_t>(read_u32(bytecode, program_counter)));
    DISPATCH();
  }
op_PUSH_CONST_I64:
op_PUSH_CONST_U64:
op_PUSH_CONST_F64:
  {
    PUSH(read_u64(bytecode, program_counter));
    DISPATCH();
  }
op_PUSH_CONST_U8:
op_PUSH_CONST_U16:
op_PUSH_CONST_U32:
op_PUSH_CONST_F32:
  {
    PUSH(static_cast<uint64_t>(read_u32(bytecode, program_counter)));
    DISPATCH();
  }
op_PUSH_CONST_STRING:
  {
    { // inner scope to avoid goto-over-destructor
      std::string str = read_string(bytecode, program_counter);
      size_t len = str.length();
      Address addr = memory.dynamic_allocation(len);
      for (size_t i = 0; i < len; ++i)
        heap_data[addr + i] = static_cast<uint64_t>(str[i]);
      PUSH(addr);
    }
    DISPATCH();
  }
op_PUSH_TRUE:
  PUSH(1);
  DISPATCH();
op_PUSH_FALSE:
  PUSH(0);
  DISPATCH();
op_PUSH_NULL:
  PUSH(0);
  DISPATCH();
op_POP:
  POP();
  DISPATCH();
op_DUP:
  {
    uint64_t top = PEEK();
    PUSH(top);
    DISPATCH();
  }
op_DUP2:
  {
    uint64_t b = *(sp - 1);
    uint64_t a = *(sp - 2);
    PUSH(a);
    PUSH(b);
    DISPATCH();
  }

op_ADD_I64:
  {
    int64_t b = static_cast<int64_t>(POP());
    int64_t a = static_cast<int64_t>(POP());
    PUSH(static_cast<uint64_t>(a + b));
    DISPATCH();
  }
op_ADD_F64:
  {
    double b = bit_cast_from_u64<double>(POP());
    double a = bit_cast_from_u64<double>(POP());
    PUSH(bit_cast_to_u64(a + b));
    DISPATCH();
  }
op_SUB_I64:
  {
    int64_t b = static_cast<int64_t>(POP());
    int64_t a = static_cast<int64_t>(POP());
    PUSH(static_cast<uint64_t>(a - b));
    DISPATCH();
  }
op_SUB_F64:
  {
    double b = bit_cast_from_u64<double>(POP());
    double a = bit_cast_from_u64<double>(POP());
    PUSH(bit_cast_to_u64(a - b));
    DISPATCH();
  }
op_MUL_I64:
  {
    int64_t b = static_cast<int64_t>(POP());
    int64_t a = static_cast<int64_t>(POP());
    PUSH(static_cast<uint64_t>(a * b));
    DISPATCH();
  }
op_MUL_F64:
  {
    double b = bit_cast_from_u64<double>(POP());
    double a = bit_cast_from_u64<double>(POP());
    PUSH(bit_cast_to_u64(a * b));
    DISPATCH();
  }
op_DIV_I64:
  {
    int64_t b = static_cast<int64_t>(POP());
    int64_t a = static_cast<int64_t>(POP());
    PUSH(static_cast<uint64_t>(a / b));
    DISPATCH();
  }
op_DIV_F64:
  {
    double b = bit_cast_from_u64<double>(POP());
    double a = bit_cast_from_u64<double>(POP());
    PUSH(bit_cast_to_u64(a / b));
    DISPATCH();
  }
op_MOD_I64:
  {
    int64_t b = static_cast<int64_t>(POP());
    int64_t a = static_cast<int64_t>(POP());
    PUSH(static_cast<uint64_t>(a % b));
    DISPATCH();
  }
op_EQ_I64:
  {
    uint64_t b = POP();
    uint64_t a = POP();
    PUSH(a == b ? 1 : 0);
    DISPATCH();
  }
op_EQ_F64:
  {
    double b = bit_cast_from_u64<double>(POP());
    double a = bit_cast_from_u64<double>(POP());
    PUSH(a == b ? 1 : 0);
    DISPATCH();
  }
op_NEQ_I64:
  {
    uint64_t b = POP();
    uint64_t a = POP();
    PUSH(a != b ? 1 : 0);
    DISPATCH();
  }
op_NEQ_F64:
  {
    double b = bit_cast_from_u64<double>(POP());
    double a = bit_cast_from_u64<double>(POP());
    PUSH(a != b ? 1 : 0);
    DISPATCH();
  }
op_GREATER_I64:
  {
    int64_t b = static_cast<int64_t>(POP());
    int64_t a = static_cast<int64_t>(POP());
    PUSH(a > b ? 1 : 0);
    DISPATCH();
  }
op_GREATER_F64:
  {
    double b = bit_cast_from_u64<double>(POP());
    double a = bit_cast_from_u64<double>(POP());
    PUSH(a > b ? 1 : 0);
    DISPATCH();
  }
op_GREATER_EQ_I64:
  {
    int64_t b = static_cast<int64_t>(POP());
    int64_t a = static_cast<int64_t>(POP());
    PUSH(a >= b ? 1 : 0);
    DISPATCH();
  }
op_GREATER_EQ_F64:
  {
    double b = bit_cast_from_u64<double>(POP());
    double a = bit_cast_from_u64<double>(POP());
    PUSH(a >= b ? 1 : 0);
    DISPATCH();
  }
op_LESS_I64:
  {
    int64_t b = static_cast<int64_t>(POP());
    int64_t a = static_cast<int64_t>(POP());
    PUSH(a < b ? 1 : 0);
    DISPATCH();
  }
op_LESS_F64:
  {
    double b = bit_cast_from_u64<double>(POP());
    double a = bit_cast_from_u64<double>(POP());
    PUSH(a < b ? 1 : 0);
    DISPATCH();
  }
op_LESS_EQ_I64:
  {
    int64_t b = static_cast<int64_t>(POP());
    int64_t a = static_cast<int64_t>(POP());
    PUSH(a <= b ? 1 : 0);
    DISPATCH();
  }
op_LESS_EQ_F64:
  {
    double b = bit_cast_from_u64<double>(POP());
    double a = bit_cast_from_u64<double>(POP());
    PUSH(a <= b ? 1 : 0);
    DISPATCH();
  }
op_LOGICAL_NOT:
  {
    uint64_t a = POP();
    PUSH(a == 0 ? 1 : 0);
    DISPATCH();
  }
op_NEGATE:
  {
    double a = bit_cast_from_u64<double>(POP());
    PUSH(bit_cast_to_u64(-a));
    DISPATCH();
  }
op_INC_I64:
  {
    uint64_t a = POP();
    PUSH(a + 1);
    DISPATCH();
  }
op_INC_F64:
  {
    double a = bit_cast_from_u64<double>(POP());
    PUSH(bit_cast_to_u64(a + 1.0));
    DISPATCH();
  }
op_DEC_I64:
  {
    uint64_t a = POP();
    PUSH(a - 1);
    DISPATCH();
  }
op_DEC_F64:
  {
    double a = bit_cast_from_u64<double>(POP());
    PUSH(bit_cast_to_u64(a - 1.0));
    DISPATCH();
  }

op_GET_LOCAL:
  {
    uint32_t index = read_u32(bytecode, program_counter);
    uint32_t fp = call_stack[call_depth - 1].frame_pointer;
    if (fp + index >= memory.stack.size())
      throw std::runtime_error("Frame out of bounds on GET_LOCAL");
    PUSH(stack[fp + index]);
    DISPATCH();
  }
op_SET_LOCAL:
  {
    uint32_t index = read_u32(bytecode, program_counter);
    uint32_t fp = call_stack[call_depth - 1].frame_pointer;
    if (fp + index >= memory.stack.size())
      throw std::runtime_error("Frame out of bounds on SET_LOCAL fp=" + std::to_string(fp) + " index=" + std::to_string(index) + " size=" + std::to_string(memory.stack.size()));
    stack[fp + index] = POP();
    DISPATCH();
  }
op_GET_GLOBAL:
  {
    uint32_t idx = read_u32(bytecode, program_counter);
    PUSH(heap_data[idx]);
    DISPATCH();
  }
op_SET_GLOBAL:
  {
    uint32_t idx = read_u32(bytecode, program_counter);
    uint64_t val = POP();
    heap_data[idx] = val;
    DISPATCH();
  }

op_JUMP:
  {
    uint32_t addr = read_u32(bytecode, program_counter);
    program_counter = addr;
    DISPATCH();
  }
op_JUMP_IF_FALSE:
  {
    uint32_t addr = read_u32(bytecode, program_counter);
    uint64_t cond = POP();
    if (cond == 0)
      program_counter = addr;
    DISPATCH();
  }
op_JUMP_IF_TRUE:
  {
    uint32_t addr = read_u32(bytecode, program_counter);
    uint64_t cond = POP();
    if (cond != 0)
      program_counter = addr;
    DISPATCH();
  }

op_ALLOC_STATIC:
  {
    uint32_t size = static_cast<uint32_t>(POP());
    memory.static_allocation(size, 0);
    DISPATCH();
  }
op_ALLOC_DYNAMIC:
  {
    uint32_t size = static_cast<uint32_t>(POP());
    PUSH(memory.dynamic_allocation(size));
    DISPATCH();
  }
op_GET_PROPERTY:
  {
    uint32_t offset = read_u32(bytecode, program_counter);
    Address obj = static_cast<Address>(POP());
    if (obj == 0) throw std::runtime_error("NullPointer");
    if (obj + offset >= memory.heap.size())
      throw std::runtime_error("Heap out of bounds on GET_PROPERTY");
    PUSH(memory.read_u64(obj, offset));
    DISPATCH();
  }
op_SET_PROPERTY:
  {
    uint32_t offset = read_u32(bytecode, program_counter);
    Address obj = static_cast<Address>(POP());
    uint64_t val = POP();
    if (obj == 0) throw std::runtime_error("NullPointer");
    if (obj + offset >= memory.heap.size())
      throw std::runtime_error("Heap out of bounds on SET_PROPERTY");

    memory.write_u64(obj, offset, val);
    DISPATCH();
  }
op_WEAK_SET_PROPERTY:
  {
    uint32_t offset = read_u32(bytecode, program_counter);
    Address obj = static_cast<Address>(POP());
    uint64_t val = POP();
    if (obj == 0) throw std::runtime_error("NullPointer");
    if (obj + offset >= memory.heap.size())
      throw std::runtime_error("Heap out of bounds on WEAK_SET_PROPERTY");

    Address slot_address = obj + offset;
    Address old_val = memory.read_u64(obj, offset);

    if (old_val != 0) {
      if (auto it = memory.weak_references.find(old_val);
          it != memory.weak_references.end()) {
        it->second.erase(slot_address);
        if (it->second.empty())
          memory.weak_references.erase(it);
      }
    }

    memory.write_u64(obj, offset, val);

    if (val != 0) {
      memory.weak_references[val].insert(slot_address);
    }
    DISPATCH();
  }
op_GET_ARRAY:
  {
    uint32_t index = static_cast<uint32_t>(POP());
    Address array_addr = static_cast<Address>(POP());
    if (array_addr == 0) throw std::runtime_error("NullPointer");
    uint32_t length = static_cast<uint32_t>(heap_data[array_addr - 1] >> 32);
    if (index >= length) throw std::runtime_error("Out of Bounds on GET_ARRAY: index " + std::to_string(index) + " >= length " + std::to_string(length));
    PUSH(heap_data[array_addr + index]);
    DISPATCH();
  }
op_SET_ARRAY:
  {
    uint64_t val = POP();
    uint32_t index = static_cast<uint32_t>(POP());
    Address array_addr = static_cast<Address>(POP());
    if (array_addr == 0) throw std::runtime_error("NullPointer");
    uint32_t length = static_cast<uint32_t>(heap_data[array_addr - 1] >> 32);
    if (index >= length) {
      throw std::runtime_error("Out of Bounds on SET_ARRAY at PC=" + std::to_string(program_counter) + ": array_addr=" + std::to_string(array_addr) + " index=" + std::to_string(index) + " >= length=" + std::to_string(length) + " (header_val=" + std::to_string(heap_data[array_addr - 1]) + ", val=" + std::to_string(val) + ")");
    }
    heap_data[array_addr + index] = val;
    PUSH(val);
    DISPATCH();
  }
op_ARRAY_LENGTH:
  {
    Address array_addr = static_cast<Address>(POP());
    if (array_addr == 0) throw std::runtime_error("NullPointer");
    uint32_t length = static_cast<uint32_t>(heap_data[array_addr - 1] >> 32);
    PUSH(length);
    DISPATCH();
  }

op_INC_REF:
  {
    Address addr = static_cast<Address>(POP());
    memory.increase_reference(addr);
    PUSH(addr);
    DISPATCH();
  }
op_DEC_REF:
  {
    Address addr = static_cast<Address>(POP());
    memory.decrease_reference(addr);
    DISPATCH();
  }

op_CONV_I8:
op_CONV_I16:
op_CONV_I32:
op_CONV_I64:
op_CONV_U8:
op_CONV_U16:
op_CONV_U32:
op_CONV_U64:
op_CONV_F32:
op_CONV_F64:
  {
    // Everything is stored as 64-bit float/int natively, pass through for now
    DISPATCH();
  }

op_CALL:
  {
    uint32_t arg_count = static_cast<uint32_t>(POP());
    uint32_t frame_size = static_cast<uint32_t>(POP());
    uint32_t target_ip = static_cast<uint32_t>(POP());

    uint32_t current_sp_idx = static_cast<uint32_t>(sp - stack);
    uint32_t new_frame_pointer = current_sp_idx - arg_count;
    if (call_depth >= 65536)
      throw std::runtime_error("Stack overflow: max call depth exceeded");
    call_stack[call_depth++] = Frame(program_counter, new_frame_pointer);

    if (frame_size > arg_count) {
      sp += (frame_size - arg_count);
    }

    program_counter = target_ip;
    DISPATCH();
  }
op_CALL_NATIVE:
  {
    uint32_t id = read_u32(bytecode, program_counter);
    uint32_t arg_count = read_u32(bytecode, program_counter);
    bool is_static = bytecode[program_counter++] != 0;

    {
        std::vector<uint64_t> args(arg_count);
        for (int i = static_cast<int>(arg_count) - 1; i >= 0; --i) {
            args[i] = POP();
        }

        uint64_t self_address = 0;
        if (!is_static) {
            self_address = POP();
        }

        SYNC_SP();
        if (native_registry.count(id)) {
            uint64_t result = native_registry[id](*this, self_address, args.data(), arg_count);
            RESTORE_SP();
            PUSH(result);
        } else {
            throw std::runtime_error("Call to unknown native function: " + std::to_string(id));
        }
    }
    DISPATCH();
  }

op_DEFINE_VTABLE:
  {
    {
      uint32_t vtable_id = read_u32(bytecode, program_counter);
      int32_t base_vtable_id =
          static_cast<int32_t>(read_u32(bytecode, program_counter));
      uint32_t size = read_u32(bytecode, program_counter);
      std::vector<uint32_t> vtable(size);
      for (uint32_t i = 0; i < size; ++i) {
        vtable[i] = read_u32(bytecode, program_counter);
      }
      vtables[vtable_id] = std::move(vtable);
      vtable_bases[vtable_id] = base_vtable_id;
    }
    DISPATCH();
  }
op_INSTANCEOF:
  {
    uint32_t target_vtable_id = read_u32(bytecode, program_counter);
    Address obj = static_cast<Address>(*(sp - 1));
    --sp;

    bool is_instance = false;
    if (obj != 0) {
      int32_t current_vtable = static_cast<int32_t>(heap_data[obj]);
      while (current_vtable != -1) {
        if (current_vtable == target_vtable_id) {
          is_instance = true;
          break;
        }
        current_vtable = vtable_bases.count(current_vtable)
                             ? vtable_bases[current_vtable]
                             : -1;
      }
    }

    PUSH(is_instance ? 1 : 0);
    DISPATCH();
  }
op_CAST_CHECK:
  {
    uint32_t target_vtable_id = read_u32(bytecode, program_counter);
    Address obj = static_cast<Address>(*(sp - 1));

    if (obj != 0) {
      int32_t current_vtable = static_cast<int32_t>(heap_data[obj]);
      bool is_instance = false;
      while (current_vtable != -1) {
        if (current_vtable == target_vtable_id) {
          is_instance = true;
          break;
        }
        current_vtable = vtable_bases.count(current_vtable)
                             ? vtable_bases[current_vtable]
                             : -1;
      }
      if (!is_instance) {
        throw std::runtime_error("Invalid cast exception at runtime");
      }
    }
    DISPATCH();
  }
op_SET_VTABLE:
  {
    uint32_t vtable_id = read_u32(bytecode, program_counter);
    Address obj = static_cast<Address>(*(sp - 1));
    heap_data[obj] = vtable_id;
    DISPATCH();
  }
op_CALL_VIRTUAL:
  {
    uint32_t vtable_index = read_u32(bytecode, program_counter);
    uint32_t frame_size = read_u32(bytecode, program_counter);
    uint32_t arg_count = read_u32(bytecode, program_counter);

    uint32_t current_sp_idx = static_cast<uint32_t>(sp - stack);
    Address obj = static_cast<Address>(stack[current_sp_idx - arg_count]);
    if (obj == 0) throw std::runtime_error("NullPointer");
    uint32_t vtable_id = heap_data[obj];
    if (vtables.find(vtable_id) == vtables.end() ||
        vtable_index >= vtables[vtable_id].size()) {
      throw std::runtime_error("Virtual method resolution failed!");
    }
    uint32_t target_ip = vtables[vtable_id][vtable_index];

    uint32_t new_frame_pointer = current_sp_idx - arg_count;
    if (call_depth >= 65536)
      throw std::runtime_error("Stack overflow: max call depth exceeded");
    call_stack[call_depth++] = Frame(program_counter, new_frame_pointer);
    program_counter = target_ip;

    if (frame_size > arg_count) {
      sp += (frame_size - arg_count);
    }
    DISPATCH();
  }
op_DEFINE_NATIVE:
  {
    {
      uint32_t id = read_u32(bytecode, program_counter);
      std::string name = read_string(bytecode, program_counter);
      if (options.native_functions.count(name)) {
        native_registry[id] = options.native_functions[name];
      } else {
        std::cerr << "Warning: Native function " << name << " not found."
                  << std::endl;
      }
    }
    DISPATCH();
  }
op_RETURN:
  {
    uint64_t ret_val = 0;
    uint32_t current_sp_idx = static_cast<uint32_t>(sp - stack);
    const Frame &current_frame = call_stack[call_depth - 1];
    if (current_sp_idx > current_frame.frame_pointer) {
      ret_val = POP();
    }

    Frame frame = current_frame;
    --call_depth;
    sp = stack + frame.frame_pointer;

    if (call_depth > 0) {
      PUSH(ret_val);
      program_counter = frame.return_ip;
    } else {
      SYNC_SP();
      return;
    }
    DISPATCH();
  }
op_HALT:
  SYNC_SP();
  return;
op_THROW_ABSTRACT:
  throw std::runtime_error("Called abstract method");

op_REGISTER_RETURN_CLEANUP:
  {
      Address ret_ip = read_u32(bytecode, program_counter);
      Address cleanup_ip = read_u32(bytecode, program_counter);
      return_to_cleanup[ret_ip] = cleanup_ip;
      DISPATCH();
  }
op_JMP_TO_OUTER_CLEANUP:
  {
      if (call_depth == 0) {
          throw std::runtime_error("Unhandled exception reached top level.");
      }
      Frame current_frame = call_stack[call_depth - 1];
      Address ret_ip = current_frame.return_ip;
      
      call_depth--;
      sp = stack + current_frame.frame_pointer; // Pop locals
      
      if (call_depth == 0) {
          throw std::runtime_error("Unhandled exception reached top level.");
      }
      
      auto it = return_to_cleanup.find(ret_ip);
      if (it != return_to_cleanup.end()) {
          program_counter = it->second;
      } else {
          // If the caller has NO cleanup, just loop JMP_TO_OUTER_CLEANUP again!
          // We can simulate this by putting program_counter just before a JMP_TO_OUTER_CLEANUP
          // Or just recursively pop frames.
          while (true) {
              if (call_depth == 0) throw std::runtime_error("Unhandled exception reached top level.");
              Frame caller = call_stack[call_depth - 1];
              auto it2 = return_to_cleanup.find(caller.return_ip);
              if (it2 != return_to_cleanup.end()) {
                  program_counter = it2->second;
                  break;
              }
              call_depth--;
              sp = stack + caller.frame_pointer;
          }
      }
      DISPATCH();
  }
op_THROW_EXCEPTION:
  {
      uint64_t exc = POP();
      if (exc != 0) {
          memory.increase_reference(exc); // Prevent it from being garbage collected during unwinding
          active_exception = exc;
      }
      Address innermost_cleanup_ip = read_u32(bytecode, program_counter);
      
      if (innermost_cleanup_ip != 0xFFFFFFFF) {
          program_counter = innermost_cleanup_ip;
      } else {
          // No local cleanup, immediately trigger cross-frame bubbling
          goto op_JMP_TO_OUTER_CLEANUP;
      }
      DISPATCH();
  }
op_GET_EXCEPTION:
  {
      PUSH(active_exception);
      DISPATCH();
  }
op_CLEAR_EXCEPTION:
  {
      if (active_exception != 0) {
          memory.decrease_reference(active_exception);
          active_exception = 0;
      }
      DISPATCH();
  }
#undef PUSH
#undef POP
#undef PEEK
#undef SYNC_SP
#undef RESTORE_SP
#undef DISPATCH
}

void run(RuntimeOptions &options) {
  RuntimeContext vm(options);
  vm.execute();
}
} // namespace solix
