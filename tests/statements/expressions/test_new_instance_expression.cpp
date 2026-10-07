#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("NewInstanceExpression - Expressions", "[expressions][new_instance]") {
    SECTION("Case 3.1: Instantiation with Overloaded Constructor") {
        std::string code = R"(
class Item {
    public int32 val;
    public Item() { this.val = 0; }
    public Item(int32 v) { this.val = v; }
}

int32 main() {
    Item i1 = new Item();
    Item i2 = new Item(42);
    if (i1.val == 0 && i2.val == 42) {
        return 0;
    }
    return 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.2: Instantiating Class with In-Class Field Initializers") {
        std::string code = R"(
class Config {
    public int32 timeout = 3000;
    public bool enabled = true;
}

int32 main() {
    Config c = new Config();
    return (c.timeout == 3000 && c.enabled == true) ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Instantiating Abstract Class (Compile-Time Error)") {
        std::string code = R"(
abstract class Base {}

int32 main() {
    Base b = new Base();
    return 0;
}
)";
        assert_compile_error(code, "Cannot instantiate abstract class 'Base'");
    }

    SECTION("Case 4.2: Calling Non-Existent Constructor Overload") {
        std::string code = R"(
class Box {
    public Box(int32 w) {}
}

void test() {
    Box b = new Box("bad");
}
)";
        assert_compile_error(code, "No matching constructor");
    }
}

