#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("BlockStatement - Control Flow", "[control_flow][block]") {
    SECTION("Case 3.1: Empty and Nested Empty Blocks") {
        std::string code = R"(
static int32 main() {
    {}
    {
        {}
        { {} }
    }
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.2: Lexical Variable Shadowing") {
        std::string code = R"(
static int32 main() {
    int32 value = 10;
    {
        int32 value = 20;
        if (value != 20) return 1;
    }
    if (value != 10) return 2;
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.3: Strict LIFO Destruction of Multiple Reference Objects") {
        std::string code = R"(
class Res {
    int32 id;
    Res(int32 id) { this.id = id; }
}
static int32 main() {
    {
        Res first = new Res(1);
        Res second = new Res(2);
        Res third = new Res(3);
    }
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.4: Early Return from Nested Blocks") {
        std::string code = R"(
class Res {
    int32 id;
    Res(int32 id) { this.id = id; }
}
static int32 compute(bool early) {
    Res a = new Res(1);
    {
        Res b = new Res(2);
        if (early) {
            return 100;
        }
    }
    return 0;
}
static int32 main() {
    return compute(true);
}
)";
        CHECK(run_source(code) == 100);
    }

    SECTION("Case 4.1: Accessing Block-Scoped Variable Outside Its Block") {
        std::string code = R"(
void test() {
    {
        int32 temp = 100;
    }
    int32 leak = temp;
}
)";
        assert_compile_error(code, "Undefined identifier: temp");
    }

    SECTION("Case 4.2: Duplicate Variable in Same Immediate Scope") {
        std::string code = R"(
void test() {
    {
        int32 score = 10;
        float64 score = 20.0;
    }
}
)";
        assert_compile_error(code, "already defined");
    }

    SECTION("Case 4.3: Unclosed Block (Missing Brace)") {
        std::string code = R"(
void test() {
    {
        int32 x = 1;
)";
        assert_compile_error(code, "Expected '}'");
    }

    SECTION("Case 4.4: Stray Extra Closing Brace") {
        std::string code = R"(
void test() {
    { int32 x = 1; }
    }
}
)";
        assert_compile_error(code, "unexpected token '}'");
    }
}
