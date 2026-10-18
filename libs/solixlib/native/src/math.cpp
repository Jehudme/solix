#include "solix/native.h"
#include "solix/runtime.hpp"
#include "solix/native_registry.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <random>
#include <unordered_map>
#include <mutex>
#include <chrono>

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

// PRNG handle management
static std::mutex g_rng_mutex;
static std::unordered_map<uint64_t, std::mt19937_64> g_rng_instances;
static uint64_t g_next_rng_id = 1;

static uint64_t math_native_sqrt(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::sqrt(x));
}

static uint64_t math_native_cbrt(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::cbrt(x));
}

static uint64_t math_native_hypot(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    double y = bit_cast_from_u64<double>(args[1]);
    return bit_cast_to_u64(std::hypot(x, y));
}

static uint64_t math_native_pow(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double b = bit_cast_from_u64<double>(args[0]);
    double e = bit_cast_from_u64<double>(args[1]);
    return bit_cast_to_u64(std::pow(b, e));
}

static uint64_t math_native_exp(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::exp(x));
}

static uint64_t math_native_log(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::log(x));
}

static uint64_t math_native_log10(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::log10(x));
}

static uint64_t math_native_log2(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::log2(x));
}

static uint64_t math_native_sin(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::sin(x));
}

static uint64_t math_native_cos(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::cos(x));
}

static uint64_t math_native_tan(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::tan(x));
}

static uint64_t math_native_asin(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::asin(x));
}

static uint64_t math_native_acos(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::acos(x));
}

static uint64_t math_native_atan(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::atan(x));
}

static uint64_t math_native_atan2(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double y = bit_cast_from_u64<double>(args[0]);
    double x = bit_cast_from_u64<double>(args[1]);
    return bit_cast_to_u64(std::atan2(y, x));
}

static uint64_t math_native_sinh(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::sinh(x));
}

static uint64_t math_native_cosh(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::cosh(x));
}

static uint64_t math_native_tanh(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::tanh(x));
}

static uint64_t math_native_floor(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::floor(x));
}

static uint64_t math_native_ceil(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::ceil(x));
}

static uint64_t math_native_round(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::round(x));
}

static uint64_t math_native_trunc(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    double x = bit_cast_from_u64<double>(args[0]);
    return bit_cast_to_u64(std::trunc(x));
}

// PRNG native functions
static uint64_t random_native_init(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self;
    uint64_t seed;
    if (argc > 0 && args[0] != 0) {
        seed = args[0];
    } else {
        seed = static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    }

    std::lock_guard<std::mutex> lock(g_rng_mutex);
    uint64_t handle = g_next_rng_id++;
    g_rng_instances[handle] = std::mt19937_64(seed);
    return handle;
}

static uint64_t random_native_next_u64(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    uint64_t handle = args[0];
    std::lock_guard<std::mutex> lock(g_rng_mutex);
    auto it = g_rng_instances.find(handle);
    if (it == g_rng_instances.end()) {
        return 0;
    }
    return it->second();
}

static uint64_t random_native_next_double(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    uint64_t handle = args[0];
    std::lock_guard<std::mutex> lock(g_rng_mutex);
    auto it = g_rng_instances.find(handle);
    if (it == g_rng_instances.end()) {
        return 0;
    }
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double val = dist(it->second);
    return bit_cast_to_u64(val);
}

} // namespace

void register_math_natives(solix::NativeRegistry &registry) {
    auto reg_math = [&](const std::string &name, solix::NativeFunctionPtr func) {
        registry.register_function("Math_" + name, func);
        registry.register_function("solix_math_Math_" + name, func);
    };

    auto reg_rand = [&](const std::string &name, solix::NativeFunctionPtr func) {
        registry.register_function("Random_" + name, func);
        registry.register_function("solix_math_Random_" + name, func);
    };

    reg_math("native_sqrt", math_native_sqrt);
    reg_math("native_cbrt", math_native_cbrt);
    reg_math("native_hypot", math_native_hypot);
    reg_math("native_pow", math_native_pow);
    reg_math("native_exp", math_native_exp);
    reg_math("native_log", math_native_log);
    reg_math("native_log10", math_native_log10);
    reg_math("native_log2", math_native_log2);
    reg_math("native_sin", math_native_sin);
    reg_math("native_cos", math_native_cos);
    reg_math("native_tan", math_native_tan);
    reg_math("native_asin", math_native_asin);
    reg_math("native_acos", math_native_acos);
    reg_math("native_atan", math_native_atan);
    reg_math("native_atan2", math_native_atan2);
    reg_math("native_sinh", math_native_sinh);
    reg_math("native_cosh", math_native_cosh);
    reg_math("native_tanh", math_native_tanh);
    reg_math("native_floor", math_native_floor);
    reg_math("native_ceil", math_native_ceil);
    reg_math("native_round", math_native_round);
    reg_math("native_trunc", math_native_trunc);

    reg_rand("native_init", random_native_init);
    reg_rand("native_next_u64", random_native_next_u64);
    reg_rand("native_next_double", random_native_next_double);
}
