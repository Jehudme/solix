#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("SwitchStatement - Control Flow", "[control_flow][switch]") {
    SECTION("Case 3.1: Switch with Explicit Break") {
        std::string code = R"(
int32 main() {
    int32 value = 2;
    int32 res = 0;
    switch (value) {
        case 1: res = 10; break;
        case 2: res = 20; break;
        default: res = 30; break;
    }
    return res;
}
)";
        CHECK(run_source(code) == 20);
    }

    SECTION("Case 3.2: Switch on Strongly Typed Enum") {
        std::string code = R"(
enum Status { PENDING, APPROVED, REJECTED }

int32 evaluate(Status s) {
    switch (s) {
        case Status.PENDING: return 1;
        case Status.APPROVED: return 2;
        case Status.REJECTED: return 3;
        default: return 0;
    }
}

int32 main() {
    return evaluate(Status.APPROVED) == 2 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Duplicate Case Constant") {
        std::string code = R"(
void test(int32 x) {
    switch (x) {
        case 1: break;
        case 1: break;
    }
}
)";
        assert_compile_error(code, "Duplicate case value '1' in switch statement");
    }

    SECTION("Case 4.2: Variable Expression in Case Label") {
        std::string code = R"(
void test(int32 x, int32 dynamicVal) {
    switch (x) {
        case dynamicVal: break;
    }
}
)";
        assert_compile_error(code, "Case label must be a constant literal");
    }

    SECTION("Case 4.4: Switch Statement Placed Directly in Class Body") {
        std::string code = R"(
class BadClass {
    switch (1) { case 1: break; }
}
)";
        assert_compile_error(code, "Statements are not allowed directly in class body");
    }
}
