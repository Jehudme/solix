#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("BreakStatement - Control Flow", "[control_flow][break]") {
    SECTION("Case 3.1: Breaking Out of Deep Nested Blocks") {
        std::string code = R"(
class Res {
    int32 id;
    Res(int32 id) { this.id = id; }
}
static int32 main() {
    int32 count = 0;
    while (true) {
        Res s1 = new Res(1);
        {
            Res s2 = new Res(2);
            count = 42;
            break;
        }
    }
    return count;
}
)";
        CHECK(run_source(code) == 42);
    }

    SECTION("Case 4.1: Break Outside Loop or Switch") {
        std::string code = R"(
void test() {
    int32 x = 10;
    break;
}
)";
        assert_compile_error(code, "'break' statement not allowed outside of loop or switch");
    }
}
