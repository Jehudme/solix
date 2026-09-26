#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("ArrayCreationExpression - Expressions", "[expressions][array_creation]") {
    SECTION("Case 3.1: Zero-Initialized Array") {
        std::string code = R"(
static int32 main() {
    int32[] data = new int32[5];
    return data[0];
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Negative Size at Runtime (Runtime Fault)") {
        std::string code = R"(
static int32 main() {
    int32[] bad = new int32[-1];
    return 0;
}
)";
        CHECK_THROWS_WITH(run_source(code), Catch::Matchers::ContainsSubstring("NegativeArraySizeException"));
    }
}
