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

    SECTION("Case 4.1: Constructor Name Mismatch") {
        const std::string code = R"(
            class Widget {
                Gadget() {}
            }
        )";
        assert_compile_error(code, "Constructor name 'Gadget' does not match enclosing class 'Widget'");
    }
}
