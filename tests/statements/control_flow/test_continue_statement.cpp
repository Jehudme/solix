#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("ContinueStatement - Control Flow", "[control_flow][continue]") {
    SECTION("Case 3.1: Continue Advances Loop Variable") {
        std::string code = R"(
static int32 main() {
    int32 hits = 0;
    for (int32 i = 0; i < 6; i++) {
        if (i % 2 == 0) continue;
        hits++;
    }
    return hits;
}
)";
        CHECK(run_source(code) == 3);
    }

    SECTION("Case 4.1: Continue Outside Loop") {
        std::string code = R"(
void test() {
    continue;
}
)";
        assert_compile_error(code, "'continue' statement not allowed outside of loop");
    }
}
