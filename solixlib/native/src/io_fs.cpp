#include "solix/native.h"
#include "solix/runtime.hpp"
#include "solix/native_registry.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>

namespace fs = std::filesystem;

namespace {

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

// --- Path Native Functions ---

static uint64_t path_native_combine(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string p1 = read_solix_chars(vm, args[0]);
    std::string p2 = read_solix_chars(vm, args[1]);
    fs::path combined = fs::path(p1) / fs::path(p2);
    return allocate_solix_chars(vm, combined.generic_string());
}

static uint64_t path_native_get_directory_name(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string p = read_solix_chars(vm, args[0]);
    fs::path parent = fs::path(p).parent_path();
    return allocate_solix_chars(vm, parent.generic_string());
}

static uint64_t path_native_get_file_name(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string p = read_solix_chars(vm, args[0]);
    fs::path fn = fs::path(p).filename();
    return allocate_solix_chars(vm, fn.string());
}

static uint64_t path_native_get_extension(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string p = read_solix_chars(vm, args[0]);
    fs::path ext = fs::path(p).extension();
    return allocate_solix_chars(vm, ext.string());
}

static uint64_t path_native_get_file_name_without_extension(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string p = read_solix_chars(vm, args[0]);
    fs::path stem = fs::path(p).stem();
    return allocate_solix_chars(vm, stem.string());
}

static uint64_t path_native_is_absolute(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)argc;
    std::string p = read_solix_chars(vm, args[0]);
    return fs::path(p).is_absolute() ? 1ULL : 0ULL;
}

static uint64_t path_native_get_temp_path(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)args; (void)argc;
    std::error_code ec;
    fs::path tmp = fs::temp_directory_path(ec);
    return allocate_solix_chars(vm, tmp.generic_string());
}

static uint64_t path_native_normalize(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string p = read_solix_chars(vm, args[0]);
    fs::path norm = fs::path(p).lexically_normal();
    return allocate_solix_chars(vm, norm.generic_string());
}

static uint64_t path_native_get_separator(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)vm; (void)self; (void)args; (void)argc;
    return static_cast<uint64_t>(static_cast<unsigned char>(fs::path::preferred_separator));
}

// --- File Native Functions ---

static uint64_t file_native_exists(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string p = read_solix_chars(vm, args[0]);
    std::error_code ec;
    return fs::is_regular_file(p, ec) ? 1ULL : 0ULL;
}

static uint64_t file_native_read_all_text(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string p = read_solix_chars(vm, args[0]);
    uint64_t err_box = args[1];

    std::error_code ec;
    if (!fs::exists(p, ec) || !fs::is_regular_file(p, ec)) {
        if (err_box != 0) vm.memory.heap[err_box] = 1ULL; // Not found
        return allocate_solix_chars(vm, "");
    }

    std::ifstream file(p, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        if (err_box != 0) vm.memory.heap[err_box] = 2ULL; // IO error
        return allocate_solix_chars(vm, "");
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    if (err_box != 0) vm.memory.heap[err_box] = 0ULL; // Success
    return allocate_solix_chars(vm, content);
}

static uint64_t file_native_write_all_text(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string p = read_solix_chars(vm, args[0]);
    std::string content = read_solix_chars(vm, args[1]);

    std::ofstream file(p, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return 2ULL; // IO error
    }
    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!file.good()) {
        return 2ULL;
    }
    return 0ULL; // Success
}

static uint64_t file_native_append_all_text(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string p = read_solix_chars(vm, args[0]);
    std::string content = read_solix_chars(vm, args[1]);

    std::ofstream file(p, std::ios::out | std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        return 2ULL; // IO error
    }
    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!file.good()) {
        return 2ULL;
    }
    return 0ULL; // Success
}

static uint64_t file_native_delete(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string p = read_solix_chars(vm, args[0]);
    std::error_code ec;
    if (!fs::exists(p, ec)) {
        return 1ULL; // Not found
    }
    bool removed = fs::remove(p, ec);
    if (ec || !removed) {
        return 2ULL; // IO error
    }
    return 0ULL;
}

static uint64_t file_native_copy(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string src = read_solix_chars(vm, args[0]);
    std::string dest = read_solix_chars(vm, args[1]);
    bool overwrite = args[2] != 0;

    std::error_code ec;
    if (!fs::exists(src, ec)) {
        return 1ULL; // Not found
    }
    fs::copy_options opt = overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none;
    fs::copy_file(src, dest, opt, ec);
    if (ec) {
        return 2ULL; // IO error
    }
    return 0ULL;
}

static uint64_t file_native_move(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string src = read_solix_chars(vm, args[0]);
    std::string dest = read_solix_chars(vm, args[1]);

    std::error_code ec;
    if (!fs::exists(src, ec)) {
        return 1ULL; // Not found
    }
    fs::rename(src, dest, ec);
    if (ec) {
        return 2ULL; // IO error
    }
    return 0ULL;
}

static uint64_t file_native_get_size(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string p = read_solix_chars(vm, args[0]);
    uint64_t out_box = args[1];

    std::error_code ec;
    if (!fs::exists(p, ec) || !fs::is_regular_file(p, ec)) {
        return 1ULL; // Not found
    }
    auto sz = fs::file_size(p, ec);
    if (ec) {
        return 2ULL;
    }
    if (out_box != 0) {
        vm.memory.heap[out_box] = static_cast<uint64_t>(sz);
    }
    return 0ULL;
}

