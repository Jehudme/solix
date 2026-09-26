#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("ArrayLiteralExpression - Expressions", "[expressions][array_literal]") {
    SECTION("Case 3.1: Dual Literal Syntax") {
        std::string code = R"(
static int32 main() {
    int32[] a = [10, 20];
    int32[] b = {10, 20};
    if (a[0] != b[0] || a[1] != b[1]) return 1;
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Incompatible Literal Elements") {
        std::string code = R"(
void test() {
    int32[] arr = [10, "text"];
}
)";
        assert_compile_error(code, "Incompatible types in array literal");
    }
}
