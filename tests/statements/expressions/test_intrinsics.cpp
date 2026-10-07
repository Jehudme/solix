#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("Language Intrinsics - assert and exit", "[statements][intrinsics][assert][exit]") {
    SECTION("Case 50.1: Basic Boolean Assertion Passing Silently") {
        std::string code = R"(
class Main {
    public static int32 main() {
        assert true;
        assert(1 + 1 == 2);
        return 0;
    }
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 50.2: Assertion with Custom Textual Message Passing Silently") {
        std::string code = R"(
class Main {
    public static int32 main() {
        assert(10 > 5, "Arithmetic consistency holds");
        return 0;
    }
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 50.3: Failed Assertion Throws AssertionError Runtime Exception") {
        std::string code = R"(
class Main {
    public static int32 main() {
        assert(2 + 2 == 5, "Math verification failed");
        return 0;
    }
}
)";
        CHECK_THROWS_WITH(run_source(code), Catch::Matchers::ContainsSubstring("AssertionError") &&
                                            Catch::Matchers::ContainsSubstring("Math verification failed"));
    }

    SECTION("Case 50.4: Immediate Process Termination via exit(code) Intrinsics") {
        std::string code = R"(
class Main {
    public static int32 main() {
        exit(42);
        return 0;
    }
}
)";
        CHECK(run_source(code) == 42);
    }

    SECTION("Case 50.5: Negative: Assert Condition Non-Boolean Type Mismatch Rejected at Compile Time") {
        std::string code = R"(
class Main {
    public static int32 main() {
        assert 123;
        return 0;
    }
}
)";
        assert_compile_error(code, "Assert condition must be of type 'bool', got 'int32'");
    }

    SECTION("Case 50.6: Negative: Exit Code Non-Integer Type Mismatch Rejected at Compile Time") {
        std::string code = R"(
class Main {
    public static int32 main() {
        exit "fatal";
        return 0;
    }
}
)";
        assert_compile_error(code, "Exit code must be an integer type, got 'char[]'");
    }
}
