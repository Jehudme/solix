#include <array>
#include <unordered_set>
#pragma once

#include <filesystem>
#include <functional>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace solix {

using Bytecode = std::vector<uint8_t>;
using Stack = std::vector<uint64_t>;
using Heap = std::vector<uint64_t>; // Option B: Everything, including chars,
                                    // takes 1 full word
using Address = uint32_t;

struct RuntimeContext;
using NativeFunction = std::function<uint64_t(
    RuntimeContext &, uint64_t self_address, uint64_t *args, size_t arg_count)>;

struct RuntimeOptions {
  size_t stack_capacity = 1024 * 1024;     // 1M words
  size_t heap_capacity = 1024 * 1024 * 16; // 16MB words
  std::variant<Bytecode, std::filesystem::path> bytecode_source;
  std::vector<std::string> program_args;
  std::unordered_map<std::string, NativeFunction> native_functions;
};

// -----------------------------------------------------------------------------
// Frame Structure
// -----------------------------------------------------------------------------
struct Frame {
  Address return_ip = 0;
  uint32_t frame_pointer = 0; // Points to the start of locals in the unified Stack

  Frame() = default;
  Frame(Address rip, uint32_t fp) : return_ip(rip), frame_pointer(fp) {}
};

// -----------------------------------------------------------------------------
// Memory Pool & Garbage Collector (Combined)
// -----------------------------------------------------------------------------
struct Memory {
  Stack stack;
  Heap heap;

  uint32_t stack_pointer = 0;

  Address next_free_static = 1;
  Address next_free_dynamic = 1;

  // Tracks addresses of freed dynamic blocks so we can instantly reuse them
  std::vector<Address> free_blocks;

  // Weak references tracking: target_object -> set of heap slot addresses
  // holding the weak ref
  std::unordered_map<Address, std::unordered_set<Address>> weak_references;

  // Memory Usage Statistics
  size_t currently_used_words = 0;
  size_t peak_used_words = 0;

  Memory(size_t stack_cap, size_t heap_cap) {
    stack.resize(stack_cap);
    heap.resize(heap_cap);
  }

  uint64_t static_allocation(size_t size_in_words, Address address = 0);
  uint64_t dynamic_allocation(size_t size_in_words, Address address = 0);
  void deallocate(Address address);

  // ARC Reference Counting Methods (Replaces GarbageCollector class)
  void increase_reference(Address address);
  void decrease_reference(Address address);

  // Casting Helpers for 64-bit blocks
  void write_u64(Address address, uint32_t offset, uint64_t value);
  uint64_t read_u64(Address address, uint32_t offset) const;

  void write_f64(Address address, uint32_t offset, double value);
  double read_f64(Address address, uint32_t offset) const;

  void write_char(Address address, uint32_t offset, char value);
  char read_char(Address address, uint32_t offset) const;
};

int32_t run(RuntimeOptions &options);

const std::unordered_map<std::string, NativeFunction>& get_builtin_natives();

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

  std::array<Frame, 65536> call_stack;
  size_t call_depth = 0;
  
  // Exception handling state
  Address active_exception = 0;
  std::unordered_map<Address, Address> return_to_cleanup;
  
  std::unordered_map<uint32_t, NativeFunction> native_registry;
  std::unordered_map<uint32_t, std::vector<uint32_t>> vtables;
  std::unordered_map<uint32_t, int32_t> vtable_bases;

  RuntimeContext(const RuntimeOptions &opts);

  void register_native(uint32_t id, NativeFunction func);
  void execute();

  void push(uint64_t val);
  uint64_t pop();
};

} // namespace solix
