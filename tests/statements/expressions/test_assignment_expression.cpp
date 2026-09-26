#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("AssignmentExpression - Expressions", "[expressions][assignment]") {
    SECTION("Case 3.1: Chained Assignment") {
        std::string code = R"(
static int32 main() {
    int32 a = 0;
    int32 b = 0;
    int32 c = 0;
    a = b = c = 10;
    if (a != 10 || b != 10 || c != 10) return 1;
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Assigning to Literal / RValue") {
        std::string code = R"(
void test() {
    int32 x = 0;
    10 = x;
}
)";
        assert_compile_error(code, "Invalid assignment target");
    }
}
