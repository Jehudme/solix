#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("ThrowStatement - Control Flow", "[control_flow][throw]") {
    SECTION("Case 3.1: Throw Handled by Catch") {
        std::string code = R"(
class Exception {}
class MyException extends Exception {}

int32 main() {
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

    SECTION("Case 3.2: Rethrowing Caught Exception Instance") {
        std::string code = R"(
class Exception {}

int32 main() {
    bool caught_outer = false;
    try {
        try {
            throw new Exception();
        } catch (Exception e) {
            throw e;
        }
    } catch (Exception outer) {
        caught_outer = true;
    }
    return caught_outer ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
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
int32 main() {
    Exception e = null;
    throw e;
    return 0;
}
)";
        CHECK_THROWS_WITH(run_source(code), Catch::Matchers::ContainsSubstring("NullPointer"));
    }

    SECTION("Case 4.3: Throwing Uninstantiated Class Identifier") {
        std::string code = R"(
class Exception {}
void test() {
    throw Exception;
}
)";
        assert_compile_error(code, "Cannot throw non-instantiated type 'Exception'");
    }

    SECTION("Case 4.5: Throw Statement Placed Directly in Class Body") {
        std::string code = R"(
class Exception {}
class BadClass {
    throw new Exception();
}
)";
        assert_compile_error(code, "Statements are not allowed directly in class body");
    }
}
