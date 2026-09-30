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
    assert_compile_error(code, "Circular inheritance detected for class");
  }

  SECTION("Case 4.2: Unimplemented Abstract Method") {
    const std::string code = R"(
            abstract class Shape {
                public abstract float64 area();
            }

            class Circle extends Shape {}
        )";
    assert_compile_error(
        code, "must implement abstract method 'area()' from 'Shape'");
  }

  SECTION("Case 3.2: Multi-Level Inheritance Chain") {
    const std::string code = R"(
            class GrandParent {
                public int32 a;
            }
            class Parent extends GrandParent {
                public int32 b;
            }
            class Child extends Parent {
                public int32 c;
            }

            class Main {
                public static int32 main() {
                    Child obj = new Child();
                    obj.a = 1;
                    obj.b = 2;
                    obj.c = 3;
                    return (obj.a + obj.b + obj.c) == 6 ? 0 : 1;
                }
            }
        )";
    assert_compile_success(code);
    REQUIRE(run_and_evaluate_int(code) == 0);
  }

  SECTION("Case 3.3: Abstract Class Extension and Implementation") {
    const std::string code = R"(
            abstract class Shape {
                public abstract int32 getArea();
            }

            class Square extends Shape {
                public int32 side;
                public Square(int32 s) { this.side = s; }
                public override int32 getArea() { return this.side * this.side; }
            }

            class Main {
                public static int32 main() {
                    Shape s = new Square(5);
                    return s.getArea() == 25 ? 0 : 1;
                }
            }
        )";
    assert_compile_success(code);
    REQUIRE(run_and_evaluate_int(code) == 0);
  }

  SECTION("Case 3.4: Class Access Modifiers (Public vs Internal)") {
    const std::string code = R"(
            public class ExportedService {
                public int32 serve() { return 100; }
            }

            internal class InternalHelper {
                public int32 help() { return 200; }
            }

            class Main {
                public static int32 main() {
                    ExportedService es = new ExportedService();
                    InternalHelper ih = new InternalHelper();
                    return (es.serve() + ih.help()) == 300 ? 0 : 1;
                }
            }
        )";
    assert_compile_success(code);
    REQUIRE(run_and_evaluate_int(code) == 0);
  }

  SECTION("Case 3.6: Nested Class Declaration Inside Class") {
    const std::string code = R"(
            class Outer {
                public int32 outer_val;

                public class Inner {
                    public int32 inner_val;
                    public Inner(int32 v) { this.inner_val = v; }
                }
            }

            class Main {
                public static int32 main() {
                    Outer.Inner obj = new Outer.Inner(42);
                    return obj.inner_val == 42 ? 0 : 1;
                }
            }
        )";
    assert_compile_success(code);
    REQUIRE(run_and_evaluate_int(code) == 0);
  }

  SECTION("Case 4.3: Multiple Class Inheritance Disallowed") {
    const std::string code = R"(
            class A {}
            class B {}
            class C extends A, B {}
        )";
    assert_compile_error(code, "Expected '{' after class inheritance clause");
  }

  SECTION("Case 4.4: Extending Non-Class or Primitive Type") {
    const std::string code = R"(
            class Invalid extends int32 {}
        )";
    assert_compile_error(code, "Cannot extend non-class type 'int32'");
  }

  SECTION("Case 4.5: Instantiating Abstract Class Directly") {
    const std::string code = R"(
            abstract class AbstractBase {}

            class Main {
                public static int32 main() {
                    AbstractBase a = new AbstractBase();
                    return 0;
                }
            }
        )";
    assert_compile_error(code, "Cannot instantiate abstract class 'AbstractBase'");
  }

  SECTION("Case 4.6: Class Declared Inside Function Body") {
    const std::string code = R"(
            class Main {
                public static int32 main() {
                    class LocalClass {
                        int32 x;
                    }
                    return 0;
                }
            }
        )";
    assert_compile_error(code, "Classes cannot be declared inside a function or method body");
  }

  SECTION("Case 4.7: Class Declared Inside Control Flow Block") {
    const std::string code = R"(
            class Main {
                public static int32 main() {
                    if (true) {
                        class BlockClass {}
                    }
                    return 0;
                }
            }
        )";
    assert_compile_error(code, "Classes cannot be declared inside");
  }
}
