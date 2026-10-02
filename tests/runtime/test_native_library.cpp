#include "test_helper.hpp"
#include "solix/native_registry.hpp"
#include "solix/shared_library.hpp"
#include <catch2/catch_all.hpp>
#include <filesystem>
#include <fstream>

using namespace solix::test;

#ifndef SOLIX_TEST_PLUGIN_PATH
#define SOLIX_TEST_PLUGIN_PATH ""
#endif

#ifndef SOLIX_TEST_PLUGIN_B_PATH
#define SOLIX_TEST_PLUGIN_B_PATH ""
#endif

static int32_t run_with_libraries(const std::string &code, const std::vector<std::filesystem::path> &libs) {
    auto res = compile_source(code);
    if (!res.success) {
        std::string err = "Compilation failed: " + res.failure_message + "\nReports:\n";
        for (const auto &r : res.reports) {
            err += "  [" + r.code + "] " + r.message + "\n";
        }
        throw std::runtime_error(err);
    }
    solix::RuntimeOptions opts;
    opts.bytecode_source = res.bytecode;
    opts.native_libraries = libs;
    return solix::run(opts);
}

TEST_CASE("NativeInteroperability - Dynamic Shared Library Loader", "[runtime][native]") {

    // Ensure clean state
    solix::NativeRegistry::global().clear();

    SECTION("Case 49.1: Static Native Function Call") {
        const std::string code = R"(
            public class NativeMath {
                public static native int32 add(int32 a, int32 b);
            }

            public class Main {
                public static int32 main() {
                    return NativeMath.add(15, 27) == 42 ? 0 : 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_with_libraries(code, {SOLIX_TEST_PLUGIN_PATH}) == 0);
    }

    SECTION("Case 49.2: Instance Native Function Call with Object Context") {
        const std::string code = R"(
            public class Counter {
                public int32 value;
                public native int32 increment();
            }

            public class Main {
                public static int32 main() {
                    Counter c = new Counter();
                    c.value = 10;
                    int32 res = c.increment();
                    return (res == 11 && c.value == 11) ? 0 : 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_with_libraries(code, {SOLIX_TEST_PLUGIN_PATH}) == 0);
    }

    SECTION("Case 49.3: Batch Registration Hook via solix_register_natives") {
        const std::string code = R"(
            public class BatchPlugin {
                public static native int32 multiply(int32 a, int32 b);
                public static native int32 subtract(int32 a, int32 b);
            }

            public class Main {
                public static int32 main() {
                    int32 m = BatchPlugin.multiply(6, 7);
                    int32 s = BatchPlugin.subtract(50, 8);
                    return (m == 42 && s == 42) ? 0 : 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_with_libraries(code, {SOLIX_TEST_PLUGIN_PATH}) == 0);
    }

    SECTION("Case 49.4: Direct Dynamic Symbol Resolution Fallback") {
        const std::string code = R"(
            public class DynamicLib {
                public static native int32 direct_export(int32 x);
            }

            public class Main {
                public static int32 main() {
                    return DynamicLib.direct_export(99) == 100 ? 0 : 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_with_libraries(code, {SOLIX_TEST_PLUGIN_PATH}) == 0);
    }

    SECTION("Case 49.5: Multiple Shared Libraries Loaded Concurrently") {
        const std::string code = R"(
            public class LibA {
                public static native int32 funcA();
            }

            public class LibB {
                public static native int32 funcB();
            }

            public class Main {
                public static int32 main() {
                    return (LibA.funcA() + LibB.funcB()) == 100 ? 0 : 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_with_libraries(code, {SOLIX_TEST_PLUGIN_PATH, SOLIX_TEST_PLUGIN_B_PATH}) == 0);
    }

    SECTION("Case 49.6: Missing / Non-Existent Shared Library Path") {
        const std::string code = R"(
            public class Dummy {
                public static native int32 dummy();
            }

            public class Main {
                public static int32 main() {
                    return 0;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE_THROWS_AS(
            run_with_libraries(code, {"non_existent_plugin_12345.dll"}),
            solix::SharedLibraryException
        );
    }

    SECTION("Case 49.7: Corrupted or Non-Binary File Loaded as Shared Library") {
        std::filesystem::path dummy_corrupt = std::filesystem::temp_directory_path() / "corrupt_test_file.txt";
        {
            std::ofstream out(dummy_corrupt);
            out << "This is not a valid DLL or shared library binary content!";
        }

        const std::string code = R"(
            public class Main {
                public static int32 main() {
                    return 0;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE_THROWS_AS(
            run_with_libraries(code, {dummy_corrupt}),
            solix::SharedLibraryException
        );

        std::filesystem::remove(dummy_corrupt);
    }

    SECTION("Case 49.8: Unresolved Native Method Symbol at Invocation") {
        const std::string code = R"(
            public class MissingNative {
                public static native int32 non_existent_function();
            }

            public class Main {
                public static int32 main() {
                    return MissingNative.non_existent_function();
                }
            }
        )";
        assert_compile_success(code);
        // Loading without registering the function should throw runtime_error on call
        REQUIRE_THROWS_WITH(
            run_with_libraries(code, {}),
            Catch::Matchers::ContainsSubstring("Call to unknown native function")
        );
    }
}
