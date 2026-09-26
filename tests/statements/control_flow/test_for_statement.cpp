#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("ForStatement - Control Flow", "[control_flow][for]") {
    SECTION("Case 3.1: Standard For Loop with Continue") {
        std::string code = R"(
static int32 main() {
    int32 evens = 0;
    for (int32 i = 0; i < 10; i++) {
        if (i % 2 != 0) continue;
        evens++;
    }
    return evens;
}
)";
        CHECK(run_source(code) == 5);
    }

    SECTION("Case 4.1: Induction Variable Leakage") {
        std::string code = R"(
void test() {
    for (int32 i = 0; i < 5; i++) {}
    int32 leak = i;
}
)";
        assert_compile_error(code, "Undefined identifier: i");
    }
}
