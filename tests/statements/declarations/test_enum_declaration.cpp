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

    SECTION("Case 3.2: Enum as Method Parameter and Return Value") {
        const std::string code = R"(
            enum State { PENDING, ACTIVE, CLOSED }

            class Task {
                public State state;
                public void setState(State s) { this.state = s; }
                public State getState() { return this.state; }
            }

            class Main {
                public static int32 main() {
                    Task t = new Task();
                    t.setState(State.ACTIVE);
                    return t.getState() == State.ACTIVE ? 0 : 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 3.4: Nested Enum Declaration Inside Class") {
        const std::string code = R"(
            class Window {
                public enum State { MINIMIZED, MAXIMIZED, NORMAL }
                public State state;
            }

            class Main {
                public static int32 main() {
                    Window w = new Window();
                    w.state = Window.State.MAXIMIZED;
                    return w.state == Window.State.MAXIMIZED ? 0 : 1;
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

    SECTION("Case 4.2: Implicit Integer Assignment to Enum") {
        const std::string code = R"(
            enum Status { OK, FAIL }

            void test() {
                Status s = 1;
            }
        )";
        assert_compile_error(code, "Cannot convert type 'int32' to enum 'Status'");
    }

    SECTION("Case 4.3: Comparing Incompatible Enum Types") {
        const std::string code = R"(
            enum Fruit { APPLE, ORANGE }
            enum Animal { CAT, DOG }

            void test() {
                bool b = (Fruit.APPLE == Animal.CAT);
            }
        )";
        assert_compile_error(code, "Operator '==' cannot be applied to incompatible enums 'Fruit' and 'Animal'");
    }

    SECTION("Case 4.4: Enum Declared Inside Function Body") {
        const std::string code = R"(
            void test() {
                enum LocalState { ON, OFF }
            }
        )";
        assert_compile_error(code, "Enums cannot be declared inside a function or method body");
    }
}
