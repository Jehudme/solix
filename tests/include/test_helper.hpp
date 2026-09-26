#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "solix/compilation.hpp"
#include "solix/runtime.hpp"
#include "processes/lexer.hpp"
#include "processes/parser.hpp"
#include "processes/binder.hpp"
#include "processes/assembler.hpp"
#include "utilities/diagnostic.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#ifndef SOLIX_PROJECT_ROOT
#define SOLIX_PROJECT_ROOT "."
#endif

namespace solix::test {

struct CompilationResult {
    bool success = false;
    std::vector<uint8_t> bytecode;
    std::vector<solix::Report> reports;
    std::string failure_message;
};

inline void load_stdlib_into_options(solix::CompilationOptions& options) {
    std::filesystem::path stdlib_path = std::filesystem::path(SOLIX_PROJECT_ROOT) / "launcher" / "rsc" / "lib";
    if (std::filesystem::exists(stdlib_path)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(stdlib_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".slx") {
                std::ifstream ifs(entry.path());
                if (ifs) {
                    std::string content((std::istreambuf_iterator<char>(ifs)),
                                        (std::istreambuf_iterator<char>()));
                    options.sources[entry.path().string()] = content;
                }
            }
        }
    }
}

inline CompilationResult compile_sources(const std::unordered_map<std::string, std::string>& sources, bool include_stdlib = false) {
    CompilationResult result;
    solix::CompilationOptions options;
    options.log_level = solix::CompilationOptions::LogLevel::OFF;
    if (include_stdlib) {
        load_stdlib_into_options(options);
    }
    for (const auto& [name, content] : sources) {
        options.sources[name] = content;
    }

    solix::CompilationContext context(options);
    context.diagnostic = std::make_unique<solix::Diagnostic>(context);

    try {
        solix::Lexer lexer(context, "Lexer");
        lexer.execute();
        if (context.diagnostic->has_errors()) {
            result.reports = context.diagnostic->get_reports();
            result.failure_message = "Lexical analysis failed with errors";
            return result;
        }

        solix::Parser parser(context, "Parser");
        parser.execute();
        if (context.diagnostic->has_errors()) {
            result.reports = context.diagnostic->get_reports();
            result.failure_message = "Syntax analysis failed with errors";
            return result;
        }

        solix::Binder binder(context, "Binder");
        try {
            binder.execute();
        } catch (const std::exception& e) {
            result.failure_message = std::string("Semantic exception: ") + e.what();
            result.reports = context.diagnostic->get_reports();
            return result;
        }
        if (context.diagnostic->has_errors()) {
            result.reports = context.diagnostic->get_reports();
            result.failure_message = "Semantic analysis failed with errors";
            return result;
        }

        solix::Assembler assembler(context, "Assembler");
        try {
            assembler.execute();
        } catch (const std::exception& e) {
            result.failure_message = std::string("Assembly exception: ") + e.what();
            result.reports = context.diagnostic->get_reports();
            return result;
        }
        if (context.diagnostic->has_errors()) {
            result.reports = context.diagnostic->get_reports();
            result.failure_message = "Assembly failed with errors";
            return result;
        }

        result.success = true;
        result.bytecode = std::move(context.bytecode);
        result.reports = context.diagnostic->get_reports();
    } catch (const std::exception& e) {
        result.success = false;
        result.failure_message = e.what();
        if (context.diagnostic) {
            result.reports = context.diagnostic->get_reports();
        }
    }
    return result;
}

inline CompilationResult compile_source(const std::string& code, bool include_stdlib = false, const std::string& filename = "test.slx") {
    std::unordered_map<std::string, std::string> sources;
    sources[filename] = code;
    return compile_sources(sources, include_stdlib);
}

inline void assert_compile_sources_success(const std::unordered_map<std::string, std::string>& sources, bool include_stdlib = false) {
    auto res = compile_sources(sources, include_stdlib);
    if (!res.success) {
        std::string err = "Expected compilation to succeed, but failed: " + res.failure_message + "\nReports:\n";
        for (const auto& r : res.reports) {
            err += "  [" + r.code + "] " + r.message + "\n";
        }
        FAIL_CHECK(err);
    } else {
        SUCCEED();
    }
}

