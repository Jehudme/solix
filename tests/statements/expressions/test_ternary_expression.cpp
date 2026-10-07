#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("TernaryExpression - Expressions", "[expressions][ternary]") {
    SECTION("Case 3.1: Safe Guard with Null Check") {
        std::string code = R"(
class Holder {
    public int32 getVal() {
        return 42;
    }
}

int32 main() {
    Holder h = null;
    int32 len = (h != null) ? h.getVal() : 0;
    if (len != 0) {
        return 1;
    }

    h = new Holder();
    int32 len2 = (h != null) ? h.getVal() : 0;
    if (len2 != 42) {
        return 2;
    }

    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.2: Nested Ternary Evaluation") {
        std::string code = R"(
int32 classify(int32 x) {
    return x > 0 ? 1 : (x < 0 ? -1 : 0);
}

int32 main() {
    return (classify(10) == 1 && classify(-5) == -1 && classify(0) == 0) ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Non-Boolean Condition (Compile-Time Error)") {
        std::string code = R"(
int32 main() {
    int32 res = 5 ? 1 : 2;
    return 0;
}
)";
        assert_compile_error(code, "Ternary condition must be of type 'bool', got 'int32'");
    }

    SECTION("Case 4.2: Mismatched Branch Types") {
        std::string code = R"(
void test(bool cond) {
    int32 val = cond ? 42 : "string";
}
)";
        assert_compile_error(code, "Ternary branches must have the same type");
    }
}

