#include "solix/runtime.hpp"
#include "solix/utilities/optcodes.hpp"
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

  uint64_t header = (static_cast<uint64_t>(size_in_words) << 32) | 0ULL;
  heap[header_addr] = header;
  return header_addr + 1;
}

void Memory::deallocate(Address address) {
  if (address == 0)
    return;

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

  // Add an initial frame so we don't underflow
  call_stack.emplace_back(0, 0);

  size_t arg_count = options.program_args.size();
  Address args_array = memory.dynamic_allocation(arg_count + 1);
  heap_data[args_array] = arg_count;
  for (size_t i = 0; i < arg_count; ++i) {
    const std::string &str = options.program_args[i];
    size_t len = str.length();
    Address str_addr = memory.dynamic_allocation(len + 1);
    heap_data[str_addr] = len;
    for (size_t j = 0; j < len; ++j) {
      heap_data[str_addr + 1 + j] = static_cast<uint64_t>(str[j]);
    }
    heap_data[args_array + 1 + i] = str_addr;
  }
  push(args_array);

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
      &&op_ADD,
      &&op_SUBTRACT,
      &&op_MULTIPLY,
      &&op_DIVIDE,
      &&op_MODULO,
      &&op_EQUAL,
      &&op_NOT_EQUAL,
      &&op_GREATER,
      &&op_GREATER_EQUAL,
      &&op_LESS,
      &&op_LESS_EQUAL,
      &&op_LOGICAL_NOT,
      &&op_NEGATE,
      &&op_INC,
      &&op_DEC,
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
  };

#define DISPATCH()                                                             \
  if (program_counter >= bytecode.size())                                      \
    return;                                                                    \
  goto *dispatch_table[code[program_counter++]]

  DISPATCH();
op_PUSH_CONST_I8:
op_PUSH_CONST_I16:
op_PUSH_CONST_I32:
  {
    push(static_cast<uint64_t>(read_u32(bytecode, program_counter)));
    DISPATCH();
  }
op_PUSH_CONST_I64:
op_PUSH_CONST_U64:
op_PUSH_CONST_F64:
  {
    push(read_u64(bytecode, program_counter));
    DISPATCH();
  }
op_PUSH_CONST_U8:
op_PUSH_CONST_U16:
op_PUSH_CONST_U32:
op_PUSH_CONST_F32:
  {
    push(static_cast<uint64_t>(read_u32(bytecode, program_counter)));
    DISPATCH();
  }
op_PUSH_CONST_STRING:
  {
    { // inner scope to avoid goto-over-destructor
      std::string str = read_string(bytecode, program_counter);
      size_t len = str.length();
      Address addr = memory.dynamic_allocation(len + 1);
      heap_data[addr] = len;
      for (size_t i = 0; i < len; ++i)
        heap_data[addr + 1 + i] = static_cast<uint64_t>(str[i]);
      push(addr);
    }
    DISPATCH();
  }
op_PUSH_TRUE:
  push(1);
  DISPATCH();
op_PUSH_FALSE:
  push(0);
  DISPATCH();
op_PUSH_NULL:
  push(0);
  DISPATCH();
op_POP:
  pop();
  DISPATCH();
op_DUP:
  {
    uint64_t top = stack[memory.stack_pointer - 1];
    push(top);
    DISPATCH();
  }

op_ADD:
  {
    double b = bit_cast_from_u64<double>(pop());
    double a = bit_cast_from_u64<double>(pop());
    push(bit_cast_to_u64(a + b));
    DISPATCH();
  }
op_SUBTRACT:
  {
    double b = bit_cast_from_u64<double>(pop());
    double a = bit_cast_from_u64<double>(pop());
    push(bit_cast_to_u64(a - b));
    DISPATCH();
  }
op_MULTIPLY:
  {
    double b = bit_cast_from_u64<double>(pop());
    double a = bit_cast_from_u64<double>(pop());
    push(bit_cast_to_u64(a * b));
    DISPATCH();
  }
op_DIVIDE:
  {
    double b = bit_cast_from_u64<double>(pop());
    double a = bit_cast_from_u64<double>(pop());
    push(bit_cast_to_u64(a / b));
    DISPATCH();
  }
op_MODULO:
  {
    double b = bit_cast_from_u64<double>(pop());
    double a = bit_cast_from_u64<double>(pop());
    push(bit_cast_to_u64(std::fmod(a, b)));
    DISPATCH();
  }
op_EQUAL:
  {
    uint64_t b = pop();
    uint64_t a = pop();
    push(a == b ? 1 : 0);
    DISPATCH();
  }
