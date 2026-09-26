#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("WhileStatement - Control Flow", "[control_flow][while]") {
    SECTION("Case 3.1: Standard Counted Loop") {
        std::string code = R"(
static int32 main() {
    int32 total = 0;
    int32 i = 1;
    while (i <= 5) {
        total = total + i;
        i++;
    }
    return total;
}
)";
        CHECK(run_source(code) == 15);
    }

    SECTION("Case 3.2: While Loop with Break and Continue") {
        std::string code = R"(
static int32 main() {
    int32 sum = 0;
    int32 i = 0;
    while (true) {
        i++;
        if (i % 2 == 0) continue;
        if (i > 5) break;
        sum = sum + i;
    }
    return sum;
}
)";
        CHECK(run_source(code) == 9);
    }

    SECTION("Case 4.1: Non-Boolean Loop Condition") {
        std::string code = R"(
void test() {
    int32 x = 5;
    while (x) {
        x--;
    }
}
)";
        assert_compile_error(code, "While condition must be of type 'bool', got 'int32'");
    }
}
