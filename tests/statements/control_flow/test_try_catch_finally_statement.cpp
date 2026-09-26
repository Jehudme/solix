#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("TryCatchFinallyStatement - Control Flow", "[control_flow][try_catch_finally]") {
    SECTION("Case 3.1: Specific Catch Hierarchy") {
        std::string code = R"(
class Exception {}
class CustomError extends Exception {}

static int32 main() {
    int32 result = 0;
    try {
        throw new CustomError();
    } catch (CustomError c) {
        result = 1;
    } catch (Exception e) {
        result = 2;
    }
    return result;
}
)";
        CHECK(run_source(code) == 1);
    }

    SECTION("Case 4.1: Unreachable Catch Clause") {
        std::string code = R"(
class Exception {}
class SubErr extends Exception {}

void test() {
    try {
        int32 x = 0;
    } catch (Exception e) {
    } catch (SubErr s) {
    }
}
)";
        assert_compile_error(code, "Unreachable catch clause: 'SubErr' is already handled by preceding catch for 'Exception'");
    }
}
