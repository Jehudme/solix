#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("BinaryExpression - Expressions", "[expressions][binary]") {
    SECTION("Case 3.1: Short-Circuit Logical AND") {
        std::string code = R"(
static int32 main() {
    bool result = false && (10 / 0 == 0);
    return result ? 1 : 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Division by Zero (Runtime Fault)") {
        std::string code = R"(
static int32 main() {
    int32 x = 10 / 0;
    return x;
}
)";
        CHECK_THROWS_WITH(run_source(code), Catch::Matchers::ContainsSubstring("Division by zero"));
    }
}
