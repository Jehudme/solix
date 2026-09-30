#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("IfStatement - Control Flow", "[control_flow][if]") {
    SECTION("Case 3.1: Single Branch If") {
        std::string code = R"(
static int32 main() {
    int32 x = 10;
    if (x > 5) {
        x = 20;
    }
    return x;
}
)";
        CHECK(run_source(code) == 20);
    }

    SECTION("Case 3.2: If-Else Chain") {
        std::string code = R"(
static int32 main() {
    int32 score = 85;
    int32 grade = 0;
    if (score >= 90) {
        grade = 1;
    } else if (score >= 80) {
        grade = 2;
    } else {
        grade = 3;
    }
    return grade;
}
)";
        CHECK(run_source(code) == 2);
    }

    SECTION("Case 3.3: Complex Short-Circuit Logical Condition") {
        std::string code = R"(
static int32 main() {
    int32 a = 10;
    int32 b = 20;
    bool executed = false;
    if (a == 10 && (b == 20 || false)) {
        executed = true;
    }
    return executed ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Non-Boolean Condition Type") {
        std::string code = R"(
void test() {
    int32 count = 1;
    if (count) {
    }
}
)";
        assert_compile_error(code, "If condition must be of type 'bool', got 'int32'");
    }

    SECTION("Case 4.2: Incompatible Assignment in Condition") {
        std::string code = R"(
void test() {
    int32 x = 0;
    if (x = 5) {}
}
)";
        assert_compile_error(code, "If condition must be of type 'bool', got 'int32'");
    }

    SECTION("Case 4.3: If Statement Placed Directly in Class Body") {
        std::string code = R"(
class BadClass {
    if (true) {}
}
)";
        assert_compile_error(code, "Statements are not allowed directly in class body");
    }
}
