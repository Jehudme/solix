#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("ClassDeclaration - Declarations", "[declarations][class]") {
    SECTION("Case 3.1: Single Inheritance and Dynamic Dispatch") {
        const std::string code = R"(
            class Animal {
                public virtual int32 sound() {
                    return 1;
                }
            }

            class Cat extends Animal {
                public override int32 sound() {
                    return 2;
                }
            }

            class Main {
                public static int32 main() {
                    Animal pet = new Cat();
                    return pet.sound() == 2 ? 0 : 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 4.1: Circular Class Inheritance") {
        const std::string code = R"(
            class A extends B {}
            class B extends A {}
        )";
        assert_compile_error(code, "Circular inheritance detected");
    }

    SECTION("Case 4.2: Unimplemented Abstract Method") {
        const std::string code = R"(
            abstract class Shape {
                public abstract float64 area();
            }

            class Circle extends Shape {}
        )";
        assert_compile_error(code, "must implement abstract method 'area()' from 'Shape'");
    }
}
