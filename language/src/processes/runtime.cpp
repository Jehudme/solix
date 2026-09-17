#include "solix/runtime.hpp"
#include "solix/utilities/optcodes.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <cmath>
#include <iomanip>

namespace solix {

static std::unordered_map<std::string, NativeFunction> global_native_registry;

void register_native_function(const std::string& name, NativeFunction func) {
    global_native_registry[name] = std::move(func);
}

uint64_t Memory::static_allocation(size_t size_in_words, Address address) {
    if (address == 0) {
        address = next_free_static;
        next_free_static += size_in_words;
        if (next_free_static >= heap.size()) {
            throw std::runtime_error("Heap overflow during static allocation!");
        }
    }
    currently_used_words += size_in_words;
    if (currently_used_words > peak_used_words) peak_used_words = currently_used_words;
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
        if (next_free_dynamic >= heap.size()) throw std::runtime_error("Heap overflow during dynamic allocation!");
    }
    currently_used_words += (size_in_words + 1);
    if (currently_used_words > peak_used_words) peak_used_words = currently_used_words;

    uint64_t header = (static_cast<uint64_t>(size_in_words) << 32) | 0ULL;
    heap[header_addr] = header;
    return header_addr + 1; 
}

void Memory::deallocate(Address address) {
    if (address == 0) return;
    Address header_addr = address - 1;
    uint64_t header = heap[header_addr];
    uint32_t size = static_cast<uint32_t>(header >> 32);
    currently_used_words -= (size + 1);
    free_blocks.push_back(header_addr);
}

inline void Memory::write_u64(Address address, uint32_t offset, uint64_t value) { heap[address + offset] = value; }
inline uint64_t Memory::read_u64(Address address, uint32_t offset) const { return heap[address + offset]; }
inline void Memory::write_f64(Address address, uint32_t offset, double value) {
    uint64_t val; std::memcpy(&val, &value, 8); heap[address + offset] = val;
}
inline double Memory::read_f64(Address address, uint32_t offset) const {
    double val; uint64_t raw = heap[address + offset]; std::memcpy(&val, &raw, 8); return val;
}
inline void Memory::write_char(Address address, uint32_t offset, char value) { heap[address + offset] = static_cast<uint64_t>(value); }
inline char Memory::read_char(Address address, uint32_t offset) const { return static_cast<char>(heap[address + offset]); }

inline void Memory::increase_reference(Address address) {
    if (address == 0) return;
    heap[address - 1]++;
}

inline void Memory::decrease_reference(Address address) {
    if (address == 0) return;
    uint64_t& header = heap[address - 1];
    uint32_t ref_count = static_cast<uint32_t>(header & 0xFFFFFFFF);
    if (ref_count > 0) {
        ref_count--;
        header = (header & 0xFFFFFFFF00000000ULL) | ref_count;
        if (ref_count == 0) deallocate(address);
    }
}

RuntimeContext::RuntimeContext(const RuntimeOptions& opts)
    : options(opts), memory(opts.stack_capacity, opts.heap_capacity) {
    
    if (std::holds_alternative<Bytecode>(opts.bytecode_source)) {
        bytecode = std::get<Bytecode>(opts.bytecode_source);
    } else {
        auto path = std::get<std::filesystem::path>(opts.bytecode_source);
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) throw std::runtime_error("Could not open bytecode file: " + path.string());
        
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        bytecode.resize(size);
        if (!file.read(reinterpret_cast<char*>(bytecode.data()), size)) {
            throw std::runtime_error("Failed to read bytecode file: " + path.string());
        }
    }
}

void RuntimeContext::register_native(uint32_t id, NativeFunction func) {
    native_registry[id] = std::move(func);
}

inline void RuntimeContext::push(uint64_t val) { memory.stack[memory.stack_pointer++] = val; }
inline uint64_t RuntimeContext::pop() { return memory.stack[--memory.stack_pointer]; }

