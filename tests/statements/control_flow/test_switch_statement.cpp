#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("SwitchStatement - Control Flow", "[control_flow][switch]") {
    SECTION("Case 3.1: Switch with Explicit Break") {
        std::string code = R"(
static int32 main() {
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
}
