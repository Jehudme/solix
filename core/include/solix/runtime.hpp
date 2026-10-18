#include <array>
#include <unordered_set>
#pragma once

#include <cstring>
#include <filesystem>
#include <functional>
#include <map>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace solix {

using Bytecode = std::vector<uint8_t>;
using Stack = std::vector<uint64_t>;
using Heap = std::vector<uint8_t>; // Byte-addressed heap buffer (v1.1.0)
using Address = uint32_t;

enum class BlockTag : uint8_t {
  RAW = 0,
  OBJECT = 1,
  ARRAY_U8 = 2,
  ARRAY_I8 = 3,
  ARRAY_U16 = 4,
  ARRAY_I16 = 5,
  ARRAY_U32 = 6,
  ARRAY_I32 = 7,
  ARRAY_U64 = 8,
  ARRAY_REF = 9,
  CLOSURE = 10
};

struct RuntimeContext;
using NativeFunctionPtr = uint64_t (*)(
    RuntimeContext &, uint64_t self_address, uint64_t *args, size_t arg_count);
using NativeFunction = NativeFunctionPtr;

struct RuntimeOptions {
  size_t stack_capacity = 1024 * 1024;          // 1M words
  size_t heap_capacity = 1024 * 1024 * 64;      // 64MB bytes
  std::variant<Bytecode, std::filesystem::path> bytecode_source;
  std::vector<std::string> program_args;
  std::unordered_map<std::string, NativeFunctionPtr> native_functions;
  std::vector<std::filesystem::path> native_libraries;
  std::filesystem::path resource_directory;
  uint64_t instruction_budget = 0;              // 0 = unlimited discrete ISA instructions
  bool sandboxed = false;                       // true = disable ambient native shared libraries
  std::function<std::unique_ptr<RuntimeContext>(const RuntimeOptions &)> custom_context_factory = nullptr;
};

// -----------------------------------------------------------------------------
// Frame Structure
// -----------------------------------------------------------------------------
struct Frame {
  Address return_ip = 0;
  uint32_t frame_pointer = 0; // Points to the start of locals in the unified Stack
  uint32_t arg_count = 0;
  Address closure_env = 0;

  Frame() = default;
  Frame(Address rip, uint32_t fp, uint32_t args = 0, Address c_env = 0)
      : return_ip(rip), frame_pointer(fp), arg_count(args), closure_env(c_env) {}
};

// -----------------------------------------------------------------------------
// Memory Pool & Garbage Collector (Combined)
// -----------------------------------------------------------------------------
struct Memory {
  Stack stack;
  Heap heap;

  uint32_t stack_pointer = 0;

  Address next_free_static = 0;
  Address next_free_dynamic = 8;

  // Segregated / size-indexed free lists and address-indexed blocks for coalescing
  std::map<Address, uint32_t> free_by_addr;
  std::multimap<uint32_t, Address> free_by_size;

  // Tracks addresses of freed dynamic blocks so we can instantly reuse them
  std::vector<Address> free_blocks;

  inline void sync_free_blocks() {
    free_blocks.clear();
    free_blocks.reserve(free_by_addr.size());
    for (const auto &pair : free_by_addr) {
      free_blocks.push_back(pair.first);
    }
  }

  // Weak references tracking: target_object -> set of heap slot addresses
  // holding the weak ref
  std::unordered_map<Address, std::unordered_set<Address>> weak_references;

  // Memory Usage Statistics
  size_t currently_used_words = 0;
  size_t peak_used_words = 0;
  size_t live_objects_count = 0;

  std::function<void(Memory &, Address)> object_destructor = nullptr;

  Memory(size_t stack_cap, size_t heap_cap) {
    stack.resize(stack_cap);
    heap.resize(heap_cap);
  }

