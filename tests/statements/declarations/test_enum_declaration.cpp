#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("EnumDeclaration - Declarations", "[declarations][enum]") {
    SECTION("Case 3.1: Enum Equality and Switch") {
        const std::string code = R"(
            enum Color {
                RED,
                GREEN,
                BLUE
            }

            class Main {
                public static int32 main() {
                    Color c = Color.RED;
                    if (c == Color.RED) {
                        return 0;
                    }
                    return 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 4.1: Duplicate Enum Member") {
        const std::string code = R"(
            enum State {
                READY,
                READY
            }
        )";
        assert_compile_error(code, "Duplicate enum member 'READY' in enum 'State'");
    }
}