static uint64_t file_native_get_last_modified(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string p = read_solix_chars(vm, args[0]);
    uint64_t out_box = args[1];

    std::error_code ec;
    if (!fs::exists(p, ec)) {
        return 1ULL;
    }
    auto ftime = fs::last_write_time(p, ec);
    if (ec) {
        return 2ULL;
    }
    // Convert file_time_type to epoch milliseconds
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
    );
    int64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(sctp.time_since_epoch()).count();
    if (out_box != 0) {
        vm.memory.heap[out_box] = static_cast<uint64_t>(ms);
    }
    return 0ULL;
}

// --- Directory Native Functions ---

static uint64_t directory_native_exists(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string p = read_solix_chars(vm, args[0]);
    std::error_code ec;
    return fs::is_directory(p, ec) ? 1ULL : 0ULL;
}

static uint64_t directory_native_create(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string p = read_solix_chars(vm, args[0]);
    std::error_code ec;
    fs::create_directories(p, ec);
    if (ec) {
        return 2ULL; // IO error
    }
    return 0ULL;
}

static uint64_t directory_native_delete(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string p = read_solix_chars(vm, args[0]);
    bool recursive = args[1] != 0;

    std::error_code ec;
    if (!fs::exists(p, ec) || !fs::is_directory(p, ec)) {
        return 1ULL; // Not found
    }
    if (recursive) {
        fs::remove_all(p, ec);
    } else {
        fs::remove(p, ec);
    }
    if (ec) {
        return 2ULL; // IO error / Directory not empty
    }
    return 0ULL;
}

static uint64_t directory_native_list_entries(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string p = read_solix_chars(vm, args[0]);
    int32_t mode = static_cast<int32_t>(args[1]); // 0=all, 1=files, 2=dirs
    uint64_t err_box = args[2];

    std::error_code ec;
    if (!fs::exists(p, ec) || !fs::is_directory(p, ec)) {
        if (err_box != 0) vm.memory.heap[err_box] = 1ULL;
        return allocate_solix_chars(vm, "");
    }

    std::string result;
    bool first = true;
    for (const auto &entry : fs::directory_iterator(p, ec)) {
        bool include = false;
        if (mode == 0) {
            include = true;
        } else if (mode == 1 && entry.is_regular_file()) {
            include = true;
        } else if (mode == 2 && entry.is_directory()) {
            include = true;
        }

        if (include) {
            if (!first) {
                result.push_back('\n');
            }
            result.append(entry.path().filename().string());
            first = false;
        }
    }

    if (ec) {
        if (err_box != 0) vm.memory.heap[err_box] = 2ULL;
        return allocate_solix_chars(vm, "");
    }

    if (err_box != 0) vm.memory.heap[err_box] = 0ULL;
    return allocate_solix_chars(vm, result);
}

static uint64_t directory_native_get_current(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)args; (void)argc;
    std::error_code ec;
    fs::path cur = fs::current_path(ec);
    return allocate_solix_chars(vm, cur.generic_string());
}

static uint64_t directory_native_set_current(solix::RuntimeContext &vm, uint64_t self, uint64_t *args, size_t argc) {
    (void)self; (void)argc;
    std::string p = read_solix_chars(vm, args[0]);
    std::error_code ec;
    fs::current_path(p, ec);
    if (ec) {
        return 2ULL;
    }
    return 0ULL;
}

} // namespace

void register_io_fs_natives(solix::NativeRegistry &registry) {
    auto reg = [&](const std::string &cls, const std::string &name, solix::NativeFunctionPtr func) {
        registry.register_function(cls + "_" + name, func);
        registry.register_function("solix_io_filesystem_" + cls + "_" + name, func);
        registry.register_function("solix.io.filesystem." + cls + "." + name, func);
    };

    // Path
    reg("Path", "native_combine", path_native_combine);
    reg("Path", "native_get_directory_name", path_native_get_directory_name);
    reg("Path", "native_get_file_name", path_native_get_file_name);
    reg("Path", "native_get_extension", path_native_get_extension);
    reg("Path", "native_get_file_name_without_extension", path_native_get_file_name_without_extension);
    reg("Path", "native_is_absolute", path_native_is_absolute);
    reg("Path", "native_get_temp_path", path_native_get_temp_path);
    reg("Path", "native_normalize", path_native_normalize);
    reg("Path", "native_get_separator", path_native_get_separator);

    // File
    reg("File", "native_exists", file_native_exists);
    reg("File", "native_read_all_text", file_native_read_all_text);
    reg("File", "native_write_all_text", file_native_write_all_text);
    reg("File", "native_append_all_text", file_native_append_all_text);
    reg("File", "native_delete", file_native_delete);
    reg("File", "native_copy", file_native_copy);
    reg("File", "native_move", file_native_move);
    reg("File", "native_get_size", file_native_get_size);
    reg("File", "native_get_last_modified", file_native_get_last_modified);

    // Directory
    reg("Directory", "native_exists", directory_native_exists);
    reg("Directory", "native_create", directory_native_create);
    reg("Directory", "native_delete", directory_native_delete);
    reg("Directory", "native_list_entries", directory_native_list_entries);
    reg("Directory", "native_get_current", directory_native_get_current);
    reg("Directory", "native_set_current", directory_native_set_current);
}
