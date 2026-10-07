#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("ArrayAccessExpression - Expressions", "[expressions][array_access]") {
    SECTION("Case 3.1: In-Bounds Read and Write") {
        std::string code = R"(
int32 main() {
    int32[] buffer = new int32[3];
    buffer[0] = 100;
    buffer[1] = 200;
    int32 sum = buffer[0] + buffer[1];
    return sum;
}
)";
        CHECK(run_source(code) == 300);
    }

    SECTION("Case 3.2: Multi-Dimensional Array Access") {
        std::string code = R"(
int32 main() {
    int32[][] grid = new int32[][2];
    grid[0] = new int32[2];
    grid[0][1] = 42;
    return grid[0][1] == 42 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Index Out of Bounds (Runtime Fault)") {
        std::string code = R"(
int32 main() {
    int32[] data = new int32[2];
    int32 fail = data[5];
    return fail;
}
)";
        CHECK_THROWS_WITH(run_source(code), Catch::Matchers::ContainsSubstring("Out of Bounds"));
    }

    SECTION("Case 4.2: Non-Integer Array Subscript") {
        std::string code = R"(
void test(int32[] arr) {
    int32 v = arr["key"];
}
)";
        assert_compile_error(code, "Array index must be int32, got 'String'");
    }

    SECTION("Case 4.4: Subscript Access on Null Reference at Runtime") {
        std::string code = R"(
int32 main() {
    int32[] arr = null;
    int32 v = arr[0];
    return 0;
}
)";
        CHECK_THROWS_WITH(run_source(code), Catch::Matchers::ContainsSubstring("NullPointer"));
    }
}
