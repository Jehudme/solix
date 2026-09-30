#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("LiteralNode - Expressions", "[expressions][literal]") {
    SECTION("Case 3.1: All Literal Types") {
        std::string code = R"(
static int32 main() {
    int64 big = 10000000000;
    char letter = 'Z';
    bool flag = true;
    float64 pi = 3.14;
    if (letter != 'Z' || !flag) return 1;
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.2: Hexadecimal and Binary Numeric Literals") {
        std::string code = R"(
static int32 main() {
    int32 hex = 0x2A;
    int32 bin = 0b101010;
    return (hex == 42 && bin == 42) ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.3: Character Literals with Escapes") {
        std::string code = R"(
static int32 main() {
    char newline = '\n';
    char tab = '\t';
    char quote = '\'';
    if (newline == 10 && tab == 9 && quote == 39) {
        return 0;
    }
    return 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 4.1: Integer Literal Overflow") {
        std::string code = R"(
void test() {
    int32 x = 99999999999999999999;
}
)";
        assert_compile_error(code, "Integer literal out of range");
    }

    SECTION("Case 4.2: Unterminated String Literal") {
        std::string code = R"(
void test() {
    String s = "unterminated;
}
)";
        assert_compile_error(code, "Unterminated string literal");
    }
}
