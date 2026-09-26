#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("IdentifierNode - Expressions", "[expressions][identifier]") {
    SECTION("Case 3.1: Local Resolution Precedence") {
        std::string code = R"(
static int32 main() {
    int32 val = 100;
    {
        int32 val = 200;
        return val;
    }
}
)";
        CHECK(run_source(code) == 200);
    }

    SECTION("Case 4.1: Undefined Identifier") {
        std::string code = R"(
void test() {
    int32 a = unknown_var;
}
)";
        assert_compile_error(code, "Undefined identifier: unknown_var");
    }
}
