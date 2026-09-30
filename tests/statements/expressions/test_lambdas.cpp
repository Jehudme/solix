#include "test_helper.hpp"
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace solix::test;

TEST_CASE("LambdasAndClosures - Expressions", "[expressions][lambdas]") {
    SECTION("Case 1.1: Stateless Lambda Expression ([])") {
        std::string code = R"(
public class Main {
    public static int32 main() {
        int32(*)(int32, int32) add = [](int32 a, int32 b) => a + b;
        return add(10, 20) == 30 ? 0 : 1;
    }
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 1.2: Lambda Capturing Primitive Local Variable") {
        std::string code = R"(
public class Main {
    public static int32 main() {
        int32 factor = 5;
        int32(*)(int32) mult = [factor](int32 x) => x * factor;
        return mult(6) == 30 ? 0 : 1;
    }
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 1.3: Multi-Parameter Lambda with Block Body and Explicit Return Type") {
        std::string code = R"(
public class Main {
    public static int32 main() {
        int32 base = 100;
        int32(*)(int32, int32) compute = [base](int32 a, int32 b) : int32 {
            int32 sum = a + b;
            return base + sum;
        };
        return compute(20, 3) == 123 ? 0 : 1;
    }
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 1.4: Lambda Capturing Object Reference with ARC Tracking") {
        std::string code = R"(
public class Counter {
    public int32 val;
    public Counter(int32 v) { this.val = v; }
}

public class Main {
    public static int32 main() {
        Counter c = new Counter(42);
        int32(*)() getVal = [c]() => c.val;
        return getVal() == 42 ? 0 : 1;
    }
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 1.5: Returning Closure from Function (Escaping Stack Frame)") {
        std::string code = R"(
public class Main {
    public static int32(*)(int32) makeAdder(int32 x) {
        return [x](int32 y) => x + y;
    }

    public static int32 main() {
        int32(*)(int32) addTen = makeAdder(10);
        return addTen(25) == 35 ? 0 : 1;
    }
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 1.6: Capturing this in Instance Method") {
        std::string code = R"(
public class Multiplier {
    public int32 factor;
    public Multiplier(int32 f) { this.factor = f; }

    public int32(*)(int32) getMultiplier() {
        return [this](int32 x) => x * this.factor;
    }
}

public class Main {
    public static int32 main() {
        Multiplier m = new Multiplier(7);
        int32(*)(int32) fn = m.getMultiplier();
        return fn(6) == 42 ? 0 : 1;
    }
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 1.7: Reassigning Closure Variable in a Loop (ARC Recycling)") {
        std::string code = R"(
public class Main {
    public static int32 main() {
        int32(*)(int32) fn = [](int32 x) => x;
        for (int32 i = 0; i < 50; ++i) {
            int32 cur = i;
            fn = [cur](int32 x) => x + cur;
        }
        return fn(10) == 59 ? 0 : 1;
    }
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 1.8: Nested Closures with Deep Capture Hierarchy") {
        std::string code = R"(
public class Main {
    public static int32 main() {
        int32 a = 10;
        int32(*)(int32) outer = [a](int32 b) : int32 {
            int32(*)(int32) inner = [a, b](int32 c) => a + b + c;
            return inner(30);
        };
        return outer(20) == 60 ? 0 : 1;
    }
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 2.1: Undefined Variable in Capture List (Compile-Time Error)") {
        std::string code = R"(
public class Main {
    public static void test() {
        int32(*)() fn = [nonExistentVar]() => 0;
    }
}
)";
        assert_compile_error(code, "Undefined capture variable: nonExistentVar");
    }

    SECTION("Case 2.2: Capturing this Outside Instance Method (Compile-Time Error)") {
        std::string code = R"(
public class Main {
    public static void test() {
        int32(*)() fn = [this]() => 0;
    }
}
)";
        assert_compile_error(code, "Cannot capture 'this' outside of an instance method");
    }

    SECTION("Case 2.3: Lambda Signature Incompatible with Target Function Pointer (Compile-Time Error)") {
        std::string code = R"(
public class Main {
    public static void test() {
        int32(*)(int32, int32) fn = [](int32 a) => a;
    }
}
)";
        assert_compile_error(code, "Type mismatch in variable declaration");
    }
}
