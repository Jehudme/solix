#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("ArrayAccessExpression - Expressions", "[expressions][array_access]") {
    SECTION("Case 3.1: In-Bounds Read and Write") {
        std::string code = R"(
static int32 main() {
    int32[] buffer = new int32[3];
    buffer[0] = 100;
    buffer[1] = 200;
    int32 sum = buffer[0] + buffer[1];
    return sum;
}
)";
        CHECK(run_source(code) == 300);
    }

    SECTION("Case 4.1: Index Out of Bounds (Runtime Fault)") {
        std::string code = R"(
static int32 main() {
    int32[] data = new int32[2];
    int32 fail = data[5];
    return fail;
}
)";
        CHECK_THROWS_WITH(run_source(code), Catch::Matchers::ContainsSubstring("Out of Bounds"));
    }
}
