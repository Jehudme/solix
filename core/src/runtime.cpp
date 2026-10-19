#include "solix/runtime.hpp"
#include "solix/native_registry.hpp"
#include "utilities/optcodes.hpp"
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace solix {

uint64_t Memory::static_allocation(size_t size_in_bytes, Address address) {
  if (address == 0) {
    address = (next_free_static + 7) & ~7;
    next_free_static = address + static_cast<Address>(size_in_bytes);
    Address dynamic_start = (static_cast<Address>(heap.size() / 2) + 7) & ~7;
    if (next_free_static >= dynamic_start) {
      throw std::runtime_error("Heap overflow: static memory exceeded boundary into dynamic heap!");
    }
  }
  currently_used_words += ((size_in_bytes + 7) / 8);
  if (currently_used_words > peak_used_words)
    peak_used_words = currently_used_words;
  return address;
}

void Memory::deallocate(Address address) {
  if (address == 0)
    return;

  if (auto it = weak_references.find(address); it != weak_references.end()) {
    for (Address weak_slot : it->second) {
      write_u64(weak_slot, 0, 0);
    }
    weak_references.erase(it);
  }

  Address header_addr = address - 8;
  uint32_t count = read_u32(header_addr, 0);
  uint32_t tag_and_ref = read_u32(header_addr, 4);
  BlockTag tag = static_cast<BlockTag>((tag_and_ref >> 28) & 0xF);

  size_t payload_bytes = count;
  if (tag == BlockTag::ARRAY_U8 || tag == BlockTag::ARRAY_I8) payload_bytes = count * 1;
  else if (tag == BlockTag::ARRAY_U16 || tag == BlockTag::ARRAY_I16) payload_bytes = count * 2;
  else if (tag == BlockTag::ARRAY_U32 || tag == BlockTag::ARRAY_I32) payload_bytes = count * 4;
  else if (tag == BlockTag::ARRAY_U64 || tag == BlockTag::ARRAY_REF || tag == BlockTag::CLOSURE) payload_bytes = count * 8;

  size_t aligned_payload = (payload_bytes + 7) & ~7;
  if (aligned_payload == 0) aligned_payload = 8;
  size_t total_block_bytes = aligned_payload + 8;

  currently_used_words -= (total_block_bytes / 8);
  if (live_objects_count > 0) {
    live_objects_count--;
  }

  write_u32(header_addr, 4, 0); // tag RAW, ref 0
  uint32_t free_payload = static_cast<uint32_t>(aligned_payload);

  // Coalesce with adjacent next free block
  Address next_addr = header_addr + 8 + free_payload;
  auto next_it = free_by_addr.find(next_addr);
  if (next_it != free_by_addr.end()) {
    uint32_t next_payload = next_it->second;
    auto range = free_by_size.equal_range(next_payload);
    for (auto it = range.first; it != range.second; ++it) {
      if (it->second == next_addr) {
        free_by_size.erase(it);
        break;
      }
    }
    free_by_addr.erase(next_it);
    free_payload += 8 + next_payload;
  }

  // Coalesce with adjacent previous free block
  auto prev_it = free_by_addr.lower_bound(header_addr);
  if (prev_it != free_by_addr.begin()) {
    --prev_it;
    Address prev_addr = prev_it->first;
    uint32_t prev_payload = prev_it->second;
    if (prev_addr + 8 + prev_payload == header_addr) {
      auto range = free_by_size.equal_range(prev_payload);
      for (auto it = range.first; it != range.second; ++it) {
        if (it->second == prev_addr) {
          free_by_size.erase(it);
          break;
        }
      }
      free_by_addr.erase(prev_it);
      header_addr = prev_addr;
      free_payload += 8 + prev_payload;
    }
  }

  // If the coalesced block is at the top of dynamic heap, retract next_free_dynamic
  if (header_addr + 8 + free_payload == next_free_dynamic) {
    next_free_dynamic = header_addr;
  } else {
    write_u32(header_addr, 0, free_payload);
    free_by_addr[header_addr] = free_payload;
    free_by_size.insert({free_payload, header_addr});
  }

  sync_free_blocks();
}

void Memory::increase_reference(Address address) {
  if (address < 8 || address >= heap.size())
    return;
  Address header_addr = address - 8;
  uint32_t tag_and_ref = read_u32(header_addr, 4);
  uint32_t ref_count = (tag_and_ref & 0x0FFFFFFF) + 1;
  tag_and_ref = (tag_and_ref & 0xF0000000) | (ref_count & 0x0FFFFFFF);
  write_u32(header_addr, 4, tag_and_ref);
}

void Memory::decrease_reference(Address address) {
  if (address < 8 || address >= heap.size())
    return;
  Address header_addr = address - 8;
  uint32_t tag_and_ref = read_u32(header_addr, 4);
  uint32_t ref_count = tag_and_ref & 0x0FFFFFFF;
  if (ref_count > 0) {
    ref_count--;
    tag_and_ref = (tag_and_ref & 0xF0000000) | (ref_count & 0x0FFFFFFF);
    write_u32(header_addr, 4, tag_and_ref);
    if (ref_count == 0) {
      BlockTag tag = static_cast<BlockTag>((tag_and_ref >> 28) & 0xF);
      if (tag == BlockTag::CLOSURE) {
        uint64_t ref_mask = read_u64(address, 0);
        uint64_t callable_mask = read_u64(address, 8);
        uint32_t count = read_u32(header_addr, 0);
        uint32_t cap_count = count >= 2 ? count - 2 : 0;
        for (uint32_t i = 0; i < cap_count; ++i) {
          if ((ref_mask >> i) & 1) {
            decrease_reference(static_cast<Address>(read_u64(address, (2 + i) * 8)));
          } else if ((callable_mask >> i) & 1) {
            uint64_t val = read_u64(address, (2 + i) * 8);
            Address nested_env = static_cast<Address>(val >> 32);
            decrease_reference_callable(nested_env);
          }
        }
      } else if (object_destructor) {
        object_destructor(*this, address);
      }
      if (get_block_ref_count(address) == 0) {
        deallocate(address);
      }
    }
  }
}

void Memory::decrease_reference_callable(Address env) {
  if (env < 8 || env >= heap.size())
    return;
  Address header_addr = env - 8;
  uint32_t tag_and_ref = read_u32(header_addr, 4);
  uint32_t ref_count = tag_and_ref & 0x0FFFFFFF;
  if (ref_count > 0) {
    ref_count--;
    tag_and_ref = (tag_and_ref & 0xF0000000) | (ref_count & 0x0FFFFFFF);
    write_u32(header_addr, 4, tag_and_ref);
    if (ref_count == 0) {
      uint64_t ref_mask = read_u64(env, 0);
      uint64_t callable_mask = read_u64(env, 8);
      uint32_t count = read_u32(header_addr, 0);
      uint32_t cap_count = count >= 2 ? count - 2 : 0;
      for (uint32_t i = 0; i < cap_count; ++i) {
        if ((ref_mask >> i) & 1) {
          decrease_reference(static_cast<Address>(read_u64(env, (2 + i) * 8)));
        } else if ((callable_mask >> i) & 1) {
          uint64_t val = read_u64(env, (2 + i) * 8);
          Address nested_env = static_cast<Address>(val >> 32);
          decrease_reference_callable(nested_env);
        }
      }
      deallocate(env);
    }
  }
}

RuntimeContext::RuntimeContext(const RuntimeOptions &opts)
    : options(opts), memory(opts.stack_capacity, opts.heap_capacity),
      resource_directory(opts.resource_directory),
      instruction_budget(opts.instruction_budget), instructions_executed(0) {
  call_stack.resize(65536);

  memory.object_destructor = [this](Memory &mem, Address address) {
    if (address == 0 || address >= mem.heap.size()) return;
    BlockTag tag = mem.get_block_tag(address);
    if (tag == BlockTag::OBJECT) {
      uint32_t vtable_id = mem.read_u32(address, 0);
      auto dtor_it = vtable_destructors.find(vtable_id);
      if (dtor_it != vtable_destructors.end() && dtor_it->second != 0) {
        if (active_destructors.insert(address).second) {
          Address header_addr = address - 8;
          uint32_t tag_and_ref = mem.read_u32(header_addr, 4);
          mem.write_u32(header_addr, 4, (tag_and_ref & 0xF0000000) | 1U);

          invoke_destructor(address, dtor_it->second);

          active_destructors.erase(address);

          tag_and_ref = mem.read_u32(header_addr, 4);
          uint32_t cur_ref = tag_and_ref & 0x0FFFFFFF;
          if (cur_ref > 1) {
            mem.write_u32(header_addr, 4, (tag_and_ref & 0xF0000000) | (cur_ref - 1));
            return;
          } else {
            mem.write_u32(header_addr, 4, (tag_and_ref & 0xF0000000) | 0U);
          }
        }
      }
      if (vtable_ref_fields.count(vtable_id)) {
        const auto &fields = vtable_ref_fields[vtable_id];
        for (uint32_t offset : fields) {
          if (address + offset + 8 <= mem.heap.size()) {
            Address child_addr = static_cast<Address>(mem.read_u64(address, offset));
            if (child_addr != 0) {
              mem.write_u64(address, offset, 0);
              mem.decrease_reference(child_addr);
            }
          }
        }
      }
    } else if (tag == BlockTag::ARRAY_REF) {
      uint32_t len = mem.get_array_length(address);
      for (uint32_t i = 0; i < len; ++i) {
        if (address + i * 8 + 8 <= mem.heap.size()) {
          Address child_addr = static_cast<Address>(mem.read_u64(address, i * 8));
          if (child_addr != 0) {
            mem.write_u64(address, i * 8, 0);
            mem.decrease_reference(child_addr);
          }
        }
      }
    } else if (tag == BlockTag::TUPLE) {
      uint32_t count = mem.get_array_length(address);
      Address types_offset = 0;
      Address values_offset = ((count * 4 + 7) & ~7);
      for (uint32_t i = 0; i < count; ++i) {
        uint32_t type_tag = mem.read_u32(address + types_offset, i * 4);
        if (type_tag & 0x80000000) {
          if (address + values_offset + i * 8 + 8 <= mem.heap.size()) {
            Address child_addr = static_cast<Address>(mem.read_u64(address + values_offset, i * 8));
            if (child_addr != 0) {
              mem.write_u64(address + values_offset, i * 8, 0);
              mem.decrease_reference(child_addr);
            }
          }
        }
      }
    }
  };

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

  if (!options.sandboxed) {
    for (const auto &lib_path : options.native_libraries) {
      NativeRegistry::global().load_library(lib_path);
    }
  }
}

void RuntimeContext::register_native(uint32_t id, NativeFunctionPtr func) {
  if (id >= native_table.size()) {
    native_table.resize(id + 1, nullptr);
  }
  native_table[id] = func;
  native_registry[id] = func;
}

void RuntimeContext::on_budget_exceeded() {
  exit_code = 75; // EX_TEMPFAIL
}

void RuntimeContext::on_out_of_memory(size_t requested_bytes) {
  (void)requested_bytes;
  exit_code = 71; // EX_OSERR
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
  if (pc + 4 > bcode.size()) {
    throw std::runtime_error("Malformed bytecode: unexpected EOF reading uint32 at pc=" + std::to_string(pc));
  }
  uint32_t val = (bcode[pc] << 24) | (bcode[pc + 1] << 16) |
                 (bcode[pc + 2] << 8) | bcode[pc + 3];
  pc += 4;
  return val;
}

static inline uint64_t read_u64(const Bytecode &bcode, Address &pc) {
  if (pc + 8 > bcode.size()) {
    throw std::runtime_error("Malformed bytecode: unexpected EOF reading uint64 at pc=" + std::to_string(pc));
  }
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
  if (pc + length > bcode.size()) {
    throw std::runtime_error("Malformed bytecode: unexpected EOF reading string at pc=" + std::to_string(pc));
  }
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

static thread_local size_t g_last_live_object_count = 0;
static thread_local Memory *g_active_memory = nullptr;

size_t get_live_object_count() {
  if (g_active_memory) {
    return g_active_memory->live_objects_count;
  }
  return g_last_live_object_count;
}

void RuntimeContext::invoke_destructor(Address obj_addr, uint32_t dtor_ip) {
  if (dtor_ip == 0 || obj_addr == 0) return;

  Address saved_pc = program_counter;
  Address saved_closure_env = active_closure_env;
  size_t saved_call_depth = call_depth;

  push(obj_addr);

  uint32_t new_frame_pointer = memory.stack_pointer - 1;
  call_stack[call_depth++] = Frame(0, new_frame_pointer, 1, 0);
  program_counter = dtor_ip;
  active_closure_env = 0;

  run_bytecode(saved_call_depth);

  program_counter = saved_pc;
  active_closure_env = saved_closure_env;
}

void RuntimeContext::execute() {
  struct MemoryScopeGuard {
    Memory &mem;
    MemoryScopeGuard(Memory &m) : mem(m) {
      g_active_memory = &mem;
    }
    ~MemoryScopeGuard() {
      g_last_live_object_count = mem.live_objects_count;
      g_active_memory = nullptr;
    }
  };
  MemoryScopeGuard scope_guard(memory);

  // Add an initial frame so we don't underflow
  call_depth = 0;
  call_stack[call_depth++] = Frame(0, 0);

  size_t arg_count = options.program_args.size();
  Address args_array = memory.dynamic_allocation(arg_count, BlockTag::ARRAY_REF);
  for (size_t i = 0; i < arg_count; ++i) {
    const std::string &str = options.program_args[i];
    size_t len = str.length();
    Address str_addr = memory.dynamic_allocation(len, BlockTag::ARRAY_U8);
    for (size_t j = 0; j < len; ++j) {
      memory.write_u8(str_addr, static_cast<uint32_t>(j), static_cast<uint8_t>(str[j]));
    }
    memory.write_u64(args_array, static_cast<uint32_t>(i * 8), str_addr);
  }
  push(args_array);

  run_bytecode(0);
}

void RuntimeContext::run_bytecode(size_t stop_call_depth) {
  const uint8_t *code = bytecode.data();
  uint64_t *stack = memory.stack.data();

  uint64_t *sp = stack + memory.stack_pointer;

#define PUSH(val) do { \
  if (sp >= stack + memory.stack.size()) { \
    SYNC_SP(); \
    throw std::runtime_error("StackOverflowException: operand stack overflow"); \
  } \
  *sp++ = (val); \
} while (0)
#define POP() (sp <= stack ? (SYNC_SP(), throw std::runtime_error("StackUnderflowException: operand stack underflow"), 0ULL) : *--sp)
#define PEEK() (sp <= stack ? (SYNC_SP(), throw std::runtime_error("StackUnderflowException: operand stack underflow on PEEK"), 0ULL) : *(sp - 1))
#define SYNC_SP() (memory.stack_pointer = static_cast<uint32_t>(sp - stack))
#define RESTORE_SP() (sp = stack + memory.stack_pointer)

#if defined(_MSC_VER) && !defined(__clang__)
#define DISPATCH() goto vm_dispatch
  goto vm_dispatch;
vm_dispatch:
  if (program_counter >= bytecode.size()) goto op_HALT;
  instructions_executed++;
  if (instruction_budget > 0 && instructions_executed >= instruction_budget) {
    on_budget_exceeded();
    goto op_HALT;
  }
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
    case 87: goto op_CONV_I_TO_F;
    case 88: goto op_CONV_F_TO_I;
    case 89: goto op_NEGATE_I64;
    case 90: goto op_SIZEOF;
    case 91: goto op_ALLOC_FRAME;
    case 92: goto op_INC_REF_CALLABLE;
    case 93: goto op_DEC_REF_CALLABLE;
    case 94: goto op_UNPACK_CAPTURES;
    case 95: goto op_PACK_CLOSURE;
    case 96: goto op_CALL_INTERFACE;
    case 97: goto op_DEFINE_ITABLE;
    case 98: goto op_DEC_REF_SLOT;
    case 99: goto op_ASSERT;
    case 100: goto op_EXIT;
    case 101: goto op_DEFINE_DESTRUCTOR;
    case 102: goto op_NEW_TUPLE;
    case 103: goto op_TUPLE_GET;
    case 104: goto op_TUPLE_LEN;
    case 105: goto op_TUPLE_TYPE;
    case 106: goto op_TUPLE_CAST;
    case 107: goto op_TUPLE_INSTANCEOF;
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
      &&op_CLEAR_EXCEPTION,
      &&op_CONV_I_TO_F,
      &&op_CONV_F_TO_I,
      &&op_NEGATE_I64,
      &&op_SIZEOF,
      &&op_ALLOC_FRAME,
      &&op_INC_REF_CALLABLE,
      &&op_DEC_REF_CALLABLE,
      &&op_UNPACK_CAPTURES,
      &&op_PACK_CLOSURE,
      &&op_CALL_INTERFACE,
      &&op_DEFINE_ITABLE,
      &&op_DEC_REF_SLOT,
      &&op_ASSERT,
      &&op_EXIT,
      &&op_DEFINE_DESTRUCTOR,
      &&op_NEW_TUPLE,
      &&op_TUPLE_GET,
      &&op_TUPLE_LEN,
      &&op_TUPLE_TYPE,
      &&op_TUPLE_CAST,
      &&op_TUPLE_INSTANCEOF
  };

#define DISPATCH() do { \
  if (program_counter >= bytecode.size()) goto op_HALT; \
  uint8_t op = code[program_counter++]; \
  instructions_executed++; \
  if (instruction_budget > 0 && instructions_executed >= instruction_budget) { \
    on_budget_exceeded(); \
    goto op_HALT; \
  } \
  if (op >= sizeof(dispatch_table) / sizeof(dispatch_table[0])) goto op_HALT; \
  goto *dispatch_table[op]; \
} while (0)

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
      Address addr = memory.dynamic_allocation(len, BlockTag::ARRAY_U8);
      for (size_t i = 0; i < len; ++i)
        memory.write_u8(addr, static_cast<uint32_t>(i), static_cast<uint8_t>(str[i]));
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
    uint64_t b = POP();
    uint64_t a = POP();
    PUSH(a + b);
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
    uint64_t b = POP();
    uint64_t a = POP();
    PUSH(a - b);
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
    uint64_t b = POP();
    uint64_t a = POP();
    PUSH(a * b);
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
    if (b == 0) throw std::runtime_error("ArithmeticException: Division by zero");
    if (a == INT64_MIN && b == -1) {
      PUSH(static_cast<uint64_t>(INT64_MIN));
    } else {
      PUSH(static_cast<uint64_t>(a / b));
    }
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
    if (b == 0) throw std::runtime_error("ArithmeticException: Division by zero");
    if (a == INT64_MIN && b == -1) {
      PUSH(0);
    } else {
      PUSH(static_cast<uint64_t>(a % b));
    }
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
    PUSH(memory.read_u64(idx * 8, 0));
    DISPATCH();
  }
