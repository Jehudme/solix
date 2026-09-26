#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("FieldDeclaration - Declarations", "[declarations][field]") {
    SECTION("Case 3.1: Cycle Breaking with Weak References") {
        std::string code = R"(
class Parent {
    public Child c;
}
class Child {
    public weak Parent p;
}

static int32 main() {
    Parent p = new Parent();
    Child c = new Child();
    p.c = c;
    c.p = p;
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.6: Static Field Initialization with Object Instantiation") {
        std::string code = R"(
alias String = solix.core.String;

class Config {
    public static String tag = new String("production");

    public static String getTag() {
        return tag;
    }
}

static int32 main() {
    return Config.getTag().equals(new String("production")) ? 0 : 1;
}
)";
        CHECK(run_source(code, true) == 0);
    }

    SECTION("Case 4.1: Duplicate Field Identifier") {
        std::string code = R"(
class Item {
    int32 count;
    float64 count;
}
)";
        assert_compile_error(code, "Field 'count' is already declared in class 'Item'");
    }

    SECTION("Case 4.2: Field Access on Null Reference (Runtime Fault)") {
        std::string code = R"(
class Item {
    public int32 count;
}
static int32 main() {
    Item item = null;
    item.count = 5;
    return 0;
}
)";
        CHECK_THROWS_WITH(run_source(code), Catch::Matchers::ContainsSubstring("NullPointer"));
    }
}
