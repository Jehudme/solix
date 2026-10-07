#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("SizeOfExpression - Expressions", "[expressions][sizeof]") {
    SECTION("Case 1.1: Primitive Type Sizing") {
        std::string code = R"(
int32 main() {
    int32 s1 = sizeof(int8);
    int32 s2 = sizeof(int16);
    int32 s4 = sizeof(int32);
    int32 s8 = sizeof(int64);
    int32 sf4 = sizeof(float32);
    int32 sf8 = sizeof(float64);
    int32 sb = sizeof(bool);
    int32 sc = sizeof(char);
    if (s1 == 1 && s2 == 2 && s4 == 4 && s8 == 8 && sf4 == 4 && sf8 == 8 && sb == 1 && sc == 1) {
        return 0;
    }
    return 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 1.2: Class Type Compile-Time Sizing") {
        std::string code = R"(
class Point {
    int32 x;
    int32 y;
}
int32 main() {
    int32 sz = sizeof(Point);
    return sz >= 16 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 2.1: Dynamic Instance Sizing on Heap Objects") {
        std::string code = R"(
class Item {
    int32 a;
    int32 b;
}
int32 main() {
    Item item = new Item();
    int32 sz = sizeof(item);
    return sz >= 16 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.1: SizeOf on Undeclared Identifier") {
        std::string code = R"(
void test() {
    int32 s = sizeof(NonExistentType);
}
)";
        assert_compile_error(code, "Unknown type 'NonExistentType' in sizeof expression");
    }
}
