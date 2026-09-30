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

    SECTION("Case 3.2: Short-Circuit Logical OR") {
        std::string code = R"(
static int32 side_effects = 0;
static bool get_false() { side_effects++; return false; }
static bool get_true() { side_effects++; return true; }

static int32 main() {
    bool res = get_true() || get_false();
    return (res && side_effects == 1) ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.3: Relational Comparisons (<, <=, >, >=)") {
        std::string code = R"(
static int32 main() {
    int32 a = 10;
    int32 b = 20;
    if (a < b && a <= 10 && b > a && b >= 20) {
        return 0;
    }
    return 1;
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

    SECTION("Case 4.2: Incompatible Arithmetic Types") {
        std::string code = R"(
void test() {
    int32 x = 10 + true;
}
)";
        assert_compile_error(code, "Cannot apply operator '+' to types 'int32' and 'bool'");
    }
}
