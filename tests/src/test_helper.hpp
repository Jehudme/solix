#pragma once

#include <catch2/catch_test_macros.hpp>
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

inline CompilationResult compile_source(const std::string& code, bool include_stdlib = false, const std::string& filename = "test.slx") {
    CompilationResult result;
    solix::CompilationOptions options;
    options.log_level = solix::CompilationOptions::LogLevel::OFF;
    if (include_stdlib) {
        load_stdlib_into_options(options);
    }
    options.sources[filename] = code;

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
        bool found = (res.failure_message.find(expected_substr) != std::string::npos);
        if (!found) {
            for (const auto& r : res.reports) {
                if (r.message.find(expected_substr) != std::string::npos ||
                    r.code.find(expected_substr) != std::string::npos) {
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
            CHECK(all_msgs.find(expected_substr) != std::string::npos);
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
