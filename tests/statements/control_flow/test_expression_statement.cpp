#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("ExpressionStatement - Control Flow", "[control_flow][expression]") {
    SECTION("Case 3.1: Method Calls and Assignments") {
        std::string code = R"(
static int32 main() {
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
static Res generate() { return new Res(42); }
static int32 main() {
    generate();
    return 0;
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
static int32 main() {
    Res r = null;
    return r.get_val();
}
)";
        CHECK_THROWS_WITH(run_source(code), Catch::Matchers::ContainsSubstring("NullPointer"));
    }
}
