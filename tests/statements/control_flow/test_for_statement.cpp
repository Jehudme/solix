#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("ForStatement - Control Flow", "[control_flow][for]") {
    SECTION("Case 3.1: Standard For Loop with Continue") {
        std::string code = R"(
int32 main() {
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

    SECTION("Case 3.2: Empty Header Clauses for Infinite Loop with Break") {
        std::string code = R"(
int32 main() {
    int32 count = 0;
    for (;;) {
        count++;
        if (count == 5) {
            break;
        }
    }
    return count == 5 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.3: Nested For Loops for Matrix Summation") {
        std::string code = R"(
int32 main() {
    int32 total = 0;
    for (int32 i = 0; i < 3; i++) {
        for (int32 j = 0; j < 3; j++) {
            total += 1;
        }
    }
    return total == 9 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
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

    SECTION("Case 4.2: Non-Boolean Condition in For Loop") {
        std::string code = R"(
void test() {
    for (int32 i = 0; 100; i++) {}
}
)";
        assert_compile_error(code, "Loop condition must be of type 'bool', got 'int32'");
    }

    SECTION("Case 4.3: For Loop Placed Directly in Class Body") {
        std::string code = R"(
class BadClass {
    for (int32 i = 0; i < 10; i++) {}
}
)";
        assert_compile_error(code, "Statements are not allowed directly in class body");
    }
}
