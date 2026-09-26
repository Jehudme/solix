#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("InstanceOfExpression - Expressions", "[expressions][instanceof]") {
    SECTION("Case 3.1: Safe Null Evaluation") {
        std::string code = R"(
class Animal {}
class Dog extends Animal {}

static int32 main() {
    Animal a = null;
    bool check = a instanceof Dog;
    return check ? 1 : 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Primitive Target") {
        std::string code = R"(
void test() {
    bool b = 10 instanceof int32;
}
)";
        assert_compile_error(code, "'instanceof' cannot be applied to primitive types");
    }
}
