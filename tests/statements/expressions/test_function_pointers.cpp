#include "test_helper.hpp"
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace solix::test;

TEST_CASE("FunctionPointers - Expressions", "[expressions][function_pointers]") {
    SECTION("Case 1.1: Direct Assignment and Invocation") {
        std::string code = R"(
public class MathUtils {
    public static int32 add(int32 a, int32 b) {
        return a + b;
    }
}

int32 main() {
    int32(*)(int32, int32) op = MathUtils.add;
    return op(10, 20) == 30 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 1.2: Null Initialization and Reassignment") {
        std::string code = R"(
public class MathUtils {
    public static int32 mul(int32 a, int32 b) {
        return a * b;
    }
}

int32 main() {
    int32(*)(int32, int32) op = null;
    if (op != null) return 1;
    op = MathUtils.mul;
    if (op == null) return 2;
    return op(6, 7) == 42 ? 0 : 3;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 1.3: Higher-Order Function Passing") {
        std::string code = R"(
public class Operations {
    public static int32 sub(int32 a, int32 b) {
        return a - b;
    }

    public static int32 apply(int32 a, int32 b, int32(*)(int32, int32) fn) {
        return fn(a, b);
    }
}

int32 main() {
    int32 res = Operations.apply(50, 18, Operations.sub);
    return res == 32 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 1.4: Type Aliasing (alias) with Function Pointers") {
        std::string code = R"(
alias BinaryOp = int32(*)(int32, int32);

public class Calc {
    public static int32 add(int32 a, int32 b) {
        return a + b;
    }
}

int32 main() {
    BinaryOp op = Calc.add;
    return op(100, 200) == 300 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 1.5: Returning Function Pointer from Method") {
        std::string code = R"(
public class Factory {
    public static int32 double_val(int32 x) {
        return x * 2;
    }

    public static int32(*)(int32) get_fn() {
        return Factory.double_val;
    }
}

int32 main() {
    int32(*)(int32) f = Factory.get_fn();
    return f(21) == 42 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 2.1: Invoking Null Function Pointer Throws NullPointerException") {
        std::string code = R"(
int32 main() {
    int32(*)(int32, int32) op = null;
    return op(1, 2);
}
)";
        CHECK_THROWS_WITH(run_source(code), Catch::Matchers::ContainsSubstring("NullPointer"));
    }

    SECTION("Case 2.2: Parameter Count Mismatch (Compile-Time Error)") {
        std::string code = R"(
public class MathUtils {
    public static int32 add(int32 a, int32 b) {
        return a + b;
    }
}

void test() {
    int32(*)(int32) op = MathUtils.add;
}
)";
        assert_compile_error(code, "Type mismatch in variable declaration");
    }

    SECTION("Case 2.3: Return Type Mismatch (Compile-Time Error)") {
        std::string code = R"(
public class MathUtils {
    public static int32 add(int32 a, int32 b) {
        return a + b;
    }
}

void test() {
    void(*)(int32, int32) op = MathUtils.add;
}
)";
        assert_compile_error(code, "Type mismatch in variable declaration");
    }

    SECTION("Case 2.4: Address of Non-Static Method Error") {
        std::string code = R"(
public class Greeter {
    public void greet() {}
}

void test() {
    void(*)() f = Greeter.greet;
}
)";
        assert_compile_error(code, "Cannot take address of non-static method 'greet'");
    }

    SECTION("Case 2.5: SizeOf Function Pointer Type") {
        std::string code = R"(
int32 main() {
    int32 sz = sizeof(int32(*)(int32, int32));
    return sz == 8 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }
}