op_SET_GLOBAL:
  {
    uint32_t idx = read_u32(bytecode, program_counter);
    uint64_t val = POP();
    memory.write_u64(idx * 8, 0, val);
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
    memory.static_allocation(size * 8, 0);
    DISPATCH();
  }
op_ALLOC_DYNAMIC:
  {
    uint8_t tag_val = code[program_counter++];
    BlockTag tag = static_cast<BlockTag>(tag_val);
    int64_t raw_size = static_cast<int64_t>(POP());
    if (raw_size < 0) {
      throw std::runtime_error("NegativeArraySizeException: Attempted to create array with negative size " + std::to_string(raw_size));
    }
    uint32_t size = static_cast<uint32_t>(raw_size);
    PUSH(memory.dynamic_allocation(size, tag));
    DISPATCH();
  }
op_GET_PROPERTY:
  {
    uint32_t imm = read_u32(bytecode, program_counter);
    uint32_t offset = imm & 0x0FFFFFFF;
    uint32_t width = imm >> 28;
    if (width == 0) width = 8;
    Address obj = static_cast<Address>(POP());
    if (obj == 0) throw std::runtime_error("NullPointer");
    uint32_t byte_width = (width == 5) ? 1 : (width == 6) ? 2 : (width == 7) ? 4 : width;
    if (obj + offset + byte_width > memory.heap.size())
      throw std::runtime_error("Heap out of bounds on GET_PROPERTY");
    uint64_t val = 0;
    switch (width) {
      case 1: val = memory.read_u8(obj, offset); break;
      case 2: val = memory.read_u16(obj, offset); break;
      case 4: val = memory.read_u32(obj, offset); break;
      case 5: val = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int8_t>(memory.read_u8(obj, offset)))); break;
      case 6: val = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int16_t>(memory.read_u16(obj, offset)))); break;
      case 7: val = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(memory.read_u32(obj, offset)))); break;
      case 8: default: val = memory.read_u64(obj, offset); break;
    }
    PUSH(val);
    DISPATCH();
  }
