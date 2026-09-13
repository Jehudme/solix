#include "solix/runtime.hpp"

#include "solix/compiler.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace solix {
namespace runtime {

namespace {
constexpr uint64_t STRING_REF_TAG = 0xFF00'0000'0000'0000ULL;
constexpr uint64_t STRING_REF_MASK = 0x00FF'FFFF'FFFF'FFFFULL;
}

MemoryPool::MemoryPool(std::size_t heap_capacity, std::size_t stack_capacity)
    : heap_memory(heap_capacity, 0),
      stack_memory(stack_capacity, 0),
      stack_pointer(0) {
    if (heap_capacity > 1) {
        free_segments[1] = heap_capacity - 1;
    }
}

uint32_t MemoryPool::find_first_fit(std::size_t size) {
    if (size == 0) {
        return 0;
    }
    if (heap_memory.size() < 2 || size >= heap_memory.size()) {
        throw std::runtime_error("Heap is too small for allocation");
    }

    for (uint32_t candidate = 1; candidate + size <= heap_memory.size(); ++candidate) {
        bool overlaps = false;
        for (const auto& [base, alloc_size] : heap_allocations) {
            const uint64_t alloc_start = base;
            const uint64_t alloc_end = alloc_start + alloc_size;
            const uint64_t cand_start = candidate;
            const uint64_t cand_end = cand_start + size;
            if (!(cand_end <= alloc_start || cand_start >= alloc_end)) {
                overlaps = true;
                break;
            }
        }
        if (!overlaps) {
            return candidate;
        }
    }

    throw std::runtime_error("Out of heap memory");
}

uint32_t MemoryPool::allocate(std::size_t size) {
    const uint32_t address = find_first_fit(size);
    heap_allocations[address] = size;
    return address;
}

void MemoryPool::deallocate(uint32_t address) {
    auto it = heap_allocations.find(address);
    if (it == heap_allocations.end()) {
        return;
    }
    const std::size_t size = it->second;
    std::fill(heap_memory.begin() + address, heap_memory.begin() + address + size, 0);
    heap_allocations.erase(it);
}

std::vector<uint64_t> MemoryPool::read_heap(uint32_t address, std::size_t size) const {
    auto it = heap_allocations.find(address);
    if (it == heap_allocations.end()) {
        throw std::runtime_error("Invalid heap read at unallocated address");
    }
    if (size > it->second) {
        throw std::runtime_error("Invalid heap read size");
    }
    return std::vector<uint64_t>(heap_memory.begin() + address, heap_memory.begin() + address + size);
}

void MemoryPool::write_heap(uint32_t address, const std::vector<uint64_t>& data) {
    auto it = heap_allocations.find(address);
    if (it == heap_allocations.end()) {
        throw std::runtime_error("Invalid heap write at unallocated address");
    }
    if (data.size() > it->second) {
        throw std::runtime_error("Invalid heap write size");
    }
    std::copy(data.begin(), data.end(), heap_memory.begin() + address);
}

uint64_t MemoryPool::read_heap_word(uint32_t address, uint32_t offset) const {
    auto it = heap_allocations.find(address);
    if (it == heap_allocations.end() || offset >= it->second) {
        throw std::runtime_error("Invalid heap word read");
    }
    return heap_memory[address + offset];
}

void MemoryPool::write_heap_word(uint32_t address, uint32_t offset, uint64_t value) {
    auto it = heap_allocations.find(address);
    if (it == heap_allocations.end() || offset >= it->second) {
        throw std::runtime_error("Invalid heap word write");
    }
    heap_memory[address + offset] = value;
}

void MemoryPool::push_stack(uint64_t value) {
    if (stack_pointer >= stack_memory.size()) {
        throw std::runtime_error("Stack overflow");
    }
    stack_memory[stack_pointer++] = value;
}

uint64_t MemoryPool::pop_stack() {
    if (stack_pointer == 0) {
        throw std::runtime_error("Stack underflow");
    }
    return stack_memory[--stack_pointer];
}

GarbageCollector::GarbageCollector(MemoryPool& memory_pool)
    : memory_pool(memory_pool) {}

void GarbageCollector::increase_reference(uint32_t address) {
    if (address == 0) {
        return;
    }
    reference_counts[address]++;
}

void GarbageCollector::decrease_reference(uint32_t address) {
    if (address == 0) {
        return;
    }
    auto it = reference_counts.find(address);
    if (it == reference_counts.end()) {
        return;
    }
    if (it->second > 0) {
        --it->second;
    }
    if (it->second == 0) {
        deallocation_queue.push(address);
        reference_counts.erase(it);
    }
}

void GarbageCollector::collect() {
    while (!deallocation_queue.empty()) {
        memory_pool.deallocate(deallocation_queue.front());
        deallocation_queue.pop();
    }
}

Program::Program(const std::vector<uint8_t>& code, std::size_t heap_size, std::size_t stack_size)
    : bytecode(code),
      program_counter(0),
      halted(false),
      memory_pool(heap_size, stack_size),
      garbage_collector(memory_pool) {}

uint8_t Program::read_u8() {
    if (program_counter >= bytecode.size()) {
        throw std::runtime_error("Program counter out of bytecode bounds");
    }
    return bytecode[program_counter++];
}

uint32_t Program::read_u32() {
    if (program_counter + 4 > bytecode.size()) {
        throw std::runtime_error("Program counter out of bytecode bounds");
    }
    const uint32_t value = (static_cast<uint32_t>(bytecode[program_counter]) << 24) |
                           (static_cast<uint32_t>(bytecode[program_counter + 1]) << 16) |
                           (static_cast<uint32_t>(bytecode[program_counter + 2]) << 8) |
                           static_cast<uint32_t>(bytecode[program_counter + 3]);
    program_counter += 4;
    return value;
}

uint64_t Program::read_u64() {
    if (program_counter + 8 > bytecode.size()) {
        throw std::runtime_error("Program counter out of bytecode bounds");
    }
    const uint64_t value = (static_cast<uint64_t>(bytecode[program_counter]) << 56) |
                           (static_cast<uint64_t>(bytecode[program_counter + 1]) << 48) |
                           (static_cast<uint64_t>(bytecode[program_counter + 2]) << 40) |
                           (static_cast<uint64_t>(bytecode[program_counter + 3]) << 32) |
                           (static_cast<uint64_t>(bytecode[program_counter + 4]) << 24) |
                           (static_cast<uint64_t>(bytecode[program_counter + 5]) << 16) |
                           (static_cast<uint64_t>(bytecode[program_counter + 6]) << 8) |
                           static_cast<uint64_t>(bytecode[program_counter + 7]);
    program_counter += 8;
    return value;
}

float Program::read_f32() {
    const uint32_t raw = read_u32();
    float value = 0.0f;
    std::memcpy(&value, &raw, sizeof(float));
    return value;
}

double Program::read_f64() {
    const uint64_t raw = read_u64();
    double value = 0.0;
    std::memcpy(&value, &raw, sizeof(double));
    return value;
}

std::string Program::read_string() {
    const uint32_t length = read_u32();
    if (program_counter + length > bytecode.size()) {
        throw std::runtime_error("String literal out of bytecode bounds");
    }
    std::string value(reinterpret_cast<const char*>(&bytecode[program_counter]), length);
    program_counter += length;
    return value;
}

uint64_t Program::pop() {
    return memory_pool.pop_stack();
}

uint64_t Program::peek() const {
    Program* self = const_cast<Program*>(this);
    uint64_t top = self->memory_pool.pop_stack();
    self->memory_pool.push_stack(top);
    return top;
}

void Program::push(uint64_t value) {
    memory_pool.push_stack(value);
}

Program::CallFrame& Program::current_frame() {
    if (call_frames.empty()) {
        throw std::runtime_error("No active call frame");
    }
    return call_frames.back();
}

bool Program::truthy(uint64_t value) {
    return value != 0;
}

uint64_t Program::encode_string_ref(uint64_t index) {
    return STRING_REF_TAG | (index & STRING_REF_MASK);
}

bool Program::is_string_ref(uint64_t value) {
    return (value & STRING_REF_TAG) == STRING_REF_TAG;
}

uint64_t Program::decode_string_ref(uint64_t value) {
    return value & STRING_REF_MASK;
}

void Program::define_native_handler(uint32_t native_id, const std::string& symbol) {
    native_symbols[native_id] = symbol;

    if (symbol.find("print") != std::string::npos || symbol.find("Print") != std::string::npos) {
        native_handlers[native_id] = [this](Program& program) {
            const uint64_t value = program.pop();
            std::cout << program.value_to_string(value) << std::endl;
            program.push(0);
        };
    }
}

std::string Program::value_to_string(uint64_t value) const {
    if (is_string_ref(value)) {
        const uint64_t index = decode_string_ref(value);
        if (index < string_pool.size()) {
            return string_pool[static_cast<std::size_t>(index)];
        }
    }

    std::stringstream ss;
    ss << as_i64(value);
    return ss.str();
}

int64_t Program::as_i64(uint64_t value) {
    int64_t result = 0;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

uint64_t Program::as_u64(int64_t value) {
    uint64_t result = 0;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

void Program::progress() {
    if (halted) {
        return;
    }

    const auto op = static_cast<compiler::OpCode>(read_u8());
    switch (op) {
        case compiler::OpCode::PUSH_CONST_I8: push(as_u64(static_cast<int8_t>(read_u8()))); break;
        case compiler::OpCode::PUSH_CONST_I16: push(as_u64(static_cast<int16_t>(read_u32()))); break;
        case compiler::OpCode::PUSH_CONST_I32: push(as_u64(static_cast<int32_t>(read_u32()))); break;
        case compiler::OpCode::PUSH_CONST_I64: push(read_u64()); break;
        case compiler::OpCode::PUSH_CONST_U8: push(read_u8()); break;
        case compiler::OpCode::PUSH_CONST_U16: push(read_u32() & 0xFFFFULL); break;
        case compiler::OpCode::PUSH_CONST_U32: push(read_u32()); break;
        case compiler::OpCode::PUSH_CONST_U64: push(read_u64()); break;
        case compiler::OpCode::PUSH_CONST_F32: {
            const float value = read_f32();
            uint32_t raw = 0;
            std::memcpy(&raw, &value, sizeof(raw));
            push(raw);
            break;
        }
        case compiler::OpCode::PUSH_CONST_F64: {
            const double value = read_f64();
            uint64_t raw = 0;
            std::memcpy(&raw, &value, sizeof(raw));
            push(raw);
            break;
        }
        case compiler::OpCode::PUSH_CONST_STRING: {
            string_pool.push_back(read_string());
            push(encode_string_ref(string_pool.size() - 1));
            break;
        }
        case compiler::OpCode::PUSH_TRUE: push(1); break;
        case compiler::OpCode::PUSH_FALSE: push(0); break;
        case compiler::OpCode::PUSH_NULL: push(0); break;
        case compiler::OpCode::POP: (void)pop(); break;
        case compiler::OpCode::DUP: {
            const uint64_t v = pop();
            push(v);
            push(v);
            break;
        }
        case compiler::OpCode::ADD: {
            const int64_t b = as_i64(pop());
            const int64_t a = as_i64(pop());
            push(as_u64(a + b));
            break;
        }
        case compiler::OpCode::SUBTRACT: {
            const int64_t b = as_i64(pop());
            const int64_t a = as_i64(pop());
            push(as_u64(a - b));
            break;
        }
        case compiler::OpCode::MULTIPLY: {
            const int64_t b = as_i64(pop());
            const int64_t a = as_i64(pop());
            push(as_u64(a * b));
            break;
        }
        case compiler::OpCode::DIVIDE: {
            const int64_t b = as_i64(pop());
            if (b == 0) throw std::runtime_error("Division by zero");
            const int64_t a = as_i64(pop());
            push(as_u64(a / b));
            break;
        }
        case compiler::OpCode::MODULO: {
            const int64_t b = as_i64(pop());
            if (b == 0) throw std::runtime_error("Modulo by zero");
            const int64_t a = as_i64(pop());
            push(as_u64(a % b));
            break;
        }
        case compiler::OpCode::EQUAL: {
            const uint64_t b = pop();
            const uint64_t a = pop();
            push(a == b ? 1 : 0);
            break;
        }
        case compiler::OpCode::NOT_EQUAL: {
            const uint64_t b = pop();
            const uint64_t a = pop();
            push(a != b ? 1 : 0);
            break;
        }
        case compiler::OpCode::GREATER: {
            const int64_t b = as_i64(pop());
            const int64_t a = as_i64(pop());
            push(a > b ? 1 : 0);
            break;
        }
        case compiler::OpCode::GREATER_EQUAL: {
            const int64_t b = as_i64(pop());
            const int64_t a = as_i64(pop());
            push(a >= b ? 1 : 0);
            break;
        }
        case compiler::OpCode::LESS: {
            const int64_t b = as_i64(pop());
            const int64_t a = as_i64(pop());
            push(a < b ? 1 : 0);
            break;
        }
        case compiler::OpCode::LESS_EQUAL: {
            const int64_t b = as_i64(pop());
            const int64_t a = as_i64(pop());
            push(a <= b ? 1 : 0);
            break;
        }
        case compiler::OpCode::LOGICAL_NOT: {
            const uint64_t value = pop();
            push(!truthy(value) ? 1 : 0);
            break;
        }
        case compiler::OpCode::NEGATE: {
            const int64_t value = as_i64(pop());
            push(as_u64(-value));
            break;
        }
        case compiler::OpCode::INC: {
            const int64_t value = as_i64(pop());
            push(as_u64(value + 1));
            break;
        }
        case compiler::OpCode::DEC: {
            const int64_t value = as_i64(pop());
            push(as_u64(value - 1));
            break;
        }
        case compiler::OpCode::GET_LOCAL: {
            const uint32_t index = read_u32();
            auto& frame = current_frame();
            if (index >= frame.locals.size()) throw std::runtime_error("GET_LOCAL out of range");
            push(frame.locals[index]);
            break;
        }
        case compiler::OpCode::SET_LOCAL: {
            const uint32_t index = read_u32();
            auto& frame = current_frame();
            if (index >= frame.locals.size()) throw std::runtime_error("SET_LOCAL out of range");
            frame.locals[index] = pop();
            break;
        }
        case compiler::OpCode::GET_GLOBAL: {
            const uint32_t index = read_u32();
            if (index >= static_globals.size()) throw std::runtime_error("GET_GLOBAL out of range");
            push(static_globals[index]);
            break;
        }
        case compiler::OpCode::SET_GLOBAL: {
            const uint32_t index = read_u32();
            if (index >= static_globals.size()) throw std::runtime_error("SET_GLOBAL out of range");
            static_globals[index] = pop();
            break;
        }
        case compiler::OpCode::JUMP: {
            const uint32_t target = read_u32();
            if (target >= bytecode.size()) throw std::runtime_error("JUMP target out of range");
            program_counter = target;
            break;
        }
        case compiler::OpCode::JUMP_IF_FALSE: {
            const uint32_t target = read_u32();
            const uint64_t cond = pop();
            if (!truthy(cond)) {
                if (target >= bytecode.size()) throw std::runtime_error("JUMP_IF_FALSE target out of range");
                program_counter = target;
            }
            break;
        }
        case compiler::OpCode::JUMP_IF_TRUE: {
            const uint32_t target = read_u32();
            const uint64_t cond = pop();
            if (truthy(cond)) {
                if (target >= bytecode.size()) throw std::runtime_error("JUMP_IF_TRUE target out of range");
                program_counter = target;
            }
            break;
        }
        case compiler::OpCode::ALLOC_STATIC: {
            const uint32_t count = static_cast<uint32_t>(pop());
            static_globals.assign(count, 0);
            break;
        }
        case compiler::OpCode::ALLOC_DYNAMIC: {
            const uint32_t size = static_cast<uint32_t>(pop());
            push(memory_pool.allocate(size));
            break;
        }
        case compiler::OpCode::GET_PROPERTY: {
            const uint32_t offset = read_u32();
            const uint32_t address = static_cast<uint32_t>(pop());
            push(memory_pool.read_heap_word(address, offset));
            break;
        }
        case compiler::OpCode::SET_PROPERTY: {
            const uint32_t offset = read_u32();
            const uint64_t value = pop();
            const uint32_t address = static_cast<uint32_t>(pop());
            memory_pool.write_heap_word(address, offset, value);
            break;
        }
        case compiler::OpCode::GET_ARRAY: {
            const uint32_t index = static_cast<uint32_t>(pop());
            const uint32_t address = static_cast<uint32_t>(pop());
            push(memory_pool.read_heap_word(address, index));
            break;
        }
        case compiler::OpCode::SET_ARRAY: {
            const uint64_t value = pop();
            const uint32_t index = static_cast<uint32_t>(pop());
            const uint32_t address = static_cast<uint32_t>(pop());
            memory_pool.write_heap_word(address, index, value);
            break;
        }
        case compiler::OpCode::INC_REF: {
            const uint32_t address = static_cast<uint32_t>(peek());
            garbage_collector.increase_reference(address);
            break;
        }
        case compiler::OpCode::DEC_REF: {
            const uint32_t address = static_cast<uint32_t>(pop());
            garbage_collector.decrease_reference(address);
            garbage_collector.collect();
            break;
        }
        case compiler::OpCode::CONV_I8: {
            push(as_u64(static_cast<int8_t>(as_i64(pop()))));
            break;
        }
        case compiler::OpCode::CONV_I16: {
            push(as_u64(static_cast<int16_t>(as_i64(pop()))));
            break;
        }
        case compiler::OpCode::CONV_I32: {
            push(as_u64(static_cast<int32_t>(as_i64(pop()))));
            break;
        }
        case compiler::OpCode::CONV_I64: {
            push(as_u64(static_cast<int64_t>(as_i64(pop()))));
            break;
        }
        case compiler::OpCode::CONV_U8: {
            push(static_cast<uint8_t>(pop()));
            break;
        }
        case compiler::OpCode::CONV_U16: {
            push(static_cast<uint16_t>(pop()));
            break;
        }
        case compiler::OpCode::CONV_U32: {
            push(static_cast<uint32_t>(pop()));
            break;
        }
        case compiler::OpCode::CONV_U64: {
            push(static_cast<uint64_t>(pop()));
            break;
        }
        case compiler::OpCode::CONV_F32: {
            const float v = static_cast<float>(as_i64(pop()));
            uint32_t raw = 0;
            std::memcpy(&raw, &v, sizeof(raw));
            push(raw);
            break;
        }
        case compiler::OpCode::CONV_F64: {
            const double v = static_cast<double>(as_i64(pop()));
            uint64_t raw = 0;
            std::memcpy(&raw, &v, sizeof(raw));
            push(raw);
            break;
        }
        case compiler::OpCode::CALL: {
            const uint32_t arg_count = static_cast<uint32_t>(pop());
            const uint32_t frame_size = static_cast<uint32_t>(pop());
            const uint32_t entry_ip = static_cast<uint32_t>(pop());

            std::vector<uint64_t> args(arg_count, 0);
            for (uint32_t i = 0; i < arg_count; ++i) {
                args[arg_count - i - 1] = pop();
            }

            CallFrame frame;
            frame.return_address = program_counter;
            frame.locals.assign(frame_size, 0);
            for (uint32_t i = 0; i < arg_count && i < frame.locals.size(); ++i) {
                frame.locals[i] = args[i];
            }

            call_frames.push_back(std::move(frame));
            if (entry_ip >= bytecode.size()) throw std::runtime_error("CALL target out of range");
            program_counter = entry_ip;
            break;
        }
        case compiler::OpCode::CALL_NATIVE: {
            const uint32_t native_id = read_u32();
            auto handler = native_handlers.find(native_id);
            if (handler != native_handlers.end()) {
                handler->second(*this);
            } else {
                push(0);
            }
            break;
        }
        case compiler::OpCode::DEFINE_NATIVE: {
            const uint32_t native_id = read_u32();
            const std::string symbol = read_string();
            define_native_handler(native_id, symbol);
            break;
        }
        case compiler::OpCode::RETURN: {
            if (call_frames.empty()) {
                halted = true;
                break;
            }
            const uint64_t return_ip = call_frames.back().return_address;
            call_frames.pop_back();
            program_counter = return_ip;
            break;
        }
        case compiler::OpCode::HALT: {
            halted = true;
            break;
        }
    }
}

void Program::run() {
    while (!halted) {
        progress();
    }
}

} // namespace runtime
} // namespace solix
