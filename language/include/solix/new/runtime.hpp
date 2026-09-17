#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <variant>
#include <filesystem>

namespace solix {

using OpCode = uint8_t;
using Bytecode = std::vector<uint8_t>;
using Stack = std::vector<uint64_t>;
using Heap = std::vector<uint64_t>; // Option B: Everything, including chars, takes 1 full word
using Address = uint32_t;

struct RuntimeOptions {
    size_t stack_capacity = 1024 * 1024;       // 1M words
    size_t heap_capacity = 1024 * 1024 * 16;   // 16MB words
    std::variant<Bytecode, std::filesystem::path> bytecode_source;
};

// -----------------------------------------------------------------------------
// Frame Structure
// -----------------------------------------------------------------------------
struct Frame {
    Address return_ip;
    uint32_t frame_pointer; // Points to the start of locals in the unified Stack
    
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
    inline void increase_reference(Address address);
    inline void decrease_reference(Address address);

    // Casting Helpers for 64-bit blocks
    inline void write_u64(Address address, uint32_t offset, uint64_t value);
    inline uint64_t read_u64(Address address, uint32_t offset) const;
    
    inline void write_f64(Address address, uint32_t offset, double value);
    inline double read_f64(Address address, uint32_t offset) const;

    inline void write_char(Address address, uint32_t offset, char value);
    inline char read_char(Address address, uint32_t offset) const;
};

struct RuntimeContext;
using NativeFunction = std::function<void(RuntimeContext&)>;

void register_native_function(const std::string& name, NativeFunction func);

// -----------------------------------------------------------------------------
// Execution Engine
// -----------------------------------------------------------------------------
struct RuntimeContext {
    RuntimeOptions options;
    Memory memory;

    Bytecode bytecode;
    Address program_counter = 0;

    std::vector<Frame> call_stack;
    std::unordered_map<uint32_t, NativeFunction> native_registry;

    RuntimeContext(const RuntimeOptions& opts);

    void register_native(uint32_t id, NativeFunction func);
    void execute();

    inline void push(uint64_t val);
    inline uint64_t pop();
};

} // namespace solix
