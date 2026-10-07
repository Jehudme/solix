#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("CastExpression - Expressions", "[expressions][cast]") {
    SECTION("Case 3.1: Valid Downcast") {
        std::string code = R"(
class Animal {
    public virtual int32 speak() { return 1; }
}
class Dog extends Animal {
    public override int32 speak() { return 7; }
}

int32 main() {
    Animal a = new Dog();
    Dog d = (Dog)a;
    return d.speak();
}
)";
        CHECK(run_source(code) == 7);
    }

    SECTION("Case 3.2: Numeric Widening and Narrowing Conversions") {
        std::string code = R"(
int32 main() {
    int32 small = 42;
    int64 big = (int64)small;
    int8 tiny = (int8)small;
    return (big == 42 && tiny == 42) ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.4: Casting Null Literal to Reference Type") {
        std::string code = R"(
class Person {}

int32 main() {
    Person p = (Person)null;
    return p == null ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Bad Downcast (Runtime Fault)") {
        std::string code = R"(
class Animal {
    public virtual int32 speak() { return 1; }
}
class Dog extends Animal {
    public override int32 speak() { return 7; }
}
class Cat extends Animal {
    public override int32 speak() { return 3; }
}

int32 main() {
    Animal a = new Cat();
    Dog d = (Dog)a;
    return 0;
}
)";
        CHECK_THROWS_WITH(run_source(code), Catch::Matchers::ContainsSubstring("TypeCastException"));
    }

    SECTION("Case 4.2: Compile-Time Rejection of Unrelated Class Cast") {
        std::string code = R"(
class Cat {}
class Dog {}

void test() {
    Cat c = new Cat();
    Dog d = (Dog)c;
}
)";
        assert_compile_error(code, "Cannot cast between unrelated types 'Cat' and 'Dog'");
    }

    SECTION("Case 4.4: Cast Expression with Non-Type Identifier") {
        std::string code = R"(
void test(int32 x) {
    int32 y = (123)x;
}
)";
        assert_compile_error(code, "Expected type name in cast expression");
    }
}
