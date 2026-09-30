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

    SECTION("Case 3.2: Calling Inherited Superclass Method") {
        std::string code = R"(
class BaseCalc {
    public int32 add(int32 a, int32 b) { return a + b; }
}
class AdvancedCalc extends BaseCalc {
    public int32 doubleAdd(int32 a, int32 b) {
        return this.add(a, b) * 2;
    }
}

static int32 main() {
    AdvancedCalc calc = new AdvancedCalc();
    return calc.doubleAdd(3, 4) == 14 ? 0 : 1;
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

    SECTION("Case 4.2: Method Call with Incorrect Argument Count") {
        std::string code = R"(
class Calculator {
    public int32 compute(int32 a, int32 b) { return a + b; }
}

void test() {
    Calculator c = new Calculator();
    c.compute(10);
}
)";
        assert_compile_error(code, "No matching method");
    }
}

