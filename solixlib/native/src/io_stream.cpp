#include "solix/native.h"
#include "solix/runtime.hpp"
#include "solix/native_registry.hpp"
#include <cstdio>
#include <string>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace {

static std::unordered_map<int64_t, FILE*> s_streams;
static std::mutex s_mutex;
static int64_t s_next_stream_id = 1;

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

static uint64_t stream_native_open(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string path = read_solix_chars(vm, args[0]);
    int32_t mode = static_cast<int32_t>(args[1]);

    const char *mode_str = "rb";
    switch (mode) {
        case 0: mode_str = "rb"; break;       // READ
        case 1: mode_str = "wb+"; break;      // WRITE (create/truncate)
        case 2: mode_str = "ab+"; break;      // APPEND
        case 3: mode_str = "r+b"; break;      // READ_WRITE (existing)
        default: mode_str = "rb"; break;
    }

    FILE *fp = std::fopen(path.c_str(), mode_str);
    if (!fp && mode == 3) {
        // Fallback for ReadWrite creating new file
        fp = std::fopen(path.c_str(), "w+b");
    }

    if (!fp) {
        return static_cast<uint64_t>(-1LL);
    }

    std::lock_guard<std::mutex> lock(s_mutex);
    int64_t handle = s_next_stream_id++;
    s_streams[handle] = fp;
    return static_cast<uint64_t>(handle);
}

static uint64_t stream_native_close(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    int64_t handle = static_cast<int64_t>(args[0]);

    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_streams.find(handle);
    if (it != s_streams.end()) {
        std::fclose(it->second);
        s_streams.erase(it);
        return 0ULL;
    }
    return 1ULL;
}

static uint64_t stream_native_read_byte(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    int64_t handle = static_cast<int64_t>(args[0]);

    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_streams.find(handle);
    if (it == s_streams.end() || !it->second) {
        return static_cast<uint64_t>(-1LL);
    }

    int ch = std::fgetc(it->second);
    if (ch == EOF) {
        return static_cast<uint64_t>(-1LL);
    }
    return static_cast<uint64_t>(static_cast<uint8_t>(ch));
}

static uint64_t stream_native_write_byte(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    int64_t handle = static_cast<int64_t>(args[0]);
    int32_t val = static_cast<int32_t>(args[1]);

    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_streams.find(handle);
    if (it == s_streams.end() || !it->second) {
        return 0ULL;
    }

    std::fputc(static_cast<unsigned char>(val), it->second);
    return 0ULL;
}

static uint64_t stream_native_read(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int64_t handle = static_cast<int64_t>(args[0]);
    uint64_t buf_addr = args[1];
    int32_t offset = static_cast<int32_t>(args[2]);
    int32_t count = static_cast<int32_t>(args[3]);

    if (buf_addr == 0 || count <= 0) return 0ULL;

    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_streams.find(handle);
    if (it == s_streams.end() || !it->second) {
        return static_cast<uint64_t>(-1LL);
    }

    std::vector<uint8_t> temp(static_cast<size_t>(count));
    size_t read_bytes = std::fread(temp.data(), 1, static_cast<size_t>(count), it->second);
    for (size_t i = 0; i < read_bytes; ++i) {
        vm.memory.heap[buf_addr + offset + i] = static_cast<uint64_t>(temp[i]);
    }
    return static_cast<uint64_t>(read_bytes);
}

static uint64_t stream_native_write(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    int64_t handle = static_cast<int64_t>(args[0]);
    uint64_t buf_addr = args[1];
    int32_t offset = static_cast<int32_t>(args[2]);
    int32_t count = static_cast<int32_t>(args[3]);

    if (buf_addr == 0 || count <= 0) return 0ULL;

    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_streams.find(handle);
    if (it == s_streams.end() || !it->second) {
        return static_cast<uint64_t>(-1LL);
    }

    std::vector<uint8_t> temp(static_cast<size_t>(count));
    for (size_t i = 0; i < static_cast<size_t>(count); ++i) {
        temp[i] = static_cast<uint8_t>(vm.memory.heap[buf_addr + offset + i]);
    }
    size_t written = std::fwrite(temp.data(), 1, static_cast<size_t>(count), it->second);
    return static_cast<uint64_t>(written);
}

static uint64_t stream_native_seek(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    int64_t handle = static_cast<int64_t>(args[0]);
    int64_t offset = static_cast<int64_t>(args[1]);
    int32_t origin = static_cast<int32_t>(args[2]);

    int whence = SEEK_SET;
    if (origin == 1) whence = SEEK_CUR;
    else if (origin == 2) whence = SEEK_END;

    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_streams.find(handle);
    if (it == s_streams.end() || !it->second) {
        return static_cast<uint64_t>(-1LL);
    }

    std::fseek(it->second, static_cast<long>(offset), whence);
    long pos = std::ftell(it->second);
    return static_cast<uint64_t>(pos);
}

static uint64_t stream_native_tell(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    int64_t handle = static_cast<int64_t>(args[0]);

    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_streams.find(handle);
    if (it == s_streams.end() || !it->second) {
        return static_cast<uint64_t>(-1LL);
    }

    long pos = std::ftell(it->second);
    return static_cast<uint64_t>(pos);
}

static uint64_t stream_native_length(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    int64_t handle = static_cast<int64_t>(args[0]);

    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_streams.find(handle);
    if (it == s_streams.end() || !it->second) {
        return static_cast<uint64_t>(-1LL);
    }

    FILE *fp = it->second;
    long curr = std::ftell(fp);
    std::fseek(fp, 0, SEEK_END);
    long end = std::ftell(fp);
    std::fseek(fp, curr, SEEK_SET);
    return static_cast<uint64_t>(end);
}

static uint64_t stream_native_flush(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    int64_t handle = static_cast<int64_t>(args[0]);

    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_streams.find(handle);
    if (it != s_streams.end() && it->second) {
        std::fflush(it->second);
    }
    return 0ULL;
}

} // namespace

void register_io_stream_natives(solix::NativeRegistry &registry) {
    auto reg = [&](const std::string &cls, const std::string &name, solix::NativeFunctionPtr func) {
        registry.register_function(cls + "_" + name, func);
        registry.register_function("solix_io_" + cls + "_" + name, func);
        registry.register_function("solix.io." + cls + "." + name, func);
    };

    reg("FileStream", "native_open", stream_native_open);
    reg("FileStream", "native_close", stream_native_close);
    reg("FileStream", "native_read_byte", stream_native_read_byte);
    reg("FileStream", "native_write_byte", stream_native_write_byte);
    reg("FileStream", "native_read", stream_native_read);
    reg("FileStream", "native_write", stream_native_write);
    reg("FileStream", "native_seek", stream_native_seek);
    reg("FileStream", "native_tell", stream_native_tell);
    reg("FileStream", "native_length", stream_native_length);
    reg("FileStream", "native_flush", stream_native_flush);
}
