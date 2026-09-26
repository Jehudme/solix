#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("LiteralNode - Expressions", "[expressions][literal]") {
    SECTION("Case 3.1: All Literal Types") {
        std::string code = R"(
static int32 main() {
    int64 big = 10000000000;
    char letter = 'Z';
    bool flag = true;
    float64 pi = 3.14;
    if (letter != 'Z' || !flag) return 1;
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Integer Literal Overflow") {
        std::string code = R"(
void test() {
    int32 x = 99999999999999999999;
}
)";
        assert_compile_error(code, "Integer literal out of range");
    }
}
