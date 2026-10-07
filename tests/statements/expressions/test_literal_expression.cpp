#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("LiteralNode - Expressions", "[expressions][literal]") {
    SECTION("Case 3.1: All Literal Types") {
        std::string code = R"(
int32 main() {
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
int32 main() {
    int32 hex = 0x2A;
    int32 bin = 0b101010;
    return (hex == 42 && bin == 42) ? 0 : 1;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 3.3: Character Literals with Escapes") {
        std::string code = R"(
int32 main() {
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

    SECTION("Case 52.1: Common Whitespace and Punctuation Escapes") {
        std::string code = R"(
int32 main() {
    char[] s = "hello\nworld\ttab\\slash\"quote";
    if (s[5] != (char)10) return 1; // '\n'
    if (s[11] != (char)9) return 2; // '\t'
    if (s[15] != (char)92) return 3; // '\\'
    if (s[21] != (char)34) return 4; // '\"'
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 52.2: Hexadecimal Escape Sequences and Null Character") {
        std::string code = R"(
int32 main() {
    char[] s = "A\x42C\0D";
    if (s[0] != 'A') return 1;
    if (s[1] != 'B') return 2; // \x42 == 'B'
    if (s[2] != 'C') return 3;
    if (s[3] != '\0') return 4; // \0 == 0
    if (s[4] != 'D') return 5;
    return 0;
}
)";
        CHECK(run_source(code) == 0);
    }

    SECTION("Case 52.3: Negative: Unterminated Hex Escape in String Literal") {
        std::string code = R"(
void test() {
    char[] s = "bad\x4";
}
)";
        assert_compile_error(code, "Invalid hex character in string literal");
    }
}
