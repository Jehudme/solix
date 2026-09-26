#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("ReturnStatement - Control Flow", "[control_flow][return]") {
    SECTION("Case 3.1: Early Return from Nested Scopes") {
        std::string code = R"(
class Res {
    int32 id;
    Res(int32 id) { this.id = id; }
}
static int32 find(bool fast) {
    Res a = new Res(1);
    {
        Res b = new Res(2);
        if (fast) return 1;
    }
    return 0;
}
static int32 main() {
    return find(true);
}
)";
        CHECK(run_source(code) == 1);
    }

    SECTION("Case 4.1: Missing Return Value in Non-Void Method") {
        std::string code = R"(
int32 get_val() {
    return;
}
)";
        assert_compile_error(code, "Must return a value from non-void method");
    }

    SECTION("Case 4.2: Return Type Mismatch") {
        std::string code = R"(
int32 get_num() {
    return "text";
}
)";
        assert_compile_error(code, "Return type mismatch");
    }
}