  uint64_t static_allocation(size_t size_in_bytes, Address address = 0);
  inline uint64_t dynamic_allocation(size_t count, BlockTag tag = BlockTag::RAW, Address address = 0) {
    (void)address;
    if (next_free_dynamic == 8) {
      next_free_dynamic = (static_cast<Address>(heap.size() / 2) + 7) & ~7;
    }

    size_t payload_bytes = count;
    if (tag == BlockTag::ARRAY_U8 || tag == BlockTag::ARRAY_I8) payload_bytes = count * 1;
    else if (tag == BlockTag::ARRAY_U16 || tag == BlockTag::ARRAY_I16) payload_bytes = count * 2;
    else if (tag == BlockTag::ARRAY_U32 || tag == BlockTag::ARRAY_I32) payload_bytes = count * 4;
    else if (tag == BlockTag::ARRAY_U64 || tag == BlockTag::ARRAY_REF || tag == BlockTag::CLOSURE) payload_bytes = count * 8;

    size_t aligned_payload = (payload_bytes + 7) & ~7;
    if (aligned_payload == 0) aligned_payload = 8;
    size_t total_block_bytes = aligned_payload + 8; // 8 bytes header

    Address header_addr = 0;

    // 1. Look for an exact match in the segregated free lists
    auto exact_it = free_by_size.find(static_cast<uint32_t>(aligned_payload));
    if (exact_it != free_by_size.end()) {
      header_addr = exact_it->second;
      free_by_size.erase(exact_it);
      free_by_addr.erase(header_addr);
    } else {
      // 2. Look for a block large enough to split (requires aligned_payload + 16 bytes for header + min 8-byte payload)
      auto split_it = free_by_size.lower_bound(static_cast<uint32_t>(aligned_payload + 16));
      if (split_it != free_by_size.end()) {
        uint32_t blk_payload = split_it->first;
        header_addr = split_it->second;
        free_by_size.erase(split_it);
        free_by_addr.erase(header_addr);

        uint32_t rem_payload = blk_payload - static_cast<uint32_t>(aligned_payload) - 8;
        Address rem_addr = header_addr + 8 + static_cast<Address>(aligned_payload);
        write_u32(rem_addr, 0, rem_payload);
        write_u32(rem_addr, 4, 0); // tag RAW, ref 0
        free_by_addr[rem_addr] = rem_payload;
        free_by_size.insert({rem_payload, rem_addr});
      }
    }

    // 3. If no suitable free block was available, allocate from uncommitted dynamic space
    if (header_addr == 0) {
      header_addr = next_free_dynamic;
      next_free_dynamic += static_cast<Address>(total_block_bytes);
      if (next_free_dynamic >= heap.size())
        throw std::runtime_error("Heap overflow during dynamic allocation!");
    }

    currently_used_words += (total_block_bytes / 8);
    if (currently_used_words > peak_used_words)
      peak_used_words = currently_used_words;
    live_objects_count++;

    std::memset(&heap[header_addr + 8], 0, aligned_payload);

    write_u32(header_addr, 0, static_cast<uint32_t>(count));
    uint32_t tag_and_ref = (static_cast<uint32_t>(tag) << 28) | 1U;
    write_u32(header_addr, 4, tag_and_ref);

    sync_free_blocks();
    return header_addr + 8;
  }
  void deallocate(Address address);

  // ARC Reference Counting Methods (Replaces GarbageCollector class)
  void increase_reference(Address address);
  void decrease_reference(Address address);
  void decrease_reference_callable(Address env);

  // Safe inline byte-addressed access primitives (std::memcpy handles unaligned memory)
  inline void write_u8(Address address, uint32_t offset, uint8_t value) {
    heap[address + offset] = value;
  }
  inline uint8_t read_u8(Address address, uint32_t offset = 0) const {
    return heap[address + offset];
  }

  inline void write_u16(Address address, uint32_t offset, uint16_t value) {
    std::memcpy(&heap[address + offset], &value, sizeof(value));
  }
  inline uint16_t read_u16(Address address, uint32_t offset = 0) const {
    uint16_t val;
    std::memcpy(&val, &heap[address + offset], sizeof(val));
    return val;
  }

