#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("DoWhileStatement - Control Flow", "[control_flow][do_while]") {
    SECTION("Case 3.1: Guaranteed Initial Pass with False Condition") {
        std::string code = R"(
static int32 main() {
    int32 ran = 0;
    do {
        ran++;
    } while (false);
    return ran;
}
)";
        CHECK(run_source(code) == 1);
    }

    SECTION("Case 4.1: Accessing Body Variable in Condition") {
        std::string code = R"(
void test() {
    do {
        int32 inner = 10;
    } while (inner > 0);
}
)";
        assert_compile_error(code, "Undefined identifier: inner");
    }
}
