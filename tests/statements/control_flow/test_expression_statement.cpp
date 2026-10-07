#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("ExpressionStatement - Control Flow", "[control_flow][expression]") {
    SECTION("Case 3.1: Method Calls and Assignments") {
        std::string code = R"(
int32 main() {
    int32 x = 0;
    x = 10;
    x++;
    return x;
}
)";
        CHECK(run_source(code) == 11);
    }

    SECTION("Case 3.2: Immediate Temporary Reclamation") {
        std::string code = R"(
class Res {
    int32 val;
    Res(int32 v) { this.val = v; }
}
Res generate() { return new Res(42); }
int32 main() {
    generate();
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.3: Chained Fluent Method Calls") {
        std::string code = R"(
class Builder {
    public int32 val;
    public Builder add(int32 x) { this.val += x; return this; }
}

int32 main() {
    Builder b = new Builder();
    b.add(10).add(20).add(30);
    return b.val == 60 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Missing Semicolon") {
        std::string code = R"(
void test() {
    int32 x = 0;
    x = 10
}
)";
        assert_compile_error(code, "Expected ';'");
    }

    SECTION("Case 4.2: Method Call on Null Reference (Runtime Fault)") {
        std::string code = R"(
class Res {
    public virtual int32 get_val() { return 10; }
}
int32 main() {
    Res r = null;
    return r.get_val();
}
)";
        CHECK_THROWS_WITH(run_source(code), Catch::Matchers::ContainsSubstring("NullPointer"));
    }

    SECTION("Case 4.3: Incomplete Expression Statement") {
        std::string code = R"(
void test() {
    int32 x = ;
}
)";
        assert_compile_error(code, "Expected expression, got ';'");
    }

    SECTION("Case 4.4: Expression Statement Directly in Class Body") {
        std::string code = R"(
class BadClass {
    10 + 20;
}
)";
        assert_compile_error(code, "Statements are not allowed directly in class body");
    }
}