  inline void write_u32(Address address, uint32_t offset, uint32_t value) {
    std::memcpy(&heap[address + offset], &value, sizeof(value));
  }
  inline uint32_t read_u32(Address address, uint32_t offset = 0) const {
    uint32_t val;
    std::memcpy(&val, &heap[address + offset], sizeof(val));
    return val;
  }

  inline void write_u64(Address address, uint32_t offset, uint64_t value) {
    std::memcpy(&heap[address + offset], &value, sizeof(value));
  }
  inline uint64_t read_u64(Address address, uint32_t offset = 0) const {
    uint64_t val;
    std::memcpy(&val, &heap[address + offset], sizeof(val));
    return val;
  }

  inline void write_f64(Address address, uint32_t offset, double value) {
    std::memcpy(&heap[address + offset], &value, sizeof(value));
  }
  inline double read_f64(Address address, uint32_t offset = 0) const {
    double val;
    std::memcpy(&val, &heap[address + offset], sizeof(val));
    return val;
  }

  inline void write_char(Address address, uint32_t offset, char value) {
    heap[address + offset] = static_cast<uint8_t>(value);
  }
  inline char read_char(Address address, uint32_t offset = 0) const {
    return static_cast<char>(heap[address + offset]);
  }

  inline uint32_t get_array_length(Address address) const {
    if (address < 8) return 0;
    return read_u32(address - 8, 0);
  }

  inline BlockTag get_block_tag(Address address) const {
    if (address < 4) return BlockTag::RAW;
    uint32_t tag_and_ref = read_u32(address - 4, 0);
    return static_cast<BlockTag>((tag_and_ref >> 28) & 0xF);
  }

  inline uint32_t get_block_ref_count(Address address) const {
    if (address < 4) return 0;
    uint32_t tag_and_ref = read_u32(address - 4, 0);
    return tag_and_ref & 0x0FFFFFFF;
  }
};

int32_t run(RuntimeOptions &options);

size_t get_live_object_count();

// -----------------------------------------------------------------------------
// Execution Engine
// -----------------------------------------------------------------------------
struct RuntimeContext {
  RuntimeOptions options;
  Memory memory;

  Bytecode bytecode;
  Address program_counter = 0;
  int32_t exit_code = 0;
  bool entry_method_called = false;
  std::filesystem::path resource_directory;

  std::vector<Frame> call_stack;
  size_t call_depth = 0;
  Address active_closure_env = 0;
  
  // Exception handling state
  Address active_exception = 0;
  std::unordered_map<Address, Address> return_to_cleanup;
  
  std::vector<NativeFunctionPtr> native_table;
  std::unordered_map<uint32_t, NativeFunctionPtr> native_registry;
  std::unordered_map<uint32_t, std::vector<uint32_t>> vtables;
  std::unordered_map<uint32_t, int32_t> vtable_bases;
  std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::vector<uint32_t>>> itables;
  std::unordered_map<uint32_t, std::vector<uint32_t>> vtable_ref_fields;
  std::unordered_map<uint32_t, uint32_t> vtable_destructors;
  std::unordered_set<Address> active_destructors;

  // Metering state
  uint64_t instruction_budget = 0;
  uint64_t instructions_executed = 0;

  RuntimeContext(const RuntimeOptions &opts);
  virtual ~RuntimeContext() = default;

  virtual void register_native(uint32_t id, NativeFunctionPtr func);
  virtual void execute();
  virtual void run_bytecode(size_t stop_call_depth);
  virtual void invoke_destructor(Address obj_addr, uint32_t dtor_ip);

  virtual void push(uint64_t val);
  virtual uint64_t pop();

  virtual void on_budget_exceeded();
  virtual void on_out_of_memory(size_t requested_bytes);
};

} // namespace solix