op_NOT_EQUAL:
  {
    uint64_t b = pop();
    uint64_t a = pop();
    push(a != b ? 1 : 0);
    DISPATCH();
  }
op_GREATER:
  {
    double b = bit_cast_from_u64<double>(pop());
    double a = bit_cast_from_u64<double>(pop());
    push(a > b ? 1 : 0);
    DISPATCH();
  }
op_GREATER_EQUAL:
  {
    double b = bit_cast_from_u64<double>(pop());
    double a = bit_cast_from_u64<double>(pop());
    push(a >= b ? 1 : 0);
    DISPATCH();
  }
op_LESS:
  {
    double b = bit_cast_from_u64<double>(pop());
    double a = bit_cast_from_u64<double>(pop());
    push(a < b ? 1 : 0);
    DISPATCH();
  }
op_LESS_EQUAL:
  {
    double b = bit_cast_from_u64<double>(pop());
    double a = bit_cast_from_u64<double>(pop());
    push(a <= b ? 1 : 0);
    DISPATCH();
  }
op_LOGICAL_NOT:
  {
    uint64_t a = pop();
    push(a == 0 ? 1 : 0);
    DISPATCH();
  }
op_NEGATE:
  {
    double a = bit_cast_from_u64<double>(pop());
    push(bit_cast_to_u64(-a));
    DISPATCH();
  }
op_INC:
  {
    double a = bit_cast_from_u64<double>(pop());
    push(bit_cast_to_u64(a + 1.0));
    DISPATCH();
  }
op_DEC:
  {
    double a = bit_cast_from_u64<double>(pop());
    push(bit_cast_to_u64(a - 1.0));
    DISPATCH();
  }

op_GET_LOCAL:
  {
    uint32_t index = read_u32(bytecode, program_counter);
    uint32_t fp = call_stack.back().frame_pointer;
    if (fp + index >= memory.stack.size())
      throw std::runtime_error("Frame out of bounds on GET_LOCAL");
    push(stack[fp + index]);
    DISPATCH();
  }
op_SET_LOCAL:
  {
    uint32_t index = read_u32(bytecode, program_counter);
    uint32_t fp = call_stack.back().frame_pointer;
    if (fp + index >= memory.stack.size())
      throw std::runtime_error("Frame out of bounds on SET_LOCAL");
    stack[fp + index] = pop();
    DISPATCH();
  }
op_GET_GLOBAL:
  {
    uint32_t idx = read_u32(bytecode, program_counter);
    push(heap_data[idx]);
    DISPATCH();
  }
op_SET_GLOBAL:
  {
    uint32_t idx = read_u32(bytecode, program_counter);
    uint64_t val = pop();
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
    uint64_t cond = pop();
    if (cond == 0)
      program_counter = addr;
    DISPATCH();
  }
op_JUMP_IF_TRUE:
  {
    uint32_t addr = read_u32(bytecode, program_counter);
    uint64_t cond = pop();
    if (cond != 0)
      program_counter = addr;
    DISPATCH();
  }

op_ALLOC_STATIC:
  {
    uint32_t size = static_cast<uint32_t>(pop());
    memory.static_allocation(size, 0);
    DISPATCH();
  }
op_ALLOC_DYNAMIC:
  {
    uint32_t size = static_cast<uint32_t>(pop());
    push(memory.dynamic_allocation(size));
    DISPATCH();
  }
op_GET_PROPERTY:
  {
    uint32_t offset = read_u32(bytecode, program_counter);
    Address obj = static_cast<Address>(pop());
    if (obj + offset >= memory.heap.size())
      throw std::runtime_error("Heap out of bounds on GET_PROPERTY");
    push(memory.read_u64(obj, offset));
    DISPATCH();
  }
op_SET_PROPERTY:
  {
    uint32_t offset = read_u32(bytecode, program_counter);
    Address obj = static_cast<Address>(pop());
    uint64_t val = pop();
    if (obj + offset >= memory.heap.size())
      throw std::runtime_error("Heap out of bounds on SET_PROPERTY");

    memory.write_u64(obj, offset, val);
    DISPATCH();
  }
op_WEAK_SET_PROPERTY:
  {
    uint32_t offset = read_u32(bytecode, program_counter);
    Address obj = static_cast<Address>(pop());
    uint64_t val = pop();
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
    uint32_t index = static_cast<uint32_t>(pop());
    Address array_addr = static_cast<Address>(pop());
    push(heap_data[array_addr + 1 + index]);
    DISPATCH();
  }
