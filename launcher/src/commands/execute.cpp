#include "execute.hpp"
#include "solix/compilation.hpp"
#include "solix/runtime.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

namespace solix::cli {

    // ── Helper: register all standard native functions ────────────────────────
    static void register_natives(RuntimeOptions& opts, const std::string& pkg_prefix) {
        // Engine.print(int/bool/addr) — handles integer and string addresses
        opts.native_functions[pkg_prefix + "Engine.print"] = [](solix::RuntimeContext& ctx, uint64_t, uint64_t*, size_t) {
            uint64_t val = ctx.pop();
            solix::Address addr = static_cast<solix::Address>(val);

            // Heuristic: if addr is > 0 and heap[addr] looks like a string length, treat as string
            if (addr > 0 && addr < ctx.memory.heap.size()) {
                uint64_t maybe_len = ctx.memory.heap[addr];
                bool all_printable = true;
                if (maybe_len > 0 && maybe_len < 65536) {
                    for (uint64_t i = 0; i < maybe_len && i < 256; ++i) {
                        uint64_t ch = ctx.memory.heap[addr + 1 + i];
                        if (ch > 127) { all_printable = false; break; }
                    }
                } else {
                    all_printable = false;
                }
                if (all_printable && maybe_len > 0) {
                    std::string str;
                    str.reserve(static_cast<size_t>(maybe_len));
                    for (uint64_t i = 0; i < maybe_len; ++i)
                        str += static_cast<char>(ctx.memory.heap[addr + 1 + i]);
                    std::cout << str << std::endl;
                    ctx.push(0);
                    return;
                }
            }

            // Fall through: print as integer
            std::cout << static_cast<int64_t>(val) << std::endl;
            ctx.push(0);
        };

        opts.native_functions[pkg_prefix + "Engine.printFloat"] = [](solix::RuntimeContext& ctx, uint64_t, uint64_t*, size_t) {
            uint64_t val = ctx.pop();
            double d = std::bit_cast<double>(val);
            std::cout << d << std::endl;
            ctx.push(0);
        };

        opts.native_functions[pkg_prefix + "Engine.printInt"] = [](solix::RuntimeContext& ctx, uint64_t, uint64_t*, size_t) {
            uint64_t val = ctx.pop();
            std::cout << static_cast<int64_t>(val) << std::endl;
            ctx.push(0);
        };
    }

    // ── Derive package prefix from source code ────────────────────────────────
    static std::string extract_package_prefix(const std::string& source) {
        // Look for: package com.foo.bar;
        auto pos = source.find("package ");
        if (pos == std::string::npos) return "";
        auto start = pos + 8;
        auto end = source.find(';', start);
        if (end == std::string::npos) return "";
        std::string pkg = source.substr(start, end - start);
        // trim whitespace
        while (!pkg.empty() && std::isspace(pkg.front())) pkg.erase(pkg.begin());
        while (!pkg.empty() && std::isspace(pkg.back())) pkg.pop_back();
        if (!pkg.empty()) pkg += ".";
        return pkg;
    }

    void setup_execute_command(CLI::App& app) {
        auto* execute_cmd = app.add_subcommand("run", "Compile and execute a Solix source file (.slx) or run compiled bytecode (.slxb)");

        auto opts         = std::make_shared<RuntimeOptions>();
        auto input_file   = std::make_shared<std::string>();

        execute_cmd->add_option("file", *input_file, "Solix source (.slx) or compiled bytecode (.slxb)")->required()->check(CLI::ExistingFile);
        execute_cmd->add_option("-s,--stack", opts->stack_capacity, "Stack capacity in words (default: 1048576)");
        execute_cmd->add_option("-p,--heap",  opts->heap_capacity,  "Heap capacity in words (default: 16777216)");
        execute_cmd->add_option("args",       opts->program_args,   "Arguments passed to the Solix program");

        execute_cmd->callback([opts, input_file]() {
            std::filesystem::path path(*input_file);
            std::string ext = path.extension().string();

            if (ext == ".slx") {
                // ── Compile source → bytecode in memory, then execute ──────────
                std::ifstream t(path);
                if (!t.is_open()) {
                    std::cerr << "Error: Could not open " << path << std::endl;
                    exit(1);
                }
                std::ostringstream buf;
                buf << t.rdbuf();
                std::string source = buf.str();

                // Extract package prefix for native function names
                std::string pkg = extract_package_prefix(source);
                register_natives(*opts, pkg);

                CompilationOptions copts;
                copts.log_level = CompilationOptions::LogLevel::OFF;
                copts.sources[path] = source;

                try {
                    Bytecode bytecode = solix::run(copts);
                    opts->bytecode_source = bytecode;
                    solix::run(*opts);
                } catch (const std::exception& e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                    exit(1);
                }

            } else {
                // ── Load pre-compiled .slxb bytecode ──────────────────────────
                // Register natives with empty prefix (package unknown)
                register_natives(*opts, "");
                // Also register the well-known test package prefix
                register_natives(*opts, "com.solix.advanced.test.");

                opts->bytecode_source = path;

                try {
                    solix::run(*opts);
                } catch (const std::exception& e) {
                    std::cerr << "Execution failed: " << e.what() << std::endl;
                    exit(1);
                }
            }
        });
    }
}
