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

    SECTION("Case 3.2: Return from Inside Try-Finally Executing Finally First") {
        std::string code = R"(
static int32 result = 0;

static int32 compute() {
    try {
        return 42;
    } finally {
        result = 100;
    }
}

static int32 main() {
    int32 val = compute();
    return (val == 42 && result == 100) ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
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

    SECTION("Case 4.3: Returning Value from Void Method") {
        std::string code = R"(
void test() {
    return 42;
}
)";
        assert_compile_error(code, "Cannot return a value from a void method");
    }

    SECTION("Case 4.5: Return Statement Placed at File Top Level") {
        std::string code = R"(
return 0;
)";
        assert_compile_error(code, "'return' statement outside of function or method body");
    }

    SECTION("Case 4.6: Return Statement Placed Directly in Class Body") {
        std::string code = R"(
class BadClass {
    return 0;
}
)";
        assert_compile_error(code, "Statements are not allowed directly in class body");
    }
}