op_SET_PROPERTY:
  {
    uint32_t imm = read_u32(bytecode, program_counter);
    uint32_t offset = imm & 0x0FFFFFFF;
    uint32_t width = imm >> 28;
    if (width == 0) width = 8;
    Address obj = static_cast<Address>(POP());
    uint64_t val = POP();
    if (obj == 0) throw std::runtime_error("NullPointer");
    uint32_t byte_width = (width == 5) ? 1 : (width == 6) ? 2 : (width == 7) ? 4 : width;
    if (obj + offset + byte_width > memory.heap.size())
      throw std::runtime_error("Heap out of bounds on SET_PROPERTY");

    switch (width) {
      case 1:
      case 5:
        memory.write_u8(obj, offset, static_cast<uint8_t>(val)); break;
      case 2:
      case 6:
        memory.write_u16(obj, offset, static_cast<uint16_t>(val)); break;
      case 4:
      case 7:
        memory.write_u32(obj, offset, static_cast<uint32_t>(val)); break;
      case 8: default:
        memory.write_u64(obj, offset, val); break;
    }
    DISPATCH();
  }
op_WEAK_SET_PROPERTY:
  {
    uint32_t imm = read_u32(bytecode, program_counter);
    uint32_t offset = imm & 0x0FFFFFFF;
    Address obj = static_cast<Address>(POP());
    uint64_t val = POP();
    if (obj == 0) throw std::runtime_error("NullPointer");
    if (obj + offset + 8 > memory.heap.size())
      throw std::runtime_error("Heap out of bounds on WEAK_SET_PROPERTY");

    Address slot_address = obj + offset;
    Address old_val = static_cast<Address>(memory.read_u64(obj, offset));

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
      memory.weak_references[static_cast<Address>(val)].insert(slot_address);
    }
    DISPATCH();
  }
