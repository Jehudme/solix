#include "solix/native.h"
#include "solix/runtime.hpp"
#include "solix/native_registry.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <thread>
#include <cstdint>

namespace {

static uint64_t allocate_solix_chars(solix::RuntimeContext &vm, const std::string &s) {
    uint64_t addr = vm.memory.dynamic_allocation(s.length());
    for (size_t i = 0; i < s.length(); ++i) {
        vm.memory.heap[addr + i] = static_cast<uint64_t>(static_cast<unsigned char>(s[i]));
    }
    return addr;
}

static uint64_t time_native_steady_nanos(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)args; (void)argc;
    auto now = std::chrono::steady_clock::now();
    return static_cast<uint64_t>(now.time_since_epoch().count());
}

static uint64_t time_native_system_millis(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)args; (void)argc;
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return static_cast<uint64_t>(ms);
}

static uint64_t time_native_utc_parts(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int64_t epoch_ms = static_cast<int64_t>(args[0]);
    uint64_t out_arr = args[1];

    std::time_t secs = static_cast<std::time_t>(epoch_ms / 1000);
    int64_t millis = epoch_ms % 1000;
    if (millis < 0) {
        millis += 1000;
        secs -= 1;
    }

    std::tm tm_buf;
#if defined(_WIN32)
    gmtime_s(&tm_buf, &secs);
#else
    gmtime_r(&secs, &tm_buf);
#endif

    if (out_arr != 0) {
        vm.memory.heap[out_arr + 0] = static_cast<uint64_t>(tm_buf.tm_year + 1900);
        vm.memory.heap[out_arr + 1] = static_cast<uint64_t>(tm_buf.tm_mon + 1);
        vm.memory.heap[out_arr + 2] = static_cast<uint64_t>(tm_buf.tm_mday);
        vm.memory.heap[out_arr + 3] = static_cast<uint64_t>(tm_buf.tm_hour);
        vm.memory.heap[out_arr + 4] = static_cast<uint64_t>(tm_buf.tm_min);
        vm.memory.heap[out_arr + 5] = static_cast<uint64_t>(tm_buf.tm_sec);
        vm.memory.heap[out_arr + 6] = static_cast<uint64_t>(millis);
    }
    return 0;
}

static uint64_t time_native_local_parts(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int64_t epoch_ms = static_cast<int64_t>(args[0]);
    uint64_t out_arr = args[1];

    std::time_t secs = static_cast<std::time_t>(epoch_ms / 1000);
    int64_t millis = epoch_ms % 1000;
    if (millis < 0) {
        millis += 1000;
        secs -= 1;
    }

    std::tm tm_buf;
#if defined(_WIN32)
    localtime_s(&tm_buf, &secs);
#else
    localtime_r(&secs, &tm_buf);
#endif

    if (out_arr != 0) {
        vm.memory.heap[out_arr + 0] = static_cast<uint64_t>(tm_buf.tm_year + 1900);
        vm.memory.heap[out_arr + 1] = static_cast<uint64_t>(tm_buf.tm_mon + 1);
        vm.memory.heap[out_arr + 2] = static_cast<uint64_t>(tm_buf.tm_mday);
        vm.memory.heap[out_arr + 3] = static_cast<uint64_t>(tm_buf.tm_hour);
        vm.memory.heap[out_arr + 4] = static_cast<uint64_t>(tm_buf.tm_min);
        vm.memory.heap[out_arr + 5] = static_cast<uint64_t>(tm_buf.tm_sec);
        vm.memory.heap[out_arr + 6] = static_cast<uint64_t>(millis);
    }
    return 0;
}

static uint64_t time_native_iso8601(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int64_t epoch_ms = static_cast<int64_t>(args[0]);
    std::time_t secs = static_cast<std::time_t>(epoch_ms / 1000);
    int64_t millis = epoch_ms % 1000;
    if (millis < 0) {
        millis += 1000;
        secs -= 1;
    }

    std::tm tm_buf;
#if defined(_WIN32)
    gmtime_s(&tm_buf, &secs);
#else
    gmtime_r(&secs, &tm_buf);
#endif

    std::ostringstream ss;
    ss << std::setfill('0')
       << std::setw(4) << (tm_buf.tm_year + 1900) << "-"
       << std::setw(2) << (tm_buf.tm_mon + 1) << "-"
       << std::setw(2) << tm_buf.tm_mday << "T"
       << std::setw(2) << tm_buf.tm_hour << ":"
       << std::setw(2) << tm_buf.tm_min << ":"
       << std::setw(2) << tm_buf.tm_sec << "."
       << std::setw(3) << millis << "Z";

    return allocate_solix_chars(vm, ss.str());
}

static uint64_t time_native_sleep_millis(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    int64_t ms = static_cast<int64_t>(args[0]);
    if (ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
    return 0;
}

} // namespace

void register_time_natives(solix::NativeRegistry &registry) {
    auto reg_instant = [&](const std::string &name, solix::NativeFunctionPtr func) {
        registry.register_function("Instant_" + name, func);
        registry.register_function("solix_time_Instant_" + name, func);
    };

    auto reg_datetime = [&](const std::string &name, solix::NativeFunctionPtr func) {
        registry.register_function("DateTime_" + name, func);
        registry.register_function("solix_time_DateTime_" + name, func);
    };

    auto reg_stopwatch = [&](const std::string &name, solix::NativeFunctionPtr func) {
        registry.register_function("Stopwatch_" + name, func);
        registry.register_function("solix_time_Stopwatch_" + name, func);
    };

    reg_instant("native_steady_nanos", time_native_steady_nanos);

    reg_datetime("native_system_millis", time_native_system_millis);
    reg_datetime("native_utc_parts", time_native_utc_parts);
    reg_datetime("native_local_parts", time_native_local_parts);
    reg_datetime("native_iso8601", time_native_iso8601);

    reg_stopwatch("native_steady_nanos", time_native_steady_nanos);
    reg_stopwatch("native_sleep_millis", time_native_sleep_millis);
}

