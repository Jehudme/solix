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

    SECTION("Case 3.2: Reference Type Array Allocation") {
        std::string code = R"(
class Item {
    public int32 id;
}

static int32 main() {
    Item[] items = new Item[3];
    return items[0] == null ? 0 : 1;
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

    SECTION("Case 4.2: Array Creation with Missing Size") {
        std::string code = R"(
void test() {
    int32[] arr = new int32[]();
}
)";
        assert_compile_error(code, "Array allocation requires size expression");
    }
}
