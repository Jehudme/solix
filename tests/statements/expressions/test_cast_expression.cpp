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

static int32 main() {
    Animal a = new Dog();
    Dog d = (Dog)a;
    return d.speak();
}
)";
        CHECK(run_source(code) == 7);
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

static int32 main() {
    Animal a = new Cat();
    Dog d = (Dog)a;
    return 0;
}
)";
        CHECK_THROWS_WITH(run_source(code), Catch::Matchers::ContainsSubstring("TypeCastException"));
    }
}