static inline uint32_t read_u32(const Bytecode& bcode, Address& pc) {
    uint32_t val = (bcode[pc] << 24) | (bcode[pc+1] << 16) | (bcode[pc+2] << 8) | bcode[pc+3];
    pc += 4; return val;
}
static inline uint64_t read_u64(const Bytecode& bcode, Address& pc) {
    uint64_t val = (static_cast<uint64_t>(bcode[pc]) << 56) |
                   (static_cast<uint64_t>(bcode[pc+1]) << 48) | (static_cast<uint64_t>(bcode[pc+2]) << 40) |
                   (static_cast<uint64_t>(bcode[pc+3]) << 32) | (static_cast<uint64_t>(bcode[pc+4]) << 24) |
                   (static_cast<uint64_t>(bcode[pc+5]) << 16) | (static_cast<uint64_t>(bcode[pc+6]) << 8) |
                   static_cast<uint64_t>(bcode[pc+7]);
    pc += 8; return val;
}
static inline std::string read_string(const Bytecode& bcode, Address& pc) {
    uint32_t length = read_u32(bcode, pc);
    std::string str(reinterpret_cast<const char*>(&bcode[pc]), length);
    pc += length; return str;
}

template <typename T> inline uint64_t bit_cast_to_u64(T value) { uint64_t result = 0; std::memcpy(&result, &value, sizeof(T)); return result; }
template <typename T> inline T bit_cast_from_u64(uint64_t value) { T result; std::memcpy(&result, &value, sizeof(T)); return result; }

