#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("ConstructorDeclaration - Declarations", "[declarations][constructor]") {
    SECTION("Case 3.1: Chained Super Constructor") {
        const std::string code = R"(
            class Base {
                public int32 id;
                public Base(int32 id) {
                    this.id = id;
                }
            }

            class Sub extends Base {
                public Sub(int32 id) : super(id) {}
            }

            class Main {
                public static int32 main() {
                    Sub s = new Sub(42);
                    return s.id == 42 ? 0 : 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 3.2: Overloaded Constructors with Varying Arity") {
        const std::string code = R"(
            class Point {
                public int32 x;
                public int32 y;
                public Point() { this.x = 0; this.y = 0; }
                public Point(int32 x, int32 y) { this.x = x; this.y = y; }
            }

            class Main {
                public static int32 main() {
                    Point p1 = new Point();
                    Point p2 = new Point(10, 20);
                    return (p1.x == 0 && p2.x == 10 && p2.y == 20) ? 0 : 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 3.3: Protected Constructor for Subclass Construction") {
        const std::string code = R"(
            class BaseAuth {
                protected BaseAuth() {}
            }

            class UserAuth extends BaseAuth {
                public UserAuth() { super(); }
            }

            class Main {
                public static int32 main() {
                    UserAuth u = new UserAuth();
                    return 0;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 4.1: Constructor Name Mismatch") {
        const std::string code = R"(
            class Widget {
                Gadget() {}
            }
        )";
        assert_compile_error(code, "Constructor name 'Gadget' does not match enclosing class 'Widget'");
    }

    SECTION("Case 4.2: Constructor with Explicit Return Type") {
        const std::string code = R"(
            class Widget {
                void Widget() {}
            }
        )";
        assert_compile_error(code, "Constructors must not specify a return type");
    }

    SECTION("Case 4.3: Invoking Super Constructor Out of Order") {
        const std::string code = R"(
            class Base {}
            class Sub extends Base {
                public int32 val;
                public Sub() {
                    this.val = 42;
                    super();
                }
            }
        )";
        assert_compile_error(code, "Call to 'super()' must be the first statement in constructor");
    }

    SECTION("Case 4.4: Constructor Declared Outside Any Class at Top Level") {
        const std::string code = R"(
            StandaloneConstructor() {
            }
        )";
        assert_compile_error(code, "Constructors can only be declared inside a class body");
    }

    SECTION("Case 4.5: Constructor Declared Inside Method Body") {
        const std::string code = R"(
            void test() {
                MyClass() {}
            }
        )";
        assert_compile_error(code, "Constructors can only be declared inside a class body");
    }
}
