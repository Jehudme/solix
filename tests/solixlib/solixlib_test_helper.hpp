#pragma once

#include "test_helper.hpp"
#include "solix/native_registry.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <string>
#include <iostream>

namespace solix::test {

inline std::filesystem::path get_solixlib_root() {
    auto p = std::filesystem::path(SOLIX_PROJECT_ROOT) / "libraries" / "solixlib";
    if (std::filesystem::exists(p)) return p;
    p = std::filesystem::path(SOLIX_PROJECT_ROOT) / "libs" / "solixlib";
    if (std::filesystem::exists(p)) return p;
    return std::filesystem::path(SOLIX_PROJECT_ROOT) / "solixlib";
}

/**
 * @brief Returns the path to the compiled solixlib_native shared library.
 */
inline std::filesystem::path get_solixlib_native_path() {
    return get_solixlib_root() / "lib" /
#ifdef _WIN32
        "solixlib_native.dll";
#elif __APPLE__
        "libsolixlib_native.dylib";
#else
        "libsolixlib_native.so";
#endif
}

/**
 * @brief Ensures solixlib_native shared library is loaded into the NativeRegistry.
 */
inline void init_solixlib_natives() {
    auto path = get_solixlib_native_path();
    if (std::filesystem::exists(path)) {
        solix::NativeRegistry::global().load_library(path);
    }
}

/**
 * @brief Reads all .slx standard library source files from solixlib/src
 * to make them available in test compilation units.
 */
inline std::unordered_map<std::string, std::string> load_solixlib_sources() {
    std::unordered_map<std::string, std::string> sources;
    std::filesystem::path lib_dir = get_solixlib_root() / "src";
    if (std::filesystem::exists(lib_dir)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(lib_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".slx") {
                std::ifstream f(entry.path());
                if (f.is_open()) {
                    std::stringstream ss;
                    ss << f.rdbuf();
                    sources[entry.path().lexically_relative(lib_dir).generic_string()] = ss.str();
                }
            }
        }
    }
    return sources;
}

/**
 * @brief Executes sources with solixlib_native initialized.
 */
inline int32_t run_solixlib_sources(const std::unordered_map<std::string, std::string>& sources) {
    init_solixlib_natives();
    auto res = compile_sources(sources);
    if (!res.success) {
        std::string err = "Compilation failed: " + res.failure_message + "\nReports:\n";
        for (const auto& r : res.reports) {
            err += "  [" + r.code + "] " + r.message + "\n";
        }
        throw std::runtime_error(err);
    }
    solix::RuntimeOptions opts;
    opts.bytecode_source = res.bytecode;
    opts.native_libraries = {get_solixlib_native_path()};
    return solix::run(opts);
}

/**
 * @brief RAII stream redirection guard for cin, cout, cerr.
 */
struct StreamRedirectGuard {
    std::streambuf* orig_cin = nullptr;
    std::streambuf* orig_cout = nullptr;
    std::streambuf* orig_cerr = nullptr;

#ifdef _WIN32
    void* dll_orig_cin = nullptr;
    void* dll_orig_cout = nullptr;
    void* dll_orig_cerr = nullptr;
#endif

    std::stringstream cin_stream;
    std::stringstream cout_stream;
    std::stringstream cerr_stream;

    StreamRedirectGuard(const std::string& input = "") {
        init_solixlib_natives();

#ifdef _WIN32
        using SetBufFn = void* (*)(void*);
        auto set_cin = reinterpret_cast<SetBufFn>(solix::NativeRegistry::global().find_raw_symbol("solix_set_cin_rdbuf"));
        auto set_cout = reinterpret_cast<SetBufFn>(solix::NativeRegistry::global().find_raw_symbol("solix_set_cout_rdbuf"));
        auto set_cerr = reinterpret_cast<SetBufFn>(solix::NativeRegistry::global().find_raw_symbol("solix_set_cerr_rdbuf"));

        if (!input.empty()) {
            cin_stream.str(input);
            if (set_cin) dll_orig_cin = set_cin(cin_stream.rdbuf());
            orig_cin = std::cin.rdbuf(cin_stream.rdbuf());
        }
        if (set_cout) dll_orig_cout = set_cout(cout_stream.rdbuf());
        orig_cout = std::cout.rdbuf(cout_stream.rdbuf());

        if (set_cerr) dll_orig_cerr = set_cerr(cerr_stream.rdbuf());
        orig_cerr = std::cerr.rdbuf(cerr_stream.rdbuf());
#else
        if (!input.empty()) {
            cin_stream.str(input);
            orig_cin = std::cin.rdbuf(cin_stream.rdbuf());
        }
        orig_cout = std::cout.rdbuf(cout_stream.rdbuf());
        orig_cerr = std::cerr.rdbuf(cerr_stream.rdbuf());
#endif
    }

    ~StreamRedirectGuard() {
#ifdef _WIN32
        using SetBufFn = void* (*)(void*);
        auto set_cin = reinterpret_cast<SetBufFn>(solix::NativeRegistry::global().find_raw_symbol("solix_set_cin_rdbuf"));
        auto set_cout = reinterpret_cast<SetBufFn>(solix::NativeRegistry::global().find_raw_symbol("solix_set_cout_rdbuf"));
        auto set_cerr = reinterpret_cast<SetBufFn>(solix::NativeRegistry::global().find_raw_symbol("solix_set_cerr_rdbuf"));

        if (dll_orig_cin && set_cin) set_cin(dll_orig_cin);
        if (orig_cin) std::cin.rdbuf(orig_cin);

        if (dll_orig_cout && set_cout) set_cout(dll_orig_cout);
        if (orig_cout) std::cout.rdbuf(orig_cout);

        if (dll_orig_cerr && set_cerr) set_cerr(dll_orig_cerr);
        if (orig_cerr) std::cerr.rdbuf(orig_cerr);
#else
        if (orig_cin) std::cin.rdbuf(orig_cin);
        if (orig_cout) std::cout.rdbuf(orig_cout);
        if (orig_cerr) std::cerr.rdbuf(orig_cerr);
#endif
    }
};

} // namespace solix::test
