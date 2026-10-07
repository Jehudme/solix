#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("CalleeFrameAllocation - ALLOC_FRAME", "[control_flow][alloc_frame]") {
    SECTION("Case 1.1: Polymorphic Virtual Dispatch with Variable Frame Sizes") {
        std::string code = R"(
class Base {
    public virtual int32 compute() {
        return 1;
    }
}

class Derived extends Base {
    public override int32 compute() {
        int32 a = 10;
        int32 b = 20;
        int32 c = 30;
        return a + b + c;
    }
}

int32 main() {
    Base b = new Derived();
    return b.compute() == 60 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 1.2: Deep Call Stack & Recursion Integrity") {
        std::string code = R"(
int32 fib(int32 n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int32 main() {
    int32 res = fib(10);
    return res == 55 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 2.1: Method with Multiple Local Slots") {
        std::string code = R"(
int32 sum_locals(int32 x) {
    int32 l1 = 1;
    int32 l2 = 2;
    int32 l3 = 3;
    int32 l4 = 4;
    int32 l5 = 5;
    return x + l1 + l2 + l3 + l4 + l5;
}

int32 main() {
    return sum_locals(10) == 25 ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }
}
