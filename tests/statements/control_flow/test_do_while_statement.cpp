#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("DoWhileStatement - Control Flow", "[control_flow][do_while]") {
    SECTION("Case 3.1: Guaranteed Initial Pass with False Condition") {
        std::string code = R"(
int32 main() {
    int32 ran = 0;
    do {
        ran++;
    } while (false);
    return ran;
}
)";
        CHECK(run_source(code) == 1);
    }

    SECTION("Case 3.2: Multi-Pass Iteration and Condition Evaluation") {
        std::string code = R"(
int32 main() {
    int32 sum = 0;
    int32 i = 1;
    do {
        sum += i;
        i++;
    } while (i <= 5);
    return sum == 15 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
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

    SECTION("Case 4.2: Non-Boolean Condition in Do-While") {
        std::string code = R"(
void test() {
    do {} while (42);
}
)";
        assert_compile_error(code, "Do-while loop condition must be of type 'bool', got 'int32'");
    }

    SECTION("Case 4.3: Do-While Statement Placed Directly in Class Body") {
        std::string code = R"(
class BadClass {
    do {} while (true);
}
)";
        assert_compile_error(code, "Statements are not allowed directly in class body");
    }
}
