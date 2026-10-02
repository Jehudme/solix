#include "solix/native.h"
#include "solix/runtime.hpp"
#include "solix/native_registry.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

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

static uint64_t double_native_min_value(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)args; (void)argc;
    return bit_cast_to_u64(std::numeric_limits<double>::min());
}

static uint64_t double_native_max_value(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)args; (void)argc;
    return bit_cast_to_u64(std::numeric_limits<double>::max());
}

static uint64_t double_native_nan(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)args; (void)argc;
    return bit_cast_to_u64(std::numeric_limits<double>::quiet_NaN());
}

static uint64_t double_native_pos_inf(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)args; (void)argc;
    return bit_cast_to_u64(std::numeric_limits<double>::infinity());
}

static uint64_t double_native_neg_inf(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)args; (void)argc;
    return bit_cast_to_u64(-std::numeric_limits<double>::infinity());
}

static uint64_t double_native_is_nan(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double d = bit_cast_from_u64<double>(args[0]);
    return std::isnan(d) ? 1 : 0;
}

static uint64_t double_native_is_infinite(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double d = bit_cast_from_u64<double>(args[0]);
    return std::isinf(d) ? 1 : 0;
}

static uint64_t int_native_clz(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    uint32_t v = static_cast<uint32_t>(args[0]);
    return (v == 0) ? 32 : __builtin_clz(v);
}

static uint64_t int_native_ctz(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    uint32_t v = static_cast<uint32_t>(args[0]);
    return (v == 0) ? 32 : __builtin_ctz(v);
}

static uint64_t int_native_popcount(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    uint32_t v = static_cast<uint32_t>(args[0]);
    return __builtin_popcount(v);
}

static uint64_t int_native_bswap(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    uint32_t v = static_cast<uint32_t>(args[0]);
    return static_cast<uint64_t>(static_cast<int32_t>(__builtin_bswap32(v)));
}

} // namespace

void register_primitives_natives(solix::NativeRegistry &registry) {
    auto reg_double = [&](const std::string &name, solix::NativeFunctionPtr func) {
        registry.register_function("Double_" + name, func);
        registry.register_function("solix_core_Double_" + name, func);
    };

    auto reg_int = [&](const std::string &name, solix::NativeFunctionPtr func) {
        registry.register_function("Int_" + name, func);
        registry.register_function("solix_core_Int_" + name, func);
    };

    reg_double("native_min_value", double_native_min_value);
    reg_double("native_max_value", double_native_max_value);
    reg_double("native_nan", double_native_nan);
    reg_double("native_pos_inf", double_native_pos_inf);
    reg_double("native_neg_inf", double_native_neg_inf);
    reg_double("native_is_nan", double_native_is_nan);
    reg_double("native_is_infinite", double_native_is_infinite);

    reg_int("native_clz", int_native_clz);
    reg_int("native_ctz", int_native_ctz);
    reg_int("native_popcount", int_native_popcount);
    reg_int("native_bswap", int_native_bswap);
}