void RuntimeContext::execute() {
    const uint8_t* code = bytecode.data();
    uint64_t* stack = memory.stack.data();
    uint64_t* heap_data = memory.heap.data();
    
    // Add an initial frame so we don't underflow
    call_stack.emplace_back(0, 0);

    while (program_counter < bytecode.size()) {
        uint8_t op = code[program_counter++];
        
        switch (op) {
            case static_cast<uint8_t>(OpCode::PUSH_CONST_I8):
            case static_cast<uint8_t>(OpCode::PUSH_CONST_I16):
            case static_cast<uint8_t>(OpCode::PUSH_CONST_I32): {
                push(static_cast<uint64_t>(read_u32(bytecode, program_counter))); break;
            }
            case static_cast<uint8_t>(OpCode::PUSH_CONST_I64):
            case static_cast<uint8_t>(OpCode::PUSH_CONST_U64):
            case static_cast<uint8_t>(OpCode::PUSH_CONST_F64): {
                push(read_u64(bytecode, program_counter)); break;
            }
            case static_cast<uint8_t>(OpCode::PUSH_CONST_U8):
            case static_cast<uint8_t>(OpCode::PUSH_CONST_U16):
            case static_cast<uint8_t>(OpCode::PUSH_CONST_U32):
            case static_cast<uint8_t>(OpCode::PUSH_CONST_F32): {
                push(static_cast<uint64_t>(read_u32(bytecode, program_counter))); break;
            }
            case static_cast<uint8_t>(OpCode::PUSH_CONST_STRING): {
                std::string str = read_string(bytecode, program_counter);
                size_t len = str.length();
                Address addr = memory.dynamic_allocation(len + 1); 
                heap_data[addr] = len; 
                for (size_t i = 0; i < len; ++i) heap_data[addr + 1 + i] = static_cast<uint64_t>(str[i]);
                push(addr);
                break;
            }
            case static_cast<uint8_t>(OpCode::PUSH_TRUE): push(1); break;
            case static_cast<uint8_t>(OpCode::PUSH_FALSE): push(0); break;
            case static_cast<uint8_t>(OpCode::PUSH_NULL): push(0); break;
            case static_cast<uint8_t>(OpCode::POP): pop(); break;
            case static_cast<uint8_t>(OpCode::DUP): {
                uint64_t top = stack[memory.stack_pointer - 1]; push(top); break;
            }

            case static_cast<uint8_t>(OpCode::ADD): {
                double b = bit_cast_from_u64<double>(pop());
                double a = bit_cast_from_u64<double>(pop());
                push(bit_cast_to_u64(a + b)); break;
            }
            case static_cast<uint8_t>(OpCode::SUBTRACT): {
                double b = bit_cast_from_u64<double>(pop());
                double a = bit_cast_from_u64<double>(pop());
                push(bit_cast_to_u64(a - b)); break;
            }
            case static_cast<uint8_t>(OpCode::MULTIPLY): {
                double b = bit_cast_from_u64<double>(pop());
                double a = bit_cast_from_u64<double>(pop());
                push(bit_cast_to_u64(a * b)); break;
            }
            case static_cast<uint8_t>(OpCode::DIVIDE): {
                double b = bit_cast_from_u64<double>(pop());
                double a = bit_cast_from_u64<double>(pop());
                push(bit_cast_to_u64(a / b)); break;
            }
            case static_cast<uint8_t>(OpCode::MODULO): {
                double b = bit_cast_from_u64<double>(pop());
                double a = bit_cast_from_u64<double>(pop());
                push(bit_cast_to_u64(std::fmod(a, b))); break;
            }
            case static_cast<uint8_t>(OpCode::EQUAL): {
                uint64_t b = pop(); uint64_t a = pop();
                push(a == b ? 1 : 0); break;
            }
            case static_cast<uint8_t>(OpCode::NOT_EQUAL): {
                uint64_t b = pop(); uint64_t a = pop();
                push(a != b ? 1 : 0); break;
            }
            case static_cast<uint8_t>(OpCode::GREATER): {
                double b = bit_cast_from_u64<double>(pop());
                double a = bit_cast_from_u64<double>(pop());
                push(a > b ? 1 : 0); break;
            }
            case static_cast<uint8_t>(OpCode::GREATER_EQUAL): {
                double b = bit_cast_from_u64<double>(pop());
                double a = bit_cast_from_u64<double>(pop());
                push(a >= b ? 1 : 0); break;
            }
            case static_cast<uint8_t>(OpCode::LESS): {
                double b = bit_cast_from_u64<double>(pop());
                double a = bit_cast_from_u64<double>(pop());
                push(a < b ? 1 : 0); break;
            }
            case static_cast<uint8_t>(OpCode::LESS_EQUAL): {
                double b = bit_cast_from_u64<double>(pop());
                double a = bit_cast_from_u64<double>(pop());
                push(a <= b ? 1 : 0); break;
            }
            case static_cast<uint8_t>(OpCode::LOGICAL_NOT): {
                uint64_t a = pop();
                push(a == 0 ? 1 : 0); break;
            }
            case static_cast<uint8_t>(OpCode::NEGATE): {
                double a = bit_cast_from_u64<double>(pop());
                push(bit_cast_to_u64(-a)); break;
            }
            case static_cast<uint8_t>(OpCode::INC): {
                double a = bit_cast_from_u64<double>(pop());
                push(bit_cast_to_u64(a + 1.0)); break;
            }
            case static_cast<uint8_t>(OpCode::DEC): {
                double a = bit_cast_from_u64<double>(pop());
                push(bit_cast_to_u64(a - 1.0)); break;
            }

            case static_cast<uint8_t>(OpCode::GET_LOCAL): {
                uint32_t idx = read_u32(bytecode, program_counter);
                push(stack[call_stack.back().frame_pointer + idx]);
                break;
            }
            case static_cast<uint8_t>(OpCode::SET_LOCAL): {
                uint32_t idx = read_u32(bytecode, program_counter);
                uint64_t val = pop();
                uint32_t target_idx = call_stack.back().frame_pointer + idx;
                if (target_idx >= memory.stack_pointer) memory.stack_pointer = target_idx + 1;
                stack[target_idx] = val;
                break;
            }
            case static_cast<uint8_t>(OpCode::GET_GLOBAL): {
                uint32_t idx = read_u32(bytecode, program_counter);
                push(heap_data[idx]); 
                break;
            }
            case static_cast<uint8_t>(OpCode::SET_GLOBAL): {
                uint32_t idx = read_u32(bytecode, program_counter);
                uint64_t val = pop();
                heap_data[idx] = val;
                break;
            }

            case static_cast<uint8_t>(OpCode::JUMP): {
                uint32_t addr = read_u32(bytecode, program_counter);
                program_counter = addr;
                break;
            }
            case static_cast<uint8_t>(OpCode::JUMP_IF_FALSE): {
                uint32_t addr = read_u32(bytecode, program_counter);
                uint64_t cond = pop();
                if (cond == 0) program_counter = addr;
                break;
            }
            case static_cast<uint8_t>(OpCode::JUMP_IF_TRUE): {
                uint32_t addr = read_u32(bytecode, program_counter);
                uint64_t cond = pop();
                if (cond != 0) program_counter = addr;
                break;
            }

            case static_cast<uint8_t>(OpCode::ALLOC_STATIC): {
                uint32_t size = static_cast<uint32_t>(pop());
                memory.static_allocation(size, 0); 
                break;
            }
            case static_cast<uint8_t>(OpCode::ALLOC_DYNAMIC): {
                uint32_t size = static_cast<uint32_t>(pop());
                push(memory.dynamic_allocation(size));
                break;
            }
            case static_cast<uint8_t>(OpCode::GET_PROPERTY): {
                uint32_t offset = read_u32(bytecode, program_counter);
                Address obj = static_cast<Address>(pop());
                push(heap_data[obj + offset]);
                break;
            }
            case static_cast<uint8_t>(OpCode::SET_PROPERTY): {
                uint32_t offset = read_u32(bytecode, program_counter);
                Address obj = static_cast<Address>(pop());
                uint64_t val = pop();
                heap_data[obj + offset] = val;
                break;
            }
            case static_cast<uint8_t>(OpCode::GET_ARRAY): {
                uint32_t index = static_cast<uint32_t>(pop());
                Address array_addr = static_cast<Address>(pop());
                push(heap_data[array_addr + 1 + index]);
                break;
            }
            case static_cast<uint8_t>(OpCode::SET_ARRAY): {
                uint64_t val = pop();
                uint32_t index = static_cast<uint32_t>(pop());
                Address array_addr = static_cast<Address>(pop());
                heap_data[array_addr + 1 + index] = val;
                push(val);
                break;
            }

            case static_cast<uint8_t>(OpCode::INC_REF): {
                Address addr = static_cast<Address>(pop());
                memory.increase_reference(addr);
                push(addr);
                break;
            }
            case static_cast<uint8_t>(OpCode::DEC_REF): {
                Address addr = static_cast<Address>(pop());
                memory.decrease_reference(addr);
                break;
            }

            case static_cast<uint8_t>(OpCode::CONV_I8):
            case static_cast<uint8_t>(OpCode::CONV_I16):
            case static_cast<uint8_t>(OpCode::CONV_I32):
            case static_cast<uint8_t>(OpCode::CONV_I64):
            case static_cast<uint8_t>(OpCode::CONV_U8):
            case static_cast<uint8_t>(OpCode::CONV_U16):
            case static_cast<uint8_t>(OpCode::CONV_U32):
            case static_cast<uint8_t>(OpCode::CONV_U64):
            case static_cast<uint8_t>(OpCode::CONV_F32):
            case static_cast<uint8_t>(OpCode::CONV_F64): {
                // Everything is stored as 64-bit float/int natively, pass through for now
                break;
            }

            case static_cast<uint8_t>(OpCode::CALL): {
                uint32_t arg_count = static_cast<uint32_t>(pop());
                uint32_t frame_size = static_cast<uint32_t>(pop());
                uint32_t target_ip = static_cast<uint32_t>(pop());
                
                uint32_t new_frame_pointer = memory.stack_pointer - arg_count;
                call_stack.emplace_back(program_counter, new_frame_pointer);
                
                if (frame_size > arg_count) {
                    memory.stack_pointer += (frame_size - arg_count);
                }
                
                program_counter = target_ip;
                break;
            }
            case static_cast<uint8_t>(OpCode::CALL_NATIVE): {
                uint32_t id = read_u32(bytecode, program_counter);
                if (native_registry.count(id)) {
                    native_registry[id](*this, 0, nullptr, 0);
                } else {
                    throw std::runtime_error("Call to unknown native function: " + std::to_string(id));
                }
                break;
            }
            case static_cast<uint8_t>(OpCode::DEFINE_NATIVE): {
                uint32_t id = read_u32(bytecode, program_counter);
                std::string name = read_string(bytecode, program_counter);
                if (global_native_registry.count(name)) {
                    native_registry[id] = global_native_registry[name];
                } else {
                    std::cerr << "Warning: Native function " << name << " not found." << std::endl;
                }
                break;
            }
            case static_cast<uint8_t>(OpCode::RETURN): {
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
                break;
            }
            case static_cast<uint8_t>(OpCode::HALT):
                return;
                
            default:
                throw std::runtime_error("Unknown OpCode encountered! " + std::to_string(op));
        }
    }
}

} // namespace solix
