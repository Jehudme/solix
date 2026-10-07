#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("BreakStatement - Control Flow", "[control_flow][break]") {
    SECTION("Case 3.1: Breaking Out of Deep Nested Blocks") {
        std::string code = R"(
class Res {
    int32 id;
    Res(int32 id) { this.id = id; }
}
int32 main() {
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

    SECTION("Case 3.2: Break in For Loop Preserves State") {
        std::string code = R"(
int32 main() {
    int32 sum = 0;
    for (int32 i = 0; i < 10; i++) {
        if (i == 5) {
            break;
        }
        sum += i;
    }
    return sum == 10 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.3: Break Inside Switch Statement") {
        std::string code = R"(
int32 main() {
    int32 x = 2;
    int32 res = 0;
    switch (x) {
        case 1: res = 10; break;
        case 2: res = 20; break;
        default: res = 30; break;
    }
    return res == 20 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
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

    SECTION("Case 4.2: Break at Function Top Level") {
        std::string code = R"(
void test() {
    break;
}
)";
        assert_compile_error(code, "'break' statement not allowed outside of loop or switch");
    }

    SECTION("Case 4.3: Break Statement Placed Directly in Class Body") {
        std::string code = R"(
class BadClass {
    break;
}
)";
        assert_compile_error(code, "Statements are not allowed directly in class body");
    }

    SECTION("Case 4.4: Break Inside If Not Enclosed in Loop or Switch") {
        std::string code = R"(
void test(bool flag) {
    if (flag) {
        break;
    }
}
)";
        assert_compile_error(code, "'break' statement not allowed outside of loop or switch");
    }
}
