#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("ArrayLiteralExpression - Expressions", "[expressions][array_literal]") {
    SECTION("Case 3.1: Dual Literal Syntax") {
        std::string code = R"(
int32 main() {
    int32[] a = [10, 20];
    int32[] b = {10, 20};
    if (a[0] != b[0] || a[1] != b[1]) return 1;
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.2: Nested 2D Array Literal") {
        std::string code = R"(
int32 main() {
    int32[][] matrix = {{1, 2}, {3, 4}};
    return (matrix[0][0] + matrix[1][1]) == 5 ? 0 : 1;
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

    SECTION("Case 4.2: Array Literal with Mixed Incompatible Types") {
        std::string code = R"(
void test() {
    int32[] arr = {1, "two", true};
}
)";
        assert_compile_error(code, "Incompatible types in array literal");
    }
}
