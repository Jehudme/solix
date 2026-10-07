#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("ContinueStatement - Control Flow", "[control_flow][continue]") {
    SECTION("Case 3.1: Continue Advances Loop Variable") {
        std::string code = R"(
int32 main() {
    int32 hits = 0;
    for (int32 i = 0; i < 6; i++) {
        if (i % 2 == 0) continue;
        hits++;
    }
    return hits;
}
)";
        CHECK(run_source(code) == 3);
    }

    SECTION("Case 3.2: Continue in While Loop") {
        std::string code = R"(
int32 main() {
    int32 i = 0;
    int32 count = 0;
    while (i < 10) {
        i++;
        if (i % 2 == 0) {
            continue;
        }
        count++;
    }
    return count == 5 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Continue Outside Loop") {
        std::string code = R"(
void test() {
    continue;
}
)";
        assert_compile_error(code, "'continue' statement not allowed outside of loop");
    }

    SECTION("Case 4.2: Continue Inside Switch Not in Loop") {
        std::string code = R"(
void test(int32 x) {
    switch (x) {
        case 1:
            continue;
    }
}
)";
        assert_compile_error(code, "'continue' statement not allowed outside of loop");
    }

    SECTION("Case 4.3: Continue Statement Placed Directly in Class Body") {
        std::string code = R"(
class BadClass {
    continue;
}
)";
        assert_compile_error(code, "Statements are not allowed directly in class body");
    }

    SECTION("Case 4.4: Continue Inside If Not Enclosed in Loop") {
        std::string code = R"(
void test(bool flag) {
    if (flag) {
        continue;
    }
}
)";
        assert_compile_error(code, "'continue' statement not allowed outside of loop");
    }
}
