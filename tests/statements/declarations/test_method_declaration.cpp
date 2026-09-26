#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("MethodDeclaration - Declarations", "[declarations][method]") {
    SECTION("Case 3.1: Virtual Method Overriding") {
        std::string code = R"(
class Parent {
    public virtual int32 get_val() { return 1; }
}
class Child extends Parent {
    public override int32 get_val() { return 2; }
}

static int32 main() {
    Parent p = new Child();
    return p.get_val();
}
)";
        CHECK(run_source(code) == 2);
    }

    SECTION("Case 4.1: Abstract Method with Body") {
        std::string code = R"(
abstract class Base {
    abstract void run() {}
}
)";
        assert_compile_error(code, "Abstract method 'run' cannot have a body");
    }
}
