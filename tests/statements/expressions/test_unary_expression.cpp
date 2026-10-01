#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("UnaryExpression - Expressions", "[expressions][unary]") {
    SECTION("Case 3.1: Postfix vs Prefix") {
        std::string code = R"(
static int32 main() {
    int32 x = 5;
    int32 post = x++; // post = 5, x = 6
    int32 pre = ++x;  // pre = 7, x = 7
    if (post == 5 && pre == 7 && x == 7) {
        return 0;
    }
    return 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.2: Unary Negation on Numeric Expressions") {
        std::string code = R"(
static int32 main() {
    int32 x = 42;
    int32 neg = -x;
    return neg == -42 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.3: Logical NOT on Boolean Variable") {
        std::string code = R"(
static int32 main() {
    bool active = false;
    bool inverted = !active;
    return inverted == true ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.4: Extreme Signed Boundary Negation (INT32_MIN and INT64_MIN)") {
        std::string code = R"(
static int32 main() {
    int32 min32 = -2147483648;
    int32 neg32 = -min32;
    int64 min64 = -9223372036854775808L;
    int64 neg64 = -min64;
    if (neg32 == -2147483648 && neg64 == -9223372036854775808L) {
        return 0;
    }
    return 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Increment on Constant (Compile-Time Error)") {
        std::string code = R"(
static int32 main() {
    ++10;
    return 0;
}
)";
        assert_compile_error(code, "Invalid operand for increment operator: expected lvalue");
    }

    SECTION("Case 4.2: Unary Minus on Non-Numeric Operand") {
        std::string code = R"(
class NonNumeric {}

void test() {
    NonNumeric obj = new NonNumeric();
    NonNumeric neg = -obj;
}
)";
        assert_compile_error(code, "Cannot apply unary operator '-' to type 'NonNumeric'");
    }
}
