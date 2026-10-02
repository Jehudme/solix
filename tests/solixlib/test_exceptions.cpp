#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.exceptions", "[solixlib][exceptions]") {
    SECTION("Case 1.1: Base Exception instantiation and message retrieval") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.exceptions.*;

            class Main {
                public static int32 main() {
                    Exception ex = new Exception("Root error occurred");
                    if (ex.get_message() != "Root error occurred") return 1;
                    if (ex.to_string() != "Root error occurred") return 2;
                    if (ex.get_cause() != null) return 3;
                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 1.2: Catching IllegalArgumentException via RuntimeException and Exception polymorphism") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.exceptions.*;

            class Main {
                public static int32 main() {
                    int32 caught_marker = 0;
                    try {
                        throw new IllegalArgumentException("Invalid argument provided");
                    } catch (RuntimeException re) {
                        caught_marker = 42;
                    } catch (Exception ex) {
                        caught_marker = 99;
                    }
                    return caught_marker == 42 ? 0 : 1;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 1.3: IndexOutOfBoundsException with bounds metadata") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.exceptions.*;

            class Main {
                public static int32 main() {
                    try {
                        throw new IndexOutOfBoundsException("Array index out of range", 15, 0, 10);
                    } catch (IndexOutOfBoundsException ex) {
                        if (ex.get_index() != 15) return 1;
                        if (ex.get_lower_bound() != 0) return 2;
                        if (ex.get_upper_bound() != 10) return 3;
                        if (ex.get_message() != "Array index out of range") return 4;
                        return 0;
                    }
                    return 5;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 1.4: DivideByZeroException caught as ArithmeticException") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.exceptions.*;

            class Main {
                public static int32 main() {
                    int32 handled = 0;
                    try {
                        throw new DivideByZeroException("Division by zero attempted");
                    } catch (ArithmeticException ae) {
                        handled = 1;
                    } catch (Exception ex) {
                        handled = 2;
                    }
                    return handled == 1 ? 0 : 1;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 1.5: NullReferenceException instantiation and throwing") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.exceptions.*;

            class Main {
                public static int32 main() {
                    int32 caught = 0;
                    try {
                        throw new NullReferenceException("Object reference not set");
                    } catch (NullReferenceException nre) {
                        caught = 1;
                    }
                    return caught == 1 ? 0 : 1;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 1.6: InvalidOperationException on invalid state transitions") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.exceptions.*;

            class Main {
                public static int32 main() {
                    int32 caught = 0;
                    try {
                        throw new InvalidOperationException("Collection was modified during iteration");
                    } catch (InvalidOperationException ioe) {
                        caught = 1;
                    }
                    return caught == 1 ? 0 : 1;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 1.7: FormatException on failed string conversion") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.exceptions.*;

            class Main {
                public static int32 main() {
                    int32 caught = 0;
                    try {
                        throw new FormatException("Input string was not in a correct format");
                    } catch (FormatException fe) {
                        caught = 1;
                    }
                    return caught == 1 ? 0 : 1;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 1.8: FileNotFoundException as specialization of IOException") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.exceptions.*;

            class Main {
                public static int32 main() {
                    try {
                        throw new FileNotFoundException("/etc/missing.conf", "File not found on system");
                    } catch (IOException ioe) {
                        if (ioe.get_message() != "File not found on system") return 1;
                        return 0;
                    }
                    return 2;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 1.9: Exception Chaining via get_cause()") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.exceptions.*;

            class Main {
                public static int32 main() {
                    try {
                        IOException cause = new IOException("Disk failure");
                        throw new RuntimeException("High-level operation failed", cause);
                    } catch (RuntimeException re) {
                        Exception cause = re.get_cause();
                        if (cause == null) return 1;
                        if (cause.get_message() != "Disk failure") return 2;
                        return 0;
                    }
                    return 3;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 1.10: ArgumentOutOfRangeException & ArgumentNullException parameter metadata") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.exceptions.*;

            class Main {
                public static int32 main() {
                    try {
                        throw new ArgumentNullException("target", "Target reference cannot be null");
                    } catch (ArgumentNullException ane) {
                        if (ane.get_param_name() != "target") return 1;
                    }

                    try {
                        throw new ArgumentOutOfRangeException("capacity", "Capacity must be positive");
                    } catch (ArgumentOutOfRangeException aore) {
                        if (aore.get_param_name() != "capacity") return 2;
                    }

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 1.11: SocketException with error code") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.exceptions.*;

            class Main {
                public static int32 main() {
                    try {
                        throw new SocketException("Connection refused", 10061);
                    } catch (SocketException se) {
                        if (se.get_error_code() != 10061) return 1;
                        if (se.get_message() != "Connection refused") return 2;
                        return 0;
                    }
                    return 3;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 1.12: AssertionError instantiation and polymorphism") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.exceptions.*;

            class Main {
                public static int32 main() {
                    try {
                        throw new AssertionError("Assertion condition failed: x > 0");
                    } catch (Exception ex) {
                        if (ex.get_message() != "Assertion condition failed: x > 0") return 1;
                        return 0;
                    }
                    return 2;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 1.13: Nested Try-Catch-Finally Unwinding Execution") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.exceptions.*;

            class Main {
                public static int32 state = 0;

                public static void faulty_work() {
                    try {
                        state += 1;
                        throw new InvalidOperationException("Failed inside nested try");
                    } finally {
                        state += 10;
                    }
                }

                public static int32 main() {
                    try {
                        faulty_work();
                    } catch (InvalidOperationException ioe) {
                        state += 100;
                    } finally {
                        state += 1000;
                    }

                    // Expected state: 1 (try) + 10 (inner finally) + 100 (catch) + 1000 (outer finally) = 1111
                    return state == 1111 ? 0 : 1;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }
}
