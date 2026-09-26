#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("ThrowStatement - Control Flow", "[control_flow][throw]") {
    SECTION("Case 3.1: Throw Handled by Catch") {
        std::string code = R"(
class Exception {}
class MyException extends Exception {}

static int32 main() {
    int32 caught = 0;
    try {
        throw new MyException();
    } catch (MyException e) {
        caught = 1;
    }
    return caught;
}
)";
        CHECK(run_source(code) == 1);
    }

    SECTION("Case 4.1: Throwing Primitive Value") {
        std::string code = R"(
void test() {
    throw 404;
}
)";
        assert_compile_error(code, "Cannot throw type 'int32': must inherit from 'std.Exception'");
    }

    SECTION("Case 4.2: Throwing Null Reference at Runtime") {
        std::string code = R"(
class Exception {}
static int32 main() {
    Exception e = null;
    throw e;
    return 0;
}
)";
        CHECK_THROWS_WITH(run_source(code), Catch::Matchers::ContainsSubstring("NullPointer"));
    }
}
