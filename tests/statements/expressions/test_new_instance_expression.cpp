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

static int32 main() {
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

    SECTION("Case 4.1: Instantiating Abstract Class (Compile-Time Error)") {
        std::string code = R"(
abstract class Base {}

static int32 main() {
    Base b = new Base();
    return 0;
}
)";
        assert_compile_error(code, "Cannot instantiate abstract class 'Base'");
    }
}