op_GET_ARRAY:
  {
    uint32_t index = static_cast<uint32_t>(POP());
    Address array_addr = static_cast<Address>(POP());
    if (array_addr == 0) throw std::runtime_error("NullPointer");
    uint32_t length = memory.get_array_length(array_addr);
    if (index >= length) throw std::runtime_error("Out of Bounds on GET_ARRAY: index " + std::to_string(index) + " >= length " + std::to_string(length));
    BlockTag tag = memory.get_block_tag(array_addr);
    uint64_t val = 0;
    switch (tag) {
      case BlockTag::ARRAY_U8:
        val = memory.read_u8(array_addr, index);
        break;
      case BlockTag::ARRAY_I8:
        val = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int8_t>(memory.read_u8(array_addr, index))));
        break;
      case BlockTag::ARRAY_U16:
        val = memory.read_u16(array_addr, index * 2);
        break;
      case BlockTag::ARRAY_I16:
        val = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int16_t>(memory.read_u16(array_addr, index * 2))));
        break;
      case BlockTag::ARRAY_U32:
        val = memory.read_u32(array_addr, index * 4);
        break;
      case BlockTag::ARRAY_I32:
        val = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(memory.read_u32(array_addr, index * 4))));
        break;
      case BlockTag::ARRAY_U64:
      case BlockTag::ARRAY_REF:
      case BlockTag::CLOSURE:
      default:
        val = memory.read_u64(array_addr, index * 8);
        break;
    }
    PUSH(val);
    DISPATCH();
  }
op_SET_ARRAY:
  {
    uint64_t val = POP();
    uint32_t index = static_cast<uint32_t>(POP());
    Address array_addr = static_cast<Address>(POP());
    if (array_addr == 0) throw std::runtime_error("NullPointer");
    uint32_t length = memory.get_array_length(array_addr);
    if (index >= length) {
      throw std::runtime_error("Out of Bounds on SET_ARRAY at PC=" + std::to_string(program_counter) + ": array_addr=" + std::to_string(array_addr) + " index=" + std::to_string(index) + " >= length=" + std::to_string(length) + ", val=" + std::to_string(val));
    }
    BlockTag tag = memory.get_block_tag(array_addr);
    switch (tag) {
      case BlockTag::ARRAY_U8:
      case BlockTag::ARRAY_I8:
        memory.write_u8(array_addr, index, static_cast<uint8_t>(val));
        break;
      case BlockTag::ARRAY_U16:
      case BlockTag::ARRAY_I16:
        memory.write_u16(array_addr, index * 2, static_cast<uint16_t>(val));
        break;
      case BlockTag::ARRAY_U32:
      case BlockTag::ARRAY_I32:
        memory.write_u32(array_addr, index * 4, static_cast<uint32_t>(val));
        break;
      case BlockTag::ARRAY_REF:
        {
          Address old_child = static_cast<Address>(memory.read_u64(array_addr, index * 8));
          memory.write_u64(array_addr, index * 8, val);
          if (old_child != 0) {
            SYNC_SP();
            memory.decrease_reference(old_child);
            RESTORE_SP();
          }
        }
        break;
      case BlockTag::ARRAY_U64:
      case BlockTag::CLOSURE:
      default:
        memory.write_u64(array_addr, index * 8, val);
        break;
    }
    PUSH(val);
    DISPATCH();
  }
op_ARRAY_LENGTH:
  {
    Address array_addr = static_cast<Address>(POP());
    if (array_addr == 0) throw std::runtime_error("NullPointer");
    uint32_t length = memory.get_array_length(array_addr);
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
    SYNC_SP();
    memory.decrease_reference(addr);
    RESTORE_SP();
    DISPATCH();
  }

op_CONV_I8:
  {
    uint64_t v = POP();
    int8_t conv = static_cast<int8_t>(v);
    PUSH(static_cast<uint64_t>(static_cast<int64_t>(conv)));
    DISPATCH();
  }
op_CONV_I16:
  {
    uint64_t v = POP();
    int16_t conv = static_cast<int16_t>(v);
    PUSH(static_cast<uint64_t>(static_cast<int64_t>(conv)));
    DISPATCH();
  }
op_CONV_I32:
  {
    uint64_t v = POP();
    int32_t conv = static_cast<int32_t>(v);
    PUSH(static_cast<uint64_t>(static_cast<int64_t>(conv)));
    DISPATCH();
  }
op_CONV_I64:
  {
    DISPATCH();
  }
op_CONV_U8:
  {
    uint64_t v = POP();
    uint8_t conv = static_cast<uint8_t>(v);
    PUSH(static_cast<uint64_t>(conv));
    DISPATCH();
  }
