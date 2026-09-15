#pragma once

#include <cstddef>
#include <cstdint>
#include <queue>
#include <stack>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <functional>
#include <string>

namespace solix {
namespace runtime {

void register_native_function(const std::string& name, std::function<void(class Program&)> func);

// -----------------------------------------------------------------------------
// Type Aliases
// -----------------------------------------------------------------------------
using Value = uint64_t;              // Standard 64-bit VM value 
using Address = uint32_t;            // 32-bit memory address space
using InstructionPointer = uint64_t; // Index into the bytecode array

class GarbageCollector;

// -----------------------------------------------------------------------------
// Execution Frame
// Represents a single function call, holding its own locals and operand stack.
// -----------------------------------------------------------------------------
struct Frame {
    InstructionPointer return_address;
    
    // The operand stack for this specific function invocation
    std::vector<Value> operand_stack;
    
    // Indexed array for local variables
    std::vector<Value> locals;

    explicit Frame(InstructionPointer ret_addr, std::size_t num_locals = 0)
        : return_address(ret_addr), locals(num_locals) {}
};

// -----------------------------------------------------------------------------
// Memory Pool (The Heap)
// -----------------------------------------------------------------------------
class MemoryPool {
public:
    explicit MemoryPool(std::size_t heap_capacity);
    
    // Allocates memory on the heap and returns its base address
    Address allocate_heap(std::size_t size);
    Address allocate_heap(Address address, std::size_t size);

    void deallocate(Address address);

    // Read and write byte vectors with an offset into the 64-bit storage
    std::vector<uint8_t> read_global(Address address, uint32_t offset, std::size_t size) const;
    void write(Address address, uint32_t offset, const std::vector<uint8_t>& value);

private:
    // Memory is stored in 64-bit segments
    std::vector<uint64_t> heap_memory;
    Address next_free = 1;

    // Track allocated chunks: Address -> Size
    std::unordered_map<Address, std::size_t> heap_allocations;
};

// -----------------------------------------------------------------------------
// Automatic Reference Counting (ARC) Engine
// -----------------------------------------------------------------------------
class GarbageCollector {
public:
    explicit GarbageCollector(MemoryPool& memory_pool);

    // Triggered by INC_REF / ADD_REF opcode
    void increase_reference(Address address);
    
    // Triggered by DEC_REF / REMOVE_REF opcode
    void decrease_reference(Address address);

    // Sweeps the queue and asks MemoryPool to deallocate isolated blocks
    void collect();

private:
    MemoryPool& memory_pool;
    
    // Address -> Current Reference Count
    std::unordered_map<Address, std::size_t> reference_counts;
    
    // Queue of addresses that hit 0 references and need to be freed
    std::queue<Address> deallocation_queue;
};

// -----------------------------------------------------------------------------
// Virtual Machine Execution Engine
// -----------------------------------------------------------------------------
class Program {
public:
    // Initializes the VM with the compiled bytecode and memory sizes
    Program(const std::vector<uint8_t>& bytecode, std::size_t heap_size = 1048576);
    
    // Executes exactly one opcode instruction
    void progress();
    
    // Starts the VM and loops `progress()` until HALT
    void run();

    // Helper methods for current frame execution
    void push_value(Value val);
    Value pop_value();

    MemoryPool& get_memory() { return memory_pool; }

private:
    std::vector<uint8_t> bytecode;
    InstructionPointer program_counter;

    MemoryPool memory_pool;
    GarbageCollector garbage_collector;
    
    // The Call Stack: A stack of active function frames
    std::stack<Frame> call_stack;

    Frame& current_frame();
};

} // namespace runtime
} // namespace solix