#include "solix/runtime.hpp"
#include "solix/compiler.hpp"
#include <iostream>
#include <cstring>
#include <functional>

namespace solix {
namespace runtime {

// Native function registry
static std::unordered_map<std::string, std::function<void(Program&)>> global_native_functions;
static std::unordered_map<uint32_t, std::function<void(Program&)>> native_map;

void register_native_function(const std::string& name, std::function<void(Program&)> func) {
    global_native_functions[name] = func;
}

// -----------------------------------------------------------------------------
// Memory Pool (The Heap)
// -----------------------------------------------------------------------------
MemoryPool::MemoryPool(std::size_t heap_capacity) {
    heap_memory.resize(heap_capacity / 8, 0); // 64-bit segments
}

Address MemoryPool::allocate_heap(std::size_t size) {
    // Simple bump allocator for now, or just search for space.
    // We'll just append to a static counter for simplicity in this MVP.
    static Address next_free = 1; 
    Address alloc = next_free;
    // size is in bytes. We need (size + 7) / 8 words
    std::size_t words = (size + 7) / 8;
    if (alloc + words > heap_memory.size()) {
        throw std::runtime_error("Out of Memory");
    }
    next_free += words;
    heap_allocations[alloc] = size;
    return alloc;
}

Address MemoryPool::allocate_heap(Address address, std::size_t size) {
    return allocate_heap(size); // Realloc not properly supported in this simple version
}

void MemoryPool::deallocate(Address address) {
    heap_allocations.erase(address);
}

std::vector<uint8_t> MemoryPool::read_global(Address address, uint32_t offset, std::size_t size) const {
    std::vector<uint8_t> result(size);
    uint32_t start_byte = (address * 8) + offset;
    for (std::size_t i = 0; i < size; ++i) {
        uint32_t byte_idx = start_byte + i;
        uint32_t word_idx = byte_idx / 8;
        uint32_t byte_in_word = byte_idx % 8;
        result[i] = (heap_memory[word_idx] >> (byte_in_word * 8)) & 0xFF;
    }
    return result;
}

void MemoryPool::write(Address address, uint32_t offset, const std::vector<uint8_t>& value) {
    uint32_t start_byte = (address * 8) + offset;
    for (std::size_t i = 0; i < value.size(); ++i) {
        uint32_t byte_idx = start_byte + i;
        uint32_t word_idx = byte_idx / 8;
        uint32_t byte_in_word = byte_idx % 8;
        uint64_t mask = ~(0xFFULL << (byte_in_word * 8));
        heap_memory[word_idx] = (heap_memory[word_idx] & mask) | (static_cast<uint64_t>(value[i]) << (byte_in_word * 8));
    }
}

// -----------------------------------------------------------------------------
// Automatic Reference Counting (ARC) Engine
// -----------------------------------------------------------------------------
GarbageCollector::GarbageCollector(MemoryPool& memory_pool) : memory_pool(memory_pool) {}

void GarbageCollector::increase_reference(Address address) {
    if (address == 0) return;
    reference_counts[address]++;
}

void GarbageCollector::decrease_reference(Address address) {
    if (address == 0) return;
    if (reference_counts[address] > 0) {
        reference_counts[address]--;
        if (reference_counts[address] == 0) {
            deallocation_queue.push(address);
        }
    }
}

void GarbageCollector::collect() {
    while (!deallocation_queue.empty()) {
        Address addr = deallocation_queue.front();
        deallocation_queue.pop();
        if (reference_counts[addr] == 0) {
            memory_pool.deallocate(addr);
        }
    }
}

// -----------------------------------------------------------------------------
// Virtual Machine Execution Engine
// -----------------------------------------------------------------------------
Program::Program(const std::vector<uint8_t>& bytecode, std::size_t heap_size)
    : bytecode(bytecode), program_counter(0), memory_pool(heap_size), garbage_collector(memory_pool) {
    // Push an initial empty frame
    call_stack.push(Frame(0, 0));
}

Frame& Program::current_frame() {
    return call_stack.top();
}

void Program::push_value(Value val) {
    current_frame().operand_stack.push_back(val);
}

Value Program::pop_value() {
    if (current_frame().operand_stack.empty()) throw std::runtime_error("Stack underflow");
    Value val = current_frame().operand_stack.back();
    current_frame().operand_stack.pop_back();
    return val;
}

static uint32_t read_u32(const std::vector<uint8_t>& bytecode, InstructionPointer& pc) {
    uint32_t val = (bytecode[pc] << 24) | (bytecode[pc+1] << 16) | (bytecode[pc+2] << 8) | bytecode[pc+3];
    pc += 4;
    return val;
}

static uint64_t read_u64(const std::vector<uint8_t>& bytecode, InstructionPointer& pc) {
    uint64_t val = (static_cast<uint64_t>(bytecode[pc]) << 56) |
                   (static_cast<uint64_t>(bytecode[pc+1]) << 48) |
                   (static_cast<uint64_t>(bytecode[pc+2]) << 40) |
                   (static_cast<uint64_t>(bytecode[pc+3]) << 32) |
                   (static_cast<uint64_t>(bytecode[pc+4]) << 24) |
                   (static_cast<uint64_t>(bytecode[pc+5]) << 16) |
                   (static_cast<uint64_t>(bytecode[pc+6]) << 8) |
                   static_cast<uint64_t>(bytecode[pc+7]);
    pc += 8;
    return val;
}

static std::string read_string(const std::vector<uint8_t>& bytecode, InstructionPointer& pc) {
    uint32_t length = read_u32(bytecode, pc);
    std::string str(reinterpret_cast<const char*>(&bytecode[pc]), length);
    pc += length;
    return str;
}

void Program::progress() {
    if (program_counter >= bytecode.size()) throw std::runtime_error("PC out of bounds");
    uint8_t op = bytecode[program_counter++];
    
    switch (static_cast<compiler::OpCode>(op)) {
        case compiler::OpCode::PUSH_CONST_I32: {
            uint32_t val = read_u32(bytecode, program_counter);
            push_value(val);
            break;
        }
        case compiler::OpCode::PUSH_CONST_F64: {
            uint64_t val = read_u64(bytecode, program_counter);
            push_value(val);
            break;
        }
        case compiler::OpCode::PUSH_CONST_STRING: {
            std::string str = read_string(bytecode, program_counter);
            // In a real VM, we'd allocate this on the heap.
            // For now, we'll store strings as a fake heap address or something.
            // Let's allocate on heap.
            Address addr = memory_pool.allocate_heap(str.length() + 4);
            std::vector<uint8_t> data(4 + str.length());
            uint32_t len = str.length();
            data[0] = (len >> 24) & 0xFF;
            data[1] = (len >> 16) & 0xFF;
            data[2] = (len >> 8) & 0xFF;
            data[3] = len & 0xFF;
            std::memcpy(&data[4], str.c_str(), len);
            memory_pool.write(addr, 0, data);
            push_value(addr);
            break;
        }
        case compiler::OpCode::PUSH_TRUE: push_value(1); break;
        case compiler::OpCode::PUSH_FALSE: push_value(0); break;
        case compiler::OpCode::PUSH_NULL: push_value(0); break;
        case compiler::OpCode::POP: pop_value(); break;
        case compiler::OpCode::DUP: {
            Value val = pop_value();
            push_value(val);
            push_value(val);
            break;
        }
        case compiler::OpCode::ADD: {
            Value b = pop_value();
            Value a = pop_value();
            // Assuming I32 for simplicity unless F64 is used.
            push_value(a + b);
            break;
        }
        case compiler::OpCode::SUBTRACT: {
            Value b = pop_value();
            Value a = pop_value();
            push_value(a - b);
            break;
        }
        case compiler::OpCode::MULTIPLY: {
            Value b = pop_value();
            Value a = pop_value();
            push_value(a * b);
            break;
        }
        case compiler::OpCode::DIVIDE: {
            Value b = pop_value();
            Value a = pop_value();
            push_value(a / b);
            break;
        }
        case compiler::OpCode::MODULO: {
            Value b = pop_value();
            Value a = pop_value();
            push_value(a % b);
            break;
        }
        case compiler::OpCode::EQUAL: {
            Value b = pop_value();
            Value a = pop_value();
            push_value(a == b ? 1 : 0);
            break;
        }
        case compiler::OpCode::NOT_EQUAL: {
            Value b = pop_value();
            Value a = pop_value();
            push_value(a != b ? 1 : 0);
            break;
        }
        case compiler::OpCode::GREATER: {
            int32_t b = pop_value();
            int32_t a = pop_value();
            push_value(a > b ? 1 : 0);
            break;
        }
        case compiler::OpCode::LESS: {
            int32_t b = pop_value();
            int32_t a = pop_value();
            push_value(a < b ? 1 : 0);
            break;
        }
        case compiler::OpCode::LESS_EQUAL: {
            int32_t b = pop_value();
            int32_t a = pop_value();
            push_value(a <= b ? 1 : 0);
            break;
        }
        case compiler::OpCode::GREATER_EQUAL: {
            int32_t b = pop_value();
            int32_t a = pop_value();
            push_value(a >= b ? 1 : 0);
            break;
        }
        case compiler::OpCode::LOGICAL_NOT: {
            Value a = pop_value();
            push_value(a == 0 ? 1 : 0);
            break;
        }
        case compiler::OpCode::NEGATE: {
            int32_t a = pop_value();
            push_value(static_cast<uint64_t>(-a));
            break;
        }
        case compiler::OpCode::INC: {
            int32_t a = pop_value();
            push_value(a + 1);
            break;
        }
        case compiler::OpCode::DEC: {
            int32_t a = pop_value();
            push_value(a - 1);
            break;
        }
        case compiler::OpCode::GET_LOCAL: {
            uint32_t idx = read_u32(bytecode, program_counter);
            if (idx >= current_frame().locals.size()) current_frame().locals.resize(idx + 1, 0);
            push_value(current_frame().locals[idx]);
            break;
        }
        case compiler::OpCode::SET_LOCAL: {
            uint32_t idx = read_u32(bytecode, program_counter);
            Value val = pop_value();
            if (idx >= current_frame().locals.size()) current_frame().locals.resize(idx + 1, 0);
            current_frame().locals[idx] = val;
            break;
        }
        case compiler::OpCode::GET_GLOBAL: {
            uint32_t idx = read_u32(bytecode, program_counter);
            std::vector<uint8_t> data = memory_pool.read_global(0, idx * 8, 8); // Assuming globals at addr 0
            uint64_t val;
            std::memcpy(&val, data.data(), 8);
            push_value(val);
            break;
        }
        case compiler::OpCode::SET_GLOBAL: {
            uint32_t idx = read_u32(bytecode, program_counter);
            Value val = pop_value();
            std::vector<uint8_t> data(8);
            std::memcpy(data.data(), &val, 8);
            memory_pool.write(0, idx * 8, data);
            break;
        }
        case compiler::OpCode::JUMP: {
            uint32_t addr = read_u32(bytecode, program_counter);
            program_counter = addr;
            break;
        }
        case compiler::OpCode::JUMP_IF_FALSE: {
            uint32_t addr = read_u32(bytecode, program_counter);
            Value cond = pop_value();
            if (cond == 0) program_counter = addr;
            break;
        }
        case compiler::OpCode::JUMP_IF_TRUE: {
            uint32_t addr = read_u32(bytecode, program_counter);
            Value cond = pop_value();
            if (cond != 0) program_counter = addr;
            break;
        }
        case compiler::OpCode::ALLOC_STATIC: {
            uint32_t size = pop_value();
            // Static alloc at address 0
            memory_pool.allocate_heap(0, size * 8); 
            break;
        }
        case compiler::OpCode::ALLOC_DYNAMIC: {
            uint32_t size = pop_value();
            Address addr = memory_pool.allocate_heap(size);
            push_value(addr);
            break;
        }
        case compiler::OpCode::GET_PROPERTY: {
            uint32_t offset = read_u32(bytecode, program_counter);
            Address obj = pop_value();
            std::vector<uint8_t> data = memory_pool.read_global(obj, offset * 8, 8);
            uint64_t val;
            std::memcpy(&val, data.data(), 8);
            push_value(val);
            break;
        }
        case compiler::OpCode::SET_PROPERTY: {
            uint32_t offset = read_u32(bytecode, program_counter);
            Value val = pop_value();
            Address obj = pop_value();
            std::vector<uint8_t> data(8);
            std::memcpy(data.data(), &val, 8);
            memory_pool.write(obj, offset * 8, data);
            break;
        }
        case compiler::OpCode::INC_REF: {
            Address addr = pop_value();
            garbage_collector.increase_reference(addr);
            break;
        }
        case compiler::OpCode::DEC_REF: {
            Address addr = pop_value();
            garbage_collector.decrease_reference(addr);
            break;
        }
        case compiler::OpCode::DEFINE_NATIVE: {
            uint32_t id = read_u32(bytecode, program_counter);
            std::string name = read_string(bytecode, program_counter);
            if (global_native_functions.count(name)) {
                native_map[id] = global_native_functions[name];
            } else {
                std::cerr << "Warning: Native function " << name << " not found." << std::endl;
            }
            break;
        }
        case compiler::OpCode::CALL_NATIVE: {
            uint32_t id = read_u32(bytecode, program_counter);
            if (native_map.count(id)) {
                native_map[id](*this);
            } else {
                throw std::runtime_error("Call to unknown native function");
            }
            break;
        }
        case compiler::OpCode::CALL: {
            uint32_t arg_count = pop_value();
            uint32_t frame_size = pop_value();
            uint32_t target_ip = pop_value();
            
            std::vector<Value> args(arg_count);
            for (int i = arg_count - 1; i >= 0; --i) {
                args[i] = pop_value();
            }
            
            Frame new_frame(program_counter, frame_size);
            for (size_t i = 0; i < arg_count; ++i) {
                if (i < new_frame.locals.size()) {
                    new_frame.locals[i] = args[i];
                }
            }
            call_stack.push(new_frame);
            program_counter = target_ip;
            break;
        }
        case compiler::OpCode::RETURN: {
            Value ret_val = 0;
            if (!current_frame().operand_stack.empty()) {
                ret_val = current_frame().operand_stack.back();
            }
            InstructionPointer ret_addr = current_frame().return_address;
            call_stack.pop();
            
            if (!call_stack.empty()) {
                push_value(ret_val);
                program_counter = ret_addr;
            } else {
                // Main finished
                program_counter = bytecode.size(); // Halt
            }
            break;
        }
        case compiler::OpCode::HALT: {
            program_counter = bytecode.size(); // Stop execution
            break;
        }
        default:
            // Ignoring type conversions for MVP to keep it simple, or implement them if necessary
            if (op >= static_cast<uint8_t>(compiler::OpCode::CONV_I8) && op <= static_cast<uint8_t>(compiler::OpCode::CONV_F64)) {
                // Just a no-op for now
                break;
            }
            throw std::runtime_error("Unknown opcode: " + std::to_string(op));
    }
}

void Program::run() {
    while (program_counter < bytecode.size()) {
        progress();
        garbage_collector.collect();
    }
}

} // namespace runtime
} // namespace solix