op_CONV_U16:
  {
    uint64_t v = POP();
    uint16_t conv = static_cast<uint16_t>(v);
    PUSH(static_cast<uint64_t>(conv));
    DISPATCH();
  }
op_CONV_U32:
  {
    uint64_t v = POP();
    uint32_t conv = static_cast<uint32_t>(v);
    PUSH(static_cast<uint64_t>(conv));
    DISPATCH();
  }
op_CONV_U64:
  {
    DISPATCH();
  }
op_CONV_F32:
  {
    double d = bit_cast_from_u64<double>(POP());
    float f = static_cast<float>(d);
    PUSH(bit_cast_to_u64(static_cast<double>(f)));
    DISPATCH();
  }
op_CONV_F64:
  {
    DISPATCH();
  }
op_CONV_I_TO_F:
  {
    int64_t i = static_cast<int64_t>(POP());
    double d = static_cast<double>(i);
    PUSH(bit_cast_to_u64(d));
    DISPATCH();
  }
op_CONV_F_TO_I:
  {
    double d = bit_cast_from_u64<double>(POP());
    int64_t i = static_cast<int64_t>(d);
    PUSH(static_cast<uint64_t>(i));
    DISPATCH();
  }
op_NEGATE_I64:
  {
    uint64_t a = POP();
    PUSH((~a) + 1ULL);
    DISPATCH();
  }

op_CALL:
  {
    uint32_t arg_count = static_cast<uint32_t>(POP());
    uint64_t callable = POP();
    uint32_t target_ip = static_cast<uint32_t>(callable & 0xFFFFFFFF);
    uint32_t env_address = static_cast<uint32_t>(callable >> 32);

    if (target_ip == 0) {
      throw std::runtime_error("NullPointerException: Attempted to invoke null function pointer");
    }

    uint32_t current_sp_idx = static_cast<uint32_t>(sp - stack);
    uint32_t new_frame_pointer = current_sp_idx - arg_count;
    if (call_depth == 1) {
      entry_method_called = true;
    }
    if (call_depth >= 65536)
      throw std::runtime_error("Stack overflow: max call depth exceeded");
    call_stack[call_depth++] = Frame(program_counter, new_frame_pointer, arg_count, active_closure_env);
    active_closure_env = env_address;

    program_counter = target_ip;
    DISPATCH();
  }
op_CALL_NATIVE:
  {
    uint32_t id = read_u32(bytecode, program_counter);
    uint32_t arg_count = read_u32(bytecode, program_counter);
    if (program_counter >= bytecode.size()) throw std::runtime_error("Malformed bytecode: unexpected EOF");
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
        if (id < native_table.size() && native_table[id] != nullptr) {
            uint64_t result = native_table[id](*this, self_address, args.data(), arg_count);
            RESTORE_SP();
            PUSH(result);
        } else if (native_registry.count(id)) {
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

      uint32_t ref_count = read_u32(bytecode, program_counter);
      std::vector<uint32_t> ref_offsets(ref_count);
      for (uint32_t i = 0; i < ref_count; ++i) {
        ref_offsets[i] = read_u32(bytecode, program_counter);
      }
      vtable_ref_fields[vtable_id] = std::move(ref_offsets);
    }
    DISPATCH();
  }
