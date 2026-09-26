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

static int32 main() {
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

    SECTION("Case 4.1: Non-Boolean Condition (Compile-Time Error)") {
        std::string code = R"(
static int32 main() {
    int32 res = 5 ? 1 : 2;
    return 0;
}
)";
        assert_compile_error(code, "Ternary condition must be of type 'bool', got 'int32'");
    }
}
