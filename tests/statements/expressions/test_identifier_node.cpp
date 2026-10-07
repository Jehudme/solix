#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("IdentifierNode - Expressions", "[expressions][identifier]") {
    SECTION("Case 3.1: Local Resolution Precedence") {
        std::string code = R"(
int32 main() {
    int32 val = 100;
    {
        int32 val = 200;
        return val;
    }
}
)";
        CHECK(run_source(code) == 200);
    }

    SECTION("Case 3.2: Explicit Member Access via this Identifier") {
        std::string code = R"(
class ScopeTest {
    public int32 val;
    public void setVal(int32 val) {
        this.val = val;
    }
}
int32 main() {
    ScopeTest s = new ScopeTest();
    s.setVal(42);
    return s.val;
}
)";
        CHECK(run_source(code) == 42);
    }

    SECTION("Case 4.1: Undefined Identifier") {
        std::string code = R"(
void test() {
    int32 a = unknown_var;
}
)";
        assert_compile_error(code, "Undefined identifier: unknown_var");
    }

    SECTION("Case 4.2: Accessing Local Variable Before Declaration") {
        std::string code = R"(
void test() {
    x = 10;
    int32 x = 0;
}
)";
        assert_compile_error(code, "Undefined identifier: x");
    }

    SECTION("Case 4.3: Using This Keyword Outside Any Class") {
        std::string code = R"(
void test() {
    int32 x = this.val;
}
)";
        assert_compile_error(code, "Keyword 'this' is only valid within non-static class member methods");
    }

    SECTION("Case 4.4: Using This Keyword Inside Static Method") {
        std::string code = R"(
class Example {
    public static void staticMethod() {
        int32 x = this.val;
    }
}
)";
        assert_compile_error(code, "Keyword 'this' cannot be used in a static method");
    }
}

