#include "solix/native.h"
#include "solix/runtime.hpp"
#include "solix/native_registry.hpp"
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {

static void ensure_terminal_initialized() {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) {
                dwMode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
                SetConsoleMode(hOut, dwMode);
            }
        }
        HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
        if (hErr != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            if (GetConsoleMode(hErr, &dwMode)) {
                dwMode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
                SetConsoleMode(hErr, dwMode);
            }
        }
#endif
    }
}

template <typename T> inline uint64_t bit_cast_to_u64(T value) {
    uint64_t result = 0;
    std::memcpy(&result, &value, sizeof(T));
    return result;
}

template <typename T> inline T bit_cast_from_u64(uint64_t value) {
    T result;
    std::memcpy(&result, &value, sizeof(T));
    return result;
}

static std::string read_solix_chars(solix::RuntimeContext &vm, uint64_t addr) {
    if (addr == 0) return "";
    uint32_t len = static_cast<uint32_t>(vm.memory.heap[addr - 1] >> 32);
    std::string s;
    s.reserve(len);
    for (uint32_t i = 0; i < len; ++i) {
        s.push_back(static_cast<char>(vm.memory.heap[addr + i]));
    }
    return s;
}

static uint64_t allocate_solix_chars(solix::RuntimeContext &vm, const std::string &s) {
    uint64_t addr = vm.memory.dynamic_allocation(s.length());
    for (size_t i = 0; i < s.length(); ++i) {
        vm.memory.heap[addr + i] = static_cast<uint64_t>(static_cast<unsigned char>(s[i]));
    }
    return addr;
}

static uint64_t native_print_string(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ensure_terminal_initialized();
    std::string s = read_solix_chars(vm, args[0]);
    std::cout << s;
    return 0;
}

static uint64_t native_print_int(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    ensure_terminal_initialized();
    std::cout << static_cast<int64_t>(args[0]);
    return 0;
}

static uint64_t native_print_double(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    ensure_terminal_initialized();
    double d = bit_cast_from_u64<double>(args[0]);
    std::cout << d;
    return 0;
}

static uint64_t native_print_bool(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    ensure_terminal_initialized();
    std::cout << (args[0] != 0 ? "true" : "false");
    return 0;
}

static uint64_t native_print_char(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    ensure_terminal_initialized();
    std::cout << static_cast<char>(args[0]);
    return 0;
}

static uint64_t native_println(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)args; (void)argc;
    ensure_terminal_initialized();
    std::cout << std::endl;
    return 0;
}

static uint64_t native_error_string(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ensure_terminal_initialized();
    std::string s = read_solix_chars(vm, args[0]);
    std::cerr << "\033[31m" << s << "\033[0m" << std::endl;
    return 0;
}

static uint64_t native_error_int(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    ensure_terminal_initialized();
    std::cerr << "\033[31m" << static_cast<int64_t>(args[0]) << "\033[0m" << std::endl;
    return 0;
}

static uint64_t native_warning_string(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ensure_terminal_initialized();
    std::string s = read_solix_chars(vm, args[0]);
    std::cout << "\033[33m" << s << "\033[0m" << std::endl;
    return 0;
}

static uint64_t native_warning_int(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    ensure_terminal_initialized();
    std::cout << "\033[33m" << static_cast<int64_t>(args[0]) << "\033[0m" << std::endl;
    return 0;
}

static uint64_t native_info_string(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ensure_terminal_initialized();
    std::string s = read_solix_chars(vm, args[0]);
    std::cout << "\033[36m" << s << "\033[0m" << std::endl;
    return 0;
}

static uint64_t native_info_int(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    ensure_terminal_initialized();
    std::cout << "\033[36m" << static_cast<int64_t>(args[0]) << "\033[0m" << std::endl;
    return 0;
}

static uint64_t native_success_string(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ensure_terminal_initialized();
    std::string s = read_solix_chars(vm, args[0]);
    std::cout << "\033[32m" << s << "\033[0m" << std::endl;
    return 0;
}

static uint64_t native_success_int(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    ensure_terminal_initialized();
    std::cout << "\033[32m" << static_cast<int64_t>(args[0]) << "\033[0m" << std::endl;
    return 0;
}

static uint64_t native_read_line(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)args; (void)argc;
    std::string line;
    if (std::getline(std::cin, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
    }
    return allocate_solix_chars(vm, line);
}

static uint64_t native_read_char(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)args; (void)argc;
    char c = 0;
    if (std::cin.get(c)) {
        return static_cast<uint64_t>(static_cast<unsigned char>(c));
    }
    return 0;
}

static uint64_t native_clear(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)args; (void)argc;
    ensure_terminal_initialized();
    std::cout << "\033[2J\033[H" << std::flush;
    return 0;
}

static uint64_t native_flush(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)args; (void)argc;
    std::cout.flush();
    std::cerr.flush();
    return 0;
}

static uint64_t native_set_color(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    ensure_terminal_initialized();
    std::cout << "\033[" << args[0] << "m";
    return 0;
}

static uint64_t native_reset_color(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    ensure_terminal_initialized();
    std::cout << "\033[0m";
    return 0;
}

static uint64_t native_set_cursor_position(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    ensure_terminal_initialized();
    std::cout << "\033[" << args[0] << ";" << args[1] << "H";
    return 0;
}

static uint64_t native_set_title(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    ensure_terminal_initialized();
    std::string s = read_solix_chars(vm, args[0]);
    std::cout << "\033]0;" << s << "\007";
    return 0;
}

static uint64_t native_try_parse_double(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s = read_solix_chars(vm, args[0]);
    uint64_t out_box_addr = args[1];
    if (s.empty()) return 0;

    char *endptr = nullptr;
    const char *start = s.c_str();
    double val = std::strtod(start, &endptr);
    if (endptr == start || *endptr != '\0') {
        return 0;
    }

    if (out_box_addr != 0) {
        vm.memory.heap[out_box_addr] = bit_cast_to_u64<double>(val);
    }
    return 1;
}

} // namespace

void register_console_natives(solix::NativeRegistry &registry) {
    auto reg = [&](const std::string &name, solix::NativeFunctionPtr func) {
        registry.register_function("Console_" + name, func);
        registry.register_function("solix_system_Console_" + name, func);
    };

    reg("native_print_string", native_print_string);
    reg("native_print_int", native_print_int);
    reg("native_print_double", native_print_double);
    reg("native_print_bool", native_print_bool);
    reg("native_print_char", native_print_char);
    reg("native_println", native_println);

    reg("native_error_string", native_error_string);
    reg("native_error_int", native_error_int);
    reg("native_warning_string", native_warning_string);
    reg("native_warning_int", native_warning_int);
    reg("native_info_string", native_info_string);
    reg("native_info_int", native_info_int);
    reg("native_success_string", native_success_string);
    reg("native_success_int", native_success_int);

    reg("native_read_line", native_read_line);
    reg("native_read_char", native_read_char);

    reg("native_clear", native_clear);
    reg("native_flush", native_flush);
    reg("native_set_color", native_set_color);
    reg("native_reset_color", native_reset_color);
    reg("native_set_cursor_position", native_set_cursor_position);
    reg("native_set_title", native_set_title);
    reg("native_try_parse_double", native_try_parse_double);
}
