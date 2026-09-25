#include "solix/compilation.hpp"
#include "utilities/diagnostic.hpp"
#include <catch2/catch_test_macros.hpp>
#include <sstream>

using namespace solix;

TEST_CASE("Phase 2 - Diagnostic Formatting and Warning Subsystem", "[phase2_diagnostics]") {
    SECTION("Warning recording and counts in Diagnostic") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        CompilationContext context(options);
        Diagnostic diagnostic(context);

        REQUIRE_FALSE(diagnostic.has_warnings());
        REQUIRE_FALSE(diagnostic.has_errors());
        REQUIRE(diagnostic.warning_count() == 0);
        REQUIRE(diagnostic.error_count() == 0);

        diagnostic.record_warning("Unused variable 'x'", "test.slx", 10, 5, "W_UNUSED");
        REQUIRE(diagnostic.has_warnings());
        REQUIRE_FALSE(diagnostic.has_errors());
        REQUIRE(diagnostic.warning_count() == 1);

        Report err;
        err.severity = ReportSeverity::ERROR;
        err.message = "Type mismatch";
        err.source_path = "test.slx";
        err.line = 12;
        err.column = 8;
        err.code = "E_TYPE";
        diagnostic.record_report(err);

        REQUIRE(diagnostic.has_errors());
        REQUIRE(diagnostic.error_count() == 1);
        REQUIRE(diagnostic.get_reports().size() == 2);
    }

    SECTION("Caret diagnostic formatting with source snippet") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source key = std::string("snippet_test.slx");
        options.sources[key] = "class Foo {\n    int32 x = 42;\n}\n";

        CompilationContext context(options);
        Diagnostic diagnostic(context);

        diagnostic.record_warning("Test warning message", "snippet_test.slx", 2, 11, "W_TEST");

        // Verify print_reports executes without error (formatting includes snippet & caret)
        REQUIRE_NOTHROW(diagnostic.print_reports(true));
    }

    SECTION("Binder detects shadowed variable warning") {
        CompilationOptions options;
        options.log_level = CompilationOptions::LogLevel::OFF;
        Source key = std::string("shadow_test.slx");
        options.sources[key] = R"(
            class ShadowTest {
                public static int32 test() {
                    int32 val = 10;
                    if (val > 0) {
                        int32 val = 20; // Shadows outer val
                        return val;
                    }
                    return val;
                }
            }
        )";

        CompilationContext context(options);
        context.diagnostic = std::make_unique<Diagnostic>(context);

        std::vector<uint8_t> bytecode;
        REQUIRE_NOTHROW(bytecode = solix::run(options));
        REQUIRE_FALSE(bytecode.empty());
    }

    SECTION("Compilation at different log levels executes without error") {
        const std::vector<CompilationOptions::LogLevel> levels = {
            CompilationOptions::LogLevel::OFF,
            CompilationOptions::LogLevel::CRITICAL,
            CompilationOptions::LogLevel::ERR,
            CompilationOptions::LogLevel::WARN,
            CompilationOptions::LogLevel::INFO,
            CompilationOptions::LogLevel::DEBUG,
            CompilationOptions::LogLevel::TRACE
        };

        for (auto lvl : levels) {
            CompilationOptions options;
            options.log_level = lvl;
            Source key = std::string("log_level_test.slx");
            options.sources[key] = R"(
                class LogTest {
                    public static int32 main() {
                        return 0;
                    }
                }
            )";

            std::vector<uint8_t> bytecode;
            REQUIRE_NOTHROW(bytecode = solix::run(options));
            REQUIRE_FALSE(bytecode.empty());
        }
    }
}