op_INSTANCEOF:
  {
    uint32_t target_vtable_id = read_u32(bytecode, program_counter);
    Address obj = static_cast<Address>(*(sp - 1));
    --sp;

    bool is_instance = false;
    if (obj != 0 && memory.get_block_tag(obj) == BlockTag::OBJECT) {
      int32_t current_vtable = static_cast<int32_t>(memory.read_u32(obj, 0));
      while (current_vtable != -1) {
        if (static_cast<uint32_t>(current_vtable) == target_vtable_id ||
            (itables.count(current_vtable) && itables[current_vtable].count(target_vtable_id))) {
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
      if (memory.get_block_tag(obj) != BlockTag::OBJECT) {
        throw std::runtime_error("TypeCastException: Cannot cast non-object to class instance");
      }
      int32_t current_vtable = static_cast<int32_t>(memory.read_u32(obj, 0));
      bool is_instance = false;
      while (current_vtable != -1) {
        if (static_cast<uint32_t>(current_vtable) == target_vtable_id ||
            (itables.count(current_vtable) && itables[current_vtable].count(target_vtable_id))) {
          is_instance = true;
          break;
        }
        current_vtable = vtable_bases.count(current_vtable)
                             ? vtable_bases[current_vtable]
                             : -1;
      }
      if (!is_instance) {
        throw std::runtime_error("TypeCastException: Invalid cast exception at runtime");
      }
    }
    DISPATCH();
  }
op_SET_VTABLE:
  {
    uint32_t vtable_id = read_u32(bytecode, program_counter);
    Address obj = static_cast<Address>(*(sp - 1));
    memory.write_u32(obj, 0, vtable_id);
    DISPATCH();
  }
op_CALL_VIRTUAL:
  {
    uint32_t vtable_index = read_u32(bytecode, program_counter);
    uint32_t arg_count = read_u32(bytecode, program_counter);

    uint32_t current_sp_idx = static_cast<uint32_t>(sp - stack);
    Address obj = static_cast<Address>(stack[current_sp_idx - arg_count]);
    if (obj == 0) throw std::runtime_error("NullPointer");
    uint32_t vtable_id = memory.read_u32(obj, 0);
    if (vtables.find(vtable_id) == vtables.end() ||
        vtable_index >= vtables[vtable_id].size()) {
      throw std::runtime_error("Virtual method resolution failed!");
    }
    uint32_t target_ip = vtables[vtable_id][vtable_index];

    uint32_t new_frame_pointer = current_sp_idx - arg_count;
    if (call_depth >= 65536)
      throw std::runtime_error("Stack overflow: max call depth exceeded");
    call_stack[call_depth++] = Frame(program_counter, new_frame_pointer, arg_count, active_closure_env);
    active_closure_env = 0;
    program_counter = target_ip;

    DISPATCH();
  }
op_CALL_INTERFACE:
  {
    uint32_t iface_id = read_u32(bytecode, program_counter);
    uint32_t iface_method_index = read_u32(bytecode, program_counter);
    uint32_t arg_count = read_u32(bytecode, program_counter);

    uint32_t current_sp_idx = static_cast<uint32_t>(sp - stack);
    Address obj = static_cast<Address>(stack[current_sp_idx - arg_count]);
    if (obj == 0) throw std::runtime_error("NullPointer");
    uint32_t class_vtable_id = memory.read_u32(obj, 0);

    uint32_t target_ip = 0;
    {
      auto it_cls = itables.find(class_vtable_id);
      if (it_cls == itables.end()) {
        throw std::runtime_error("Virtual method resolution failed!");
      }
      auto it_iface = it_cls->second.find(iface_id);
      if (it_iface == it_cls->second.end() ||
          iface_method_index >= it_iface->second.size()) {
        throw std::runtime_error("Virtual method resolution failed!");
      }
      uint32_t vtable_index = it_iface->second[iface_method_index];
      auto it_vtable = vtables.find(class_vtable_id);
      if (it_vtable == vtables.end() ||
          vtable_index >= it_vtable->second.size()) {
        throw std::runtime_error("Virtual method resolution failed!");
      }
      target_ip = it_vtable->second[vtable_index];
    }

    uint32_t new_frame_pointer = current_sp_idx - arg_count;
    if (call_depth >= 65536)
      throw std::runtime_error("Stack overflow: max call depth exceeded");
    call_stack[call_depth++] = Frame(program_counter, new_frame_pointer, arg_count, active_closure_env);
    active_closure_env = 0;
    program_counter = target_ip;

    DISPATCH();
  }
op_DEFINE_ITABLE:
  {
    uint32_t class_vtable_id = read_u32(bytecode, program_counter);
    uint32_t iface_count = read_u32(bytecode, program_counter);
    for (uint32_t i = 0; i < iface_count; ++i) {
      uint32_t iface_id = read_u32(bytecode, program_counter);
      uint32_t method_count = read_u32(bytecode, program_counter);
      std::vector<uint32_t> slots(method_count);
      for (uint32_t j = 0; j < method_count; ++j) {
        slots[j] = read_u32(bytecode, program_counter);
      }
      itables[class_vtable_id][iface_id] = std::move(slots);
    }
    DISPATCH();
  }
op_DEFINE_NATIVE:
  {
    {
      uint32_t id = read_u32(bytecode, program_counter);
      std::string name = read_string(bytecode, program_counter);
      NativeFunctionPtr func = nullptr;

      std::vector<std::string> candidates;
      candidates.push_back(name);

      size_t paren_pos = name.find('(');
      std::string no_params = (paren_pos != std::string::npos) ? name.substr(0, paren_pos) : name;
      candidates.push_back(no_params);

      std::string with_underscores = no_params;
      for (char &c : with_underscores) {
        if (c == '.' || c == ':') c = '_';
      }
      candidates.push_back(with_underscores);

      size_t last_sep = no_params.find_last_of(".:");
      if (last_sep != std::string::npos && last_sep + 1 < no_params.size()) {
        candidates.push_back(no_params.substr(last_sep + 1));
      }

      for (const auto &cand : candidates) {
        if (options.native_functions.count(cand)) {
          func = options.native_functions[cand];
          break;
        }
        func = NativeRegistry::global().find_function(cand);
        if (func) break;
        func = NativeRegistry::global().find_symbol_in_loaded_libraries(cand);
        if (func) break;
      }

      if (func) {
        register_native(id, func);
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

    if (call_depth == stop_call_depth) {
      SYNC_SP();
      if (stop_call_depth == 0) {
        exit_code = static_cast<int32_t>(ret_val);
      }
      return;
    }

    if (call_depth > 0) {
      PUSH(ret_val);
      program_counter = frame.return_ip;
      active_closure_env = frame.closure_env;
    } else {
      SYNC_SP();
      exit_code = static_cast<int32_t>(ret_val);
      return;
    }
    DISPATCH();
  }
op_HALT:
  SYNC_SP();
  if (instruction_budget > 0 && instructions_executed >= instruction_budget) {
    return;
  }
  if (entry_method_called && sp > stack) {
    exit_code = static_cast<int32_t>(POP());
    SYNC_SP();
  }
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
      auto throw_unhandled = [&]() {
          std::string err = "Unhandled exception reached top level";
          if (active_exception != 0 && memory.get_block_tag(active_exception) == BlockTag::OBJECT) {
              uint32_t vtid = memory.read_u32(active_exception, 0);
              err += " (vtable_id=" + std::to_string(vtid) + ")";
          }
          err += ".";
          if (active_exception != 0) {
              SYNC_SP();
              memory.decrease_reference(active_exception);
              RESTORE_SP();
              active_exception = 0;
          }
          throw std::runtime_error(err);
      };

      if (call_depth == 0) {
          throw_unhandled();
      }
      Frame current_frame = call_stack[call_depth - 1];
      Address ret_ip = current_frame.return_ip;
      
      call_depth--;
      sp = stack + current_frame.frame_pointer; // Pop locals
      active_closure_env = current_frame.closure_env;
      
      if (call_depth == 0) {
          throw_unhandled();
      }
      
      bool found_cleanup = false;
      {
          auto it = return_to_cleanup.find(ret_ip);
          if (it != return_to_cleanup.end()) {
              program_counter = it->second;
              found_cleanup = true;
          }
      }

      if (!found_cleanup) {
          while (true) {
              if (call_depth == 0) {
                  throw_unhandled();
              }
              Frame caller = call_stack[call_depth - 1];
              bool inner_found = false;
              {
                  auto it2 = return_to_cleanup.find(caller.return_ip);
                  if (it2 != return_to_cleanup.end()) {
                      program_counter = it2->second;
                      inner_found = true;
                  }
              }
              if (inner_found) {
                  break;
              }
              call_depth--;
              sp = stack + caller.frame_pointer;
              active_closure_env = caller.closure_env;
          }
      }
      DISPATCH();
  }
op_THROW_EXCEPTION:
  {
      uint64_t exc = POP();
      if (exc == 0) throw std::runtime_error("NullPointer");
      memory.increase_reference(exc); // Prevent it from being garbage collected during unwinding
      active_exception = exc;
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
          SYNC_SP();
          memory.decrease_reference(active_exception);
          RESTORE_SP();
          active_exception = 0;
      }
      DISPATCH();
  }
op_SIZEOF:
  {
      Address addr = static_cast<Address>(POP());
      if (addr == 0) {
          PUSH(0);
      } else {
          BlockTag tag = memory.get_block_tag(addr);
          uint32_t count = memory.get_array_length(addr);
          uint32_t byte_size = count;
          if (tag == BlockTag::ARRAY_U16 || tag == BlockTag::ARRAY_I16) byte_size = count * 2;
          else if (tag == BlockTag::ARRAY_U32 || tag == BlockTag::ARRAY_I32) byte_size = count * 4;
          else if (tag == BlockTag::ARRAY_U64 || tag == BlockTag::ARRAY_REF || tag == BlockTag::CLOSURE) byte_size = count * 8;
          PUSH(static_cast<uint64_t>(byte_size));
      }
      DISPATCH();
  }
op_ALLOC_FRAME:
  {
      uint32_t frame_size = read_u32(bytecode, program_counter);
      uint32_t arg_count = call_stack[call_depth - 1].arg_count;
      if (frame_size > arg_count) {
          uint32_t diff = frame_size - arg_count;
          if (sp + diff > stack + memory.stack.size()) {
              SYNC_SP();
              throw std::runtime_error("StackOverflowException: execution stack exceeded capacity on ALLOC_FRAME");
          }
          std::fill(sp, sp + diff, 0);
          sp += diff;
      }
      DISPATCH();
  }
op_INC_REF_CALLABLE:
  {
      uint64_t val = POP();
      Address env = static_cast<Address>(val >> 32);
      if (env != 0) memory.increase_reference(env);
      PUSH(val);
      DISPATCH();
  }
op_DEC_REF_CALLABLE:
  {
      uint64_t val = POP();
      Address env = static_cast<Address>(val >> 32);
      if (env != 0) {
        SYNC_SP();
        memory.decrease_reference_callable(env);
        RESTORE_SP();
      }
      DISPATCH();
  }
op_UNPACK_CAPTURES:
  {
      uint32_t dest_slot = read_u32(bytecode, program_counter);
      uint32_t count = read_u32(bytecode, program_counter);
      Address env = active_closure_env;
      if (env != 0) {
          uint32_t fp = call_stack[call_depth - 1].frame_pointer;
          for (uint32_t i = 0; i < count; ++i) {
              if (fp + dest_slot + i >= memory.stack.size()) {
                  throw std::runtime_error("Frame out of bounds on UNPACK_CAPTURES");
              }
              stack[fp + dest_slot + i] = memory.read_u64(env, (2 + i) * 8);
          }
      }
      DISPATCH();
  }
op_PACK_CLOSURE:
  {
      uint32_t target_ip = static_cast<uint32_t>(POP());
      Address env = static_cast<Address>(POP());
      uint64_t callable = (static_cast<uint64_t>(env) << 32) | (static_cast<uint64_t>(target_ip) & 0xFFFFFFFFULL);
      PUSH(callable);
      DISPATCH();
  }
op_DEC_REF_SLOT:
  {
      uint32_t index = read_u32(bytecode, program_counter);
      uint64_t ref_mask = read_u64(bytecode, program_counter);
      bool is_ref = (index < 64) ? (((ref_mask >> index) & 1ULL) != 0) : ((ref_mask & 1ULL) != 0);
      if (is_ref && call_depth > 0) {
        uint32_t fp = call_stack[call_depth - 1].frame_pointer;
        if (fp + index < memory.stack.size()) {
          uint64_t val = stack[fp + index];
          if (val != 0) {
            SYNC_SP();
            memory.decrease_reference(static_cast<Address>(val));
            RESTORE_SP();
          }
        }
      }
      DISPATCH();
  }
  op_ASSERT:
  {
      uint8_t has_msg = code[program_counter++];
      uint32_t line_num = read_u32(bytecode, program_counter);
      uint32_t col_num = read_u32(bytecode, program_counter);
      uint64_t msg_val = has_msg ? POP() : 0;
      uint64_t cond = POP();
      if (cond == 0) {
          std::string msg;
          if (has_msg && msg_val > 0 && msg_val < memory.heap.size()) {
              uint64_t char_addr = msg_val;
              uint32_t len = memory.get_array_length(static_cast<Address>(char_addr));
              if (len == 0 || len > 100000) {
                  if (msg_val + 16 <= memory.heap.size()) {
                      uint64_t field = memory.read_u64(static_cast<Address>(msg_val), 8);
                      if (field > 0 && field < memory.heap.size()) {
                          uint32_t f_len = memory.get_array_length(static_cast<Address>(field));
                          if (f_len > 0 && f_len <= 100000) {
                              char_addr = field;
                              len = f_len;
                          }
                      }
                  }
                  if (len == 0 || len > 100000) {
                      uint64_t field0 = memory.read_u64(static_cast<Address>(msg_val), 0);
                      if (field0 > 0 && field0 < memory.heap.size()) {
                          uint32_t f_len = memory.get_array_length(static_cast<Address>(field0));
                          if (f_len > 0 && f_len <= 100000) {
                              char_addr = field0;
                              len = f_len;
                          }
                      }
                  }
              }
              for (uint32_t i = 0; i < len; ++i) {
                  msg.push_back(memory.read_char(static_cast<Address>(char_addr), i));
              }
          }
          std::string err = "AssertionError";
          if (!msg.empty()) {
              err += ": " + msg;
          }
          err += " (at line " + std::to_string(line_num) + ", column " + std::to_string(col_num) + ")";
          throw std::runtime_error(err);
      }
      DISPATCH();
  }
  op_EXIT:
  {
      SYNC_SP();
      uint64_t code_val = 0;
      if (sp > stack) {
          code_val = POP();
          SYNC_SP();
      }
      exit_code = static_cast<int32_t>(code_val);
      return;
  }
  op_DEFINE_DESTRUCTOR:
  {
      uint32_t vtid = read_u32(bytecode, program_counter);
      uint32_t dtor_ip = read_u32(bytecode, program_counter);
      vtable_destructors[vtid] = dtor_ip;
      DISPATCH();
  }
  op_NEW_TUPLE:
  {
      uint32_t count = read_u32(bytecode, program_counter);
      uint64_t *stack_vals = sp - count;
      size_t values_offset = (count * 4 + 7) & ~7;
      Address tuple_addr = static_cast<Address>(memory.dynamic_allocation(count, BlockTag::TUPLE));
      for (uint32_t i = 0; i < count; ++i) {
          uint32_t elem_tag = read_u32(bytecode, program_counter);
          memory.write_u32(tuple_addr, i * 4, elem_tag);
      }
      for (uint32_t i = 0; i < count; ++i) {
          memory.write_u64(tuple_addr, static_cast<uint32_t>(values_offset + i * 8), stack_vals[i]);
      }
      sp -= count;
      PUSH(tuple_addr);
      DISPATCH();
  }
  op_TUPLE_GET:
  {
      uint32_t index = static_cast<uint32_t>(POP());
      Address tuple_addr = static_cast<Address>(POP());
      if (tuple_addr == 0) throw std::runtime_error("NullPointer");
      uint32_t count = memory.get_array_length(tuple_addr);
      if (index >= count) {
          throw std::runtime_error("IndexOutOfBoundsException: tuple index " + std::to_string(index) + " >= length " + std::to_string(count));
      }
      uint32_t type_tag = memory.read_u32(tuple_addr, index * 4);
      Address values_offset = (count * 4 + 7) & ~7;
      uint64_t val = memory.read_u64(tuple_addr, values_offset + index * 8);
      if (type_tag & 0x80000000) {
          Address child = static_cast<Address>(val);
          if (child != 0) {
              memory.increase_reference(child);
          }
      }
      PUSH(val);
      DISPATCH();
  }
  op_TUPLE_LEN:
  {
      Address tuple_addr = static_cast<Address>(POP());
      if (tuple_addr == 0) throw std::runtime_error("NullPointer");
      uint32_t count = memory.get_array_length(tuple_addr);
      PUSH(static_cast<uint64_t>(count));
      DISPATCH();
  }
  op_TUPLE_TYPE:
  {
      uint32_t index = static_cast<uint32_t>(POP());
      Address tuple_addr = static_cast<Address>(POP());
      if (tuple_addr == 0) throw std::runtime_error("NullPointer");
      uint32_t count = memory.get_array_length(tuple_addr);
      if (index >= count) {
          throw std::runtime_error("IndexOutOfBoundsException: tuple index " + std::to_string(index) + " >= length " + std::to_string(count));
      }
      uint32_t type_tag = memory.read_u32(tuple_addr, index * 4);
      PUSH(static_cast<uint64_t>(type_tag));
      DISPATCH();
  }
  op_TUPLE_CAST:
  {
      uint32_t target_tag = read_u32(bytecode, program_counter);
      uint32_t index = static_cast<uint32_t>(POP());
      Address tuple_addr = static_cast<Address>(POP());
      if (tuple_addr == 0) throw std::runtime_error("NullPointer");
      uint32_t count = memory.get_array_length(tuple_addr);
      if (index >= count) {
          throw std::runtime_error("IndexOutOfBoundsException: tuple index " + std::to_string(index) + " >= length " + std::to_string(count));
      }
      uint32_t elem_tag = memory.read_u32(tuple_addr, index * 4);
      Address values_offset = (count * 4 + 7) & ~7;
      uint64_t val = memory.read_u64(tuple_addr, values_offset + index * 8);

      if (target_tag & 0x80000000) {
          if (!(elem_tag & 0x80000000)) {
              throw std::runtime_error("TypeCastException: Cannot cast primitive tuple element to reference type");
          }
          Address child = static_cast<Address>(val);
          if (child != 0) {
              if (target_tag == 0x8000000E) { // string
                  if (memory.get_block_tag(child) != BlockTag::ARRAY_U8) {
                      throw std::runtime_error("TypeCastException: Cannot cast to string");
                  }
              } else if (target_tag == 0x8000000F) { // tuple
                  if (memory.get_block_tag(child) != BlockTag::TUPLE) {
                      throw std::runtime_error("TypeCastException: Cannot cast to tuple");
                  }
              } else if (target_tag == 0x80000010) { // array
                  BlockTag btag = memory.get_block_tag(child);
                  if (btag < BlockTag::ARRAY_U8 || btag > BlockTag::ARRAY_REF) {
                      throw std::runtime_error("TypeCastException: Cannot cast to array");
                  }
              } else if (target_tag & 0x40000000) { // class
                  uint32_t target_vtid = target_tag & 0x3FFFFFFF;
                  if (target_vtid != 0x3FFFFFFF) {
                      if (memory.get_block_tag(child) != BlockTag::OBJECT) {
                          throw std::runtime_error("TypeCastException: Incompatible reference type");
                      }
                      int32_t cur_vtid = static_cast<int32_t>(memory.read_u32(child, 0));
                      bool matched = false;
                      while (cur_vtid != -1) {
                          if (static_cast<uint32_t>(cur_vtid) == target_vtid ||
                              (itables.count(cur_vtid) && itables[cur_vtid].count(target_vtid))) {
                              matched = true;
                              break;
                          }
                          cur_vtid = vtable_bases.count(cur_vtid) ? vtable_bases[cur_vtid] : -1;
                      }
                      if (!matched) {
                          throw std::runtime_error("TypeCastException: Incompatible class cast");
                      }
                  }
              }
              memory.increase_reference(child);
          }
          PUSH(child);
      } else {
          if (elem_tag & 0x80000000) {
              throw std::runtime_error("TypeCastException: Cannot cast reference tuple element to primitive type");
          }
          if (elem_tag != target_tag) {
              throw std::runtime_error("TypeCastException: Incompatible primitive type cast");
          }
          PUSH(val);
      }
      DISPATCH();
  }
  op_TUPLE_INSTANCEOF:
  {
      uint32_t target_tag = read_u32(bytecode, program_counter);
      uint32_t index = static_cast<uint32_t>(POP());
      Address tuple_addr = static_cast<Address>(POP());
      if (tuple_addr == 0) {
          PUSH(0);
          DISPATCH();
      }
      uint32_t count = memory.get_array_length(tuple_addr);
      if (index >= count) {
          PUSH(0);
          DISPATCH();
      }
      uint32_t elem_tag = memory.read_u32(tuple_addr, index * 4);
      Address values_offset = (count * 4 + 7) & ~7;
      uint64_t val = memory.read_u64(tuple_addr, values_offset + index * 8);

      if (target_tag & 0x80000000) {
          if (!(elem_tag & 0x80000000)) {
              PUSH(0);
              DISPATCH();
          }
          Address child = static_cast<Address>(val);
          if (child == 0) {
              PUSH(0);
              DISPATCH();
          }
          if (target_tag == 0x8000000E) { // string
              PUSH(memory.get_block_tag(child) == BlockTag::ARRAY_U8 ? 1 : 0);
              DISPATCH();
          } else if (target_tag == 0x8000000F) { // tuple
              PUSH(memory.get_block_tag(child) == BlockTag::TUPLE ? 1 : 0);
              DISPATCH();
          } else if (target_tag == 0x80000010) { // array
              BlockTag btag = memory.get_block_tag(child);
              PUSH((btag >= BlockTag::ARRAY_U8 && btag <= BlockTag::ARRAY_REF) ? 1 : 0);
              DISPATCH();
          } else if (target_tag & 0x40000000) { // class
              uint32_t target_vtid = target_tag & 0x3FFFFFFF;
              if (target_vtid == 0x3FFFFFFF) {
                  PUSH(1);
                  DISPATCH();
              }
              if (memory.get_block_tag(child) != BlockTag::OBJECT) {
                  PUSH(0);
                  DISPATCH();
              }
              int32_t cur_vtid = static_cast<int32_t>(memory.read_u32(child, 0));
              bool matched = false;
              while (cur_vtid != -1) {
                  if (static_cast<uint32_t>(cur_vtid) == target_vtid ||
                      (itables.count(cur_vtid) && itables[cur_vtid].count(target_vtid))) {
                      matched = true;
                      break;
                  }
                  cur_vtid = vtable_bases.count(cur_vtid) ? vtable_bases[cur_vtid] : -1;
              }
              PUSH(matched ? 1 : 0);
              DISPATCH();
          } else {
              PUSH(0);
              DISPATCH();
          }
      } else {
          PUSH((elem_tag == target_tag) ? 1 : 0);
          DISPATCH();
      }
  }
#undef PUSH
#undef POP
#undef PEEK
#undef SYNC_SP
#undef RESTORE_SP
#undef DISPATCH
}

int32_t run(RuntimeOptions &options) {
  std::unique_ptr<RuntimeContext> vm = options.custom_context_factory
      ? options.custom_context_factory(options)
      : std::make_unique<RuntimeContext>(options);
  vm->execute();
  return vm->exit_code;
}
} // namespace solix