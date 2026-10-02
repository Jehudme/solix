#include "solix/native.h"
#include "solix/runtime.hpp"
#include "solix/native_registry.hpp"
#include <charconv>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cctype>

namespace {

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

static uint64_t string_native_from_int(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int64_t val = static_cast<int64_t>(args[0]);
    char buf[64];
    auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), val);
    std::string s(buf, ptr - buf);
    return allocate_solix_chars(vm, s);
}

static uint64_t string_native_from_double(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    double val = bit_cast_from_u64<double>(args[0]);
    int32_t prec = (argc > 1) ? static_cast<int32_t>(args[1]) : 6;
    if (prec < 0) prec = 6;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", prec, val);
    // Remove trailing zeros if desired or keep standard
    std::string s(buf);
    return allocate_solix_chars(vm, s);
}

static uint64_t string_native_from_bool(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    return allocate_solix_chars(vm, args[0] != 0 ? "true" : "false");
}

static uint64_t string_native_try_parse_int(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s = read_solix_chars(vm, args[0]);
    uint64_t out_box = args[1];
    if (s.empty()) return 0;

    int64_t val = 0;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
    if (ec != std::errc() || ptr != s.data() + s.size()) {
        return 0;
    }

    if (out_box != 0) {
        vm.memory.heap[out_box] = static_cast<uint64_t>(val);
    }
    return 1;
}

static uint64_t string_native_try_parse_double(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s = read_solix_chars(vm, args[0]);
    uint64_t out_box = args[1];
    if (s.empty()) return 0;

    char *endptr = nullptr;
    const char *start = s.c_str();
    double val = std::strtod(start, &endptr);
    if (endptr == start || *endptr != '\0') {
        return 0;
    }

    if (out_box != 0) {
        vm.memory.heap[out_box] = bit_cast_to_u64<double>(val);
    }
    return 1;
}

static uint64_t string_native_to_lower(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s = read_solix_chars(vm, args[0]);
    for (char &c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return allocate_solix_chars(vm, s);
}

static uint64_t string_native_to_upper(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s = read_solix_chars(vm, args[0]);
    for (char &c : s) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return allocate_solix_chars(vm, s);
}

static uint64_t string_native_replace(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s = read_solix_chars(vm, args[0]);
    std::string old_token = read_solix_chars(vm, args[1]);
    std::string new_token = read_solix_chars(vm, args[2]);
    if (old_token.empty()) return allocate_solix_chars(vm, s);

    std::string res;
    size_t last = 0;
    size_t pos = 0;
    while ((pos = s.find(old_token, last)) != std::string::npos) {
        res.append(s, last, pos - last);
        res.append(new_token);
        last = pos + old_token.length();
    }
    res.append(s, last, s.length() - last);
    return allocate_solix_chars(vm, res);
}

static uint64_t string_native_index_of(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s = read_solix_chars(vm, args[0]);
    std::string needle = read_solix_chars(vm, args[1]);
    int32_t start = (argc > 2) ? static_cast<int32_t>(args[2]) : 0;
    if (start < 0) start = 0;
    if (static_cast<size_t>(start) >= s.length()) {
        return static_cast<uint64_t>(-1LL);
    }
    size_t pos = s.find(needle, start);
    return (pos == std::string::npos) ? static_cast<uint64_t>(-1LL) : static_cast<uint64_t>(pos);
}

static uint64_t string_native_last_index_of(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string s = read_solix_chars(vm, args[0]);
    std::string needle = read_solix_chars(vm, args[1]);
    size_t pos = s.rfind(needle);
    return (pos == std::string::npos) ? static_cast<uint64_t>(-1LL) : static_cast<uint64_t>(pos);
}

} // namespace

void register_string_natives(solix::NativeRegistry &registry) {
    auto reg = [&](const std::string &name, solix::NativeFunctionPtr func) {
        registry.register_function("String_" + name, func);
        registry.register_function("solix_core_String_" + name, func);
    };

    reg("native_from_int", string_native_from_int);
    reg("native_from_double", string_native_from_double);
    reg("native_from_bool", string_native_from_bool);
    reg("native_try_parse_int", string_native_try_parse_int);
    reg("native_try_parse_double", string_native_try_parse_double);
    reg("native_to_lower", string_native_to_lower);
    reg("native_to_upper", string_native_to_upper);
    reg("native_replace", string_native_replace);
    reg("native_index_of", string_native_index_of);
    reg("native_last_index_of", string_native_last_index_of);
}