inline bool contains_ignore_case(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char ch1, char ch2) { return std::tolower(static_cast<unsigned char>(ch1)) == std::tolower(static_cast<unsigned char>(ch2)); }
    );
    return it != haystack.end();
}

inline void assert_compile_sources_error(const std::unordered_map<std::string, std::string>& sources, const std::string& expected_substr = "", bool include_stdlib = false) {
    auto res = compile_sources(sources, include_stdlib);
    if (res.success) {
        FAIL_CHECK("Expected compilation to fail, but it succeeded!");
        return;
    }
    if (!expected_substr.empty()) {
        bool found = contains_ignore_case(res.failure_message, expected_substr);
        if (!found) {
            for (const auto& r : res.reports) {
                if (contains_ignore_case(r.message, expected_substr) ||
                    contains_ignore_case(r.code, expected_substr)) {
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            std::string all_msgs = res.failure_message + "\n";
            for (const auto& r : res.reports) {
                all_msgs += "  [" + r.code + "] " + r.message + "\n";
            }
            INFO("Compilation reports:\n" << all_msgs);
            CHECK(contains_ignore_case(all_msgs, expected_substr));
        } else {
            SUCCEED();
        }
    } else {
        SUCCEED();
    }
}

inline int32_t run_sources(const std::unordered_map<std::string, std::string>& sources, bool include_stdlib = false) {
    auto res = compile_sources(sources, include_stdlib);
    if (!res.success) {
        std::string err = "Compilation failed: " + res.failure_message + "\nReports:\n";
        for (const auto& r : res.reports) {
            err += "  [" + r.code + "] " + r.message + "\n";
        }
        throw std::runtime_error(err);
    }
    solix::RuntimeOptions opts;
    opts.bytecode_source = res.bytecode;
    return solix::run(opts);
}

inline void assert_compile_success(const std::string& code, bool include_stdlib = false, const std::string& filename = "test.slx") {
    auto res = compile_source(code, include_stdlib, filename);
    if (!res.success) {
        std::string err = "Expected compilation to succeed, but failed: " + res.failure_message + "\nReports:\n";
        for (const auto& r : res.reports) {
            err += "  [" + r.code + "] " + r.message + "\n";
        }
        FAIL_CHECK(err);
    } else {
        SUCCEED();
    }
}

inline void assert_compile_error(const std::string& code, const std::string& expected_substr = "", bool include_stdlib = false, const std::string& filename = "test.slx") {
    auto res = compile_source(code, include_stdlib, filename);
    if (res.success) {
        FAIL_CHECK("Expected compilation to fail, but it succeeded!");
        return;
    }
    if (!expected_substr.empty()) {
        bool found = contains_ignore_case(res.failure_message, expected_substr);
        if (!found) {
            for (const auto& r : res.reports) {
                if (contains_ignore_case(r.message, expected_substr) ||
                    contains_ignore_case(r.code, expected_substr)) {
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            std::string all_msgs = res.failure_message + "\n";
            for (const auto& r : res.reports) {
                all_msgs += "  [" + r.code + "] " + r.message + "\n";
            }
            INFO("Compilation reports:\n" << all_msgs);
            CHECK(contains_ignore_case(all_msgs, expected_substr));
        } else {
            SUCCEED();
        }
    } else {
        SUCCEED();
    }
}

inline int32_t run_source(const std::string& code, bool include_stdlib = false, const std::string& filename = "test.slx") {
    auto res = compile_source(code, include_stdlib, filename);
    if (!res.success) {
        std::string err = "Compilation failed: " + res.failure_message + "\nReports:\n";
        for (const auto& r : res.reports) {
            err += "  [" + r.code + "] " + r.message + "\n";
        }
        throw std::runtime_error(err);
    }
    solix::RuntimeOptions opts;
    opts.bytecode_source = res.bytecode;
    return solix::run(opts);
}

inline int32_t run_and_evaluate_int(const std::string& code, bool include_stdlib = false, const std::string& filename = "test.slx") {
    return run_source(code, include_stdlib, filename);
}

} // namespace solix::test
