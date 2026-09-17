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
using Heap = std::vector<uint64_t>;
using Address = uint32_t; // Standardizing to uint32_t for addresses

struct RuntimeOptions {
    size_t stack_capacity = 1024 * 1024;       // 1M words
    size_t heap_capacity = 1024 * 1024 * 16;   // 16MB words
    
    // The bytecode can be provided directly in memory or loaded from a file
    std::variant<Bytecode, std::filesystem::path> bytecode_source;
};

// -----------------------------------------------------------------------------
// Frame Structure
// -----------------------------------------------------------------------------
struct Frame {
    Address return_ip;
    uint64_t frame_pointer;
    
    Frame(Address rip, uint64_t fp) : return_ip(rip), frame_pointer(fp) {}
};

// -----------------------------------------------------------------------------
// Memory Pool
// -----------------------------------------------------------------------------
struct Memory {
    Stack stack;
    Heap heap;

    uint64_t stack_pointer = 0;
    
    Address next_free_static = 1;
    Address next_free_dynamic = 1;

    Memory(size_t stack_cap, size_t heap_cap) {
        stack.resize(stack_cap);
        heap.resize(heap_cap);
    }

    uint64_t static_allocation(size_t size, Address address = 0);
    uint64_t dynamic_allocation(size_t size, Address address = 0);
    void deallocate(Address address);
    
    inline void write_u64(Address address, uint32_t offset, uint64_t value);
    inline uint64_t read_u64(Address address, uint32_t offset) const;
};

// -----------------------------------------------------------------------------
// Garbage Collector
// -----------------------------------------------------------------------------
struct GarbageCollector {
    Memory& memory;
    explicit GarbageCollector(Memory& mem) : memory(mem) {}

    inline void increase_reference(Address address);
    inline void decrease_reference(Address address);
};

struct RuntimeContext;
using NativeFunction = std::function<void(RuntimeContext&)>;

// -----------------------------------------------------------------------------
// Global Native Registration API
// -----------------------------------------------------------------------------
void register_native_function(const std::string& name, NativeFunction func);

// -----------------------------------------------------------------------------
// Execution Engine (RuntimeContext)
// -----------------------------------------------------------------------------
struct RuntimeContext {
    RuntimeOptions options;
    Memory memory;
    GarbageCollector gc;

    Bytecode bytecode;
    Address program_counter = 0;

    std::vector<Frame> call_stack;
    std::unordered_map<uint32_t, NativeFunction> native_registry;

    RuntimeContext(const RuntimeOptions& opts);

    void execute();

    inline void push(uint64_t val);
    inline uint64_t pop();
};

} // namespace solix
