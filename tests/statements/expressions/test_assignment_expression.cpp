#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("AssignmentExpression - Expressions", "[expressions][assignment]") {
    SECTION("Case 3.1: Chained Assignment") {
        std::string code = R"(
int32 main() {
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

    SECTION("Case 3.2: Compound Assignments") {
        std::string code = R"(
int32 main() {
    int32 x = 10;
    x += 5;
    x -= 3;
    x *= 2;
    x /= 4;
    return x == 6 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.3: Assigning to Object Field Target") {
        std::string code = R"(
class Box {
    public int32 weight;
}

int32 main() {
    Box b = new Box();
    b.weight = 50;
    return b.weight == 50 ? 0 : 1;
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

    SECTION("Case 4.2: Compound Assignment Type Mismatch") {
        std::string code = R"(
void test() {
    int32 x = 10;
    x += "text";
}
)";
        assert_compile_error(code, "Cannot apply operator '+=' to types 'int32' and 'String'");
    }
}