op_SET_ARRAY:
  {
    uint64_t val = pop();
    uint32_t index = static_cast<uint32_t>(pop());
    Address array_addr = static_cast<Address>(pop());
    heap_data[array_addr + 1 + index] = val;
    push(val);
    DISPATCH();
  }

op_INC_REF:
  {
    Address addr = static_cast<Address>(pop());
    memory.increase_reference(addr);
    push(addr);
    DISPATCH();
  }
op_DEC_REF:
  {
    Address addr = static_cast<Address>(pop());
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
    uint32_t arg_count = static_cast<uint32_t>(pop());
    uint32_t frame_size = static_cast<uint32_t>(pop());
    uint32_t target_ip = static_cast<uint32_t>(pop());

    uint32_t new_frame_pointer = memory.stack_pointer - arg_count;
    call_stack.emplace_back(program_counter, new_frame_pointer);

    if (frame_size > arg_count) {
      memory.stack_pointer += (frame_size - arg_count);
    }

    program_counter = target_ip;
    DISPATCH();
  }
op_CALL_NATIVE:
  {
    uint32_t id = read_u32(bytecode, program_counter);
    if (native_registry.count(id)) {
      native_registry[id](*this, 0, nullptr, 0);
    } else {
      throw std::runtime_error("Call to unknown native function: " +
                               std::to_string(id));
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
    Address obj = static_cast<Address>(stack[memory.stack_pointer - 1]);
    memory.stack_pointer--;

    bool is_instance = false;
    if (obj != 0) {
      int32_t current_vtable = static_cast<int32_t>(heap_data[obj]);
      while (current_vtable != -1) {
        if (current_vtable == target_vtable_id) {
          is_instance = true;
          DISPATCH();
        }
        current_vtable = vtable_bases.count(current_vtable)
                             ? vtable_bases[current_vtable]
                             : -1;
      }
    }

    stack[memory.stack_pointer++] = is_instance ? 1 : 0;
    DISPATCH();
  }
op_CAST_CHECK:
  {
    uint32_t target_vtable_id = read_u32(bytecode, program_counter);
    Address obj = static_cast<Address>(stack[memory.stack_pointer - 1]);

    if (obj != 0) {
      int32_t current_vtable = static_cast<int32_t>(heap_data[obj]);
      bool is_instance = false;
      while (current_vtable != -1) {
        if (current_vtable == target_vtable_id) {
          is_instance = true;
          DISPATCH();
        }
        current_vtable = vtable_bases.count(current_vtable)
                             ? vtable_bases[current_vtable]
                             : -1;
      }
      if (!is_instance) {
        throw std::runtime_error("Invalid cast exception at runtime");
      }
    }
    // Leaves object on stack
    DISPATCH();
  }
op_SET_VTABLE:
  {
    uint32_t vtable_id = read_u32(bytecode, program_counter);
    Address obj = static_cast<Address>(stack[memory.stack_pointer - 1]);
    heap_data[obj] = vtable_id;
    DISPATCH();
  }
op_CALL_VIRTUAL:
  {
    uint32_t vtable_index = read_u32(bytecode, program_counter);
    uint32_t frame_size = read_u32(bytecode, program_counter);
    uint32_t arg_count = read_u32(bytecode, program_counter);

    Address obj = static_cast<Address>(stack[memory.stack_pointer - arg_count]);
    uint32_t vtable_id = heap_data[obj];
    if (vtables.find(vtable_id) == vtables.end() ||
        vtable_index >= vtables[vtable_id].size()) {
      throw std::runtime_error("Virtual method resolution failed!");
    }
    uint32_t target_ip = vtables[vtable_id][vtable_index];

    uint32_t new_frame_pointer = memory.stack_pointer - arg_count;
    call_stack.emplace_back(program_counter, new_frame_pointer);
    program_counter = target_ip;

    if (frame_size > arg_count) {
      memory.stack_pointer += (frame_size - arg_count);
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
    if (memory.stack_pointer > call_stack.back().frame_pointer) {
      ret_val = pop();
    }

    Frame frame = call_stack.back();
    call_stack.pop_back();
    memory.stack_pointer = frame.frame_pointer;

    if (call_stack.size() > 0) {
      push(ret_val);
      program_counter = frame.return_ip;
    } else {
      return;
    }
    DISPATCH();
  }
op_HALT:
  return;
op_THROW_ABSTRACT:
  throw std::runtime_error("Called abstract method");

#undef DISPATCH
}

void run(RuntimeOptions &options) {
  RuntimeContext vm(options);
  vm.execute();
}
} // namespace solix
