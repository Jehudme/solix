#pragma once

#include <cstddef>
#include <cstdint>
#include <queue>
#include <stack>
#include <vector>
#include <unordered_map>
#include <stdexcept>

namespace solix {
namespace runtime {

class GarbageCollector;

// -----------------------------------------------------------------------------
// Memory Pool (Heap & Math Stack)
// -----------------------------------------------------------------------------
class MemoryPool {
public:
    MemoryPool(std::size_t heap_capacity, std::size_t stack_capacity);

    // Allocates a contiguous block of 'size' 64-bit words and returns the Address.
    uint32_t allocate(std::size_t size);
    
    // Frees a previously allocated block at 'address'.
    void deallocate(uint32_t address);

    // Bulk read/write for whole objects
    std::vector<uint64_t> read_heap(uint32_t address, std::size_t size) const;
    void write_heap(uint32_t address, const std::vector<uint64_t>& data);

    // Fast single-word read/write for GET_PROPERTY, SET_PROPERTY, GET_ARRAY, SET_ARRAY
    uint64_t read_heap_word(uint32_t address, uint32_t offset) const;
    void write_heap_word(uint32_t address, uint32_t offset, uint64_t value);

    // Math Stack Operations (PUSH, POP, etc.)
    void push_stack(uint64_t value);
    uint64_t pop_stack();

private:
    // Helper function to scan the heap for an available contiguous block
    uint32_t find_first_fit(std::size_t size);

    std::vector<uint64_t> heap_memory;
    std::vector<uint64_t> stack_memory; // Using vector for faster inline access
    std::size_t stack_pointer;

    // Track allocated chunks: Address -> Size
    std::unordered_map<uint32_t, std::size_t> heap_allocations;
    
    // Track free chunks: Address -> Size
    std::unordered_map<uint32_t, std::size_t> free_segments;
};

// -----------------------------------------------------------------------------
// Automatic Reference Counting (ARC) Engine
// -----------------------------------------------------------------------------
class GarbageCollector {
public:
    GarbageCollector(MemoryPool& memory_pool);

    // Triggered by INC_REF / ADD_REF opcode
    void increase_reference(uint32_t address);
    
    // Triggered by DEC_REF / REMOVE_REF opcode
    void decrease_reference(uint32_t address);

    // Sweeps the queue and asks MemoryPool to deallocate isolated blocks
    void collect();

private:
    MemoryPool& memory_pool;
    std::unordered_map<uint32_t, std::size_t> reference_counts;
    std::queue<uint32_t> deallocation_queue;
};

// -----------------------------------------------------------------------------
// Virtual Machine Execution Engine
// -----------------------------------------------------------------------------
class Program {
public:
    // Initializes the VM with the compiled bytecode and memory sizes
    Program(const std::vector<uint8_t>& bytecode, std::size_t heap_size = 1048576, std::size_t stack_size = 65536);
    
    // Executes exactly one opcode instruction
    void progress();
    
    // Starts the VM and loops `progress()` until HALT
    void run();

private:
    std::vector<uint8_t> bytecode;
    uint64_t program_counter;

    MemoryPool memory_pool;
    GarbageCollector garbage_collector;
    
    // Call Frames (Local Variables & Return Addresses)
    // We can define a Frame struct locally or globally later!
};

} // namespace runtime
} // namespace solix
