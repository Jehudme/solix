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

    SECTION("Case 3.2: InstanceOf Subclass Evaluates True for Superclass") {
        std::string code = R"(
class Base {}
class Sub extends Base {}

static int32 main() {
    Base obj = new Sub();
    if (obj instanceof Base && obj instanceof Sub) {
        return 0;
    }
    return 1;
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

    SECTION("Case 4.2: InstanceOf with Undeclared Type Name") {
        std::string code = R"(
class Object {}
void test(Object o) {
    bool b = o instanceof NonExistentClass;
}
)";
        assert_compile_error(code, "Cannot resolve type 'NonExistentClass' in instanceof expression");
    }
}
