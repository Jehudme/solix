#include "solix/compiler.hpp"
#include "solix/runtime.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    using solix::compiler::OpCode;

    auto emit_u32 = [](std::vector<uint8_t>& code, uint32_t value) {
        code.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
        code.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        code.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        code.push_back(static_cast<uint8_t>(value & 0xFF));
    };

    auto emit_string = [&](std::vector<uint8_t>& code, const std::string& value) {
        emit_u32(code, static_cast<uint32_t>(value.size()));
        code.insert(code.end(), value.begin(), value.end());
    };

    std::vector<uint8_t> bytecode;

    bytecode.push_back(static_cast<uint8_t>(OpCode::DEFINE_NATIVE));
    emit_u32(bytecode, 0);
    emit_string(bytecode, "print");

    bytecode.push_back(static_cast<uint8_t>(OpCode::PUSH_CONST_I32));
    emit_u32(bytecode, 12345);

    bytecode.push_back(static_cast<uint8_t>(OpCode::CALL_NATIVE));
    emit_u32(bytecode, 0);

    bytecode.push_back(static_cast<uint8_t>(OpCode::POP));
    bytecode.push_back(static_cast<uint8_t>(OpCode::HALT));

    try {
        solix::runtime::Program program(bytecode);
        program.run();
    } catch (const std::exception& ex) {
        std::cerr << "runtime test failed: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
