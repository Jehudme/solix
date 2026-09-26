#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("MethodCallExpression - Expressions", "[expressions][method_call]") {
    SECTION("Case 3.1: Overloaded Method Selection") {
        std::string code = R"(
class Printer {
    public int32 print(int32 x) { return 1; }
    public int32 print(float64 s) { return 2; }
}

static int32 main() {
    Printer p = new Printer();
    int32 a = p.print(42);
    int32 b = p.print(3.14);
    if (a != 1 || b != 2) return 1;
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: No Matching Overload") {
        std::string code = R"(
class Printer {
    public void print(int32 x) {}
}

void test(Printer p) {
    p.print(true);
}
)";
        assert_compile_error(code, "No matching method");
    }
}
