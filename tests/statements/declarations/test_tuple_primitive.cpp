#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("TuplePrimitive - Declarations", "[declarations][tuple]") {
    SECTION("Case 55.1: Direct tuple Creation & Constant Indexing") {
        const std::string code = R"(
            int32 main() {
                tuple t = new tuple(42, 'H', true);
                if (t.length != 3) return 1;
                int32 v0 = (int32)t[0];
                char v1 = (char)t[1];
                bool v2 = (bool)t[2];
                if (v0 != 42 || v1 != 'H' || !v2) return 2;
                return 0;
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 55.2: Dynamic Loop Iteration over tuple") {
        const std::string code = R"(
            int32 main() {
                tuple t = new tuple(10, 20, 30);
                int32 sum = 0;
                for (int32 i = 0; i < t.length; i++) {
                    sum = sum + (int32)t[i];
                }
                return sum == 60 ? 0 : 1;
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 55.3: Runtime instanceof with Primitives") {
        const std::string code = R"(
            int32 main() {
                tuple t = new tuple(42, true, 'X');
                if (!(t[0] instanceof int32)) return 1;
                if (t[0] instanceof bool) return 2;
                if (!(t[1] instanceof bool)) return 3;
                if (t[1] instanceof int32) return 4;
                return 0;
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 55.4: Runtime instanceof with Classes & Interfaces") {
        const std::string code = R"(
            interface Worker {
                int32 work();
            }

            class Robot implements Worker {
                public int32 work() { return 100; }
            }

            class Person {}

            int32 main() {
                tuple t = new tuple(new Robot(), new Person());
                if (!(t[0] instanceof Worker)) return 1;
                if (!(t[0] instanceof Robot)) return 2;
                if (t[0] instanceof Person) return 3;
                if (t[1] instanceof Worker) return 4;
                if (!(t[1] instanceof Person)) return 5;
                return 0;
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 55.5: Function Template Lowering with <args...>") {
        const std::string code = R"(
            int32 sum_all<args...>(int32 first, args...) {
                int32 total = first;
                for (int32 i = 0; i < args.length; i++) {
                    total = total + (int32)args[i];
                }
                return total;
            }

            int32 main() {
                int32 res = sum_all(10, 20, 30, 40);
                return res == 100 ? 0 : 1;
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 55.6: Automatic Call-Site Argument Bundling") {
        const std::string code = R"(
            int32 count_args<args...>(int32 label, args...) {
                return args.length;
            }

            int32 main() {
                if (count_args(0) != 0) return 1;
                if (count_args(1, 100) != 1) return 2;
                if (count_args(3, 1, 'b', true) != 3) return 3;
                return 0;
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 55.7: Destructor Trigger on Tuple Scope Exit") {
        const std::string code = R"(
            class Tracker {
                public static int32 destroyed_count = 0;
                public ~Tracker() {
                    Tracker.destroyed_count = Tracker.destroyed_count + 1;
                }
            }

            void test_scope() {
                tuple t = new tuple(new Tracker(), new Tracker());
            }

            int32 main() {
                Tracker.destroyed_count = 0;
                test_scope();
                return Tracker.destroyed_count == 2 ? 0 : 1;
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 55.8: Destructor Trigger on Tuple Reassignment") {
        const std::string code = R"(
            class Tracker {
                public static int32 destroyed_count = 0;
                public ~Tracker() {
                    Tracker.destroyed_count = Tracker.destroyed_count + 1;
                }
            }

            int32 main() {
                Tracker.destroyed_count = 0;
                tuple t = new tuple(new Tracker());
                if (Tracker.destroyed_count != 0) return 1;
                t = null;
                if (Tracker.destroyed_count != 1) return 2;
                return 0;
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 55.9: Exception Stack Unwinding RAII with tuple") {
        const std::string code = R"(
            class Exception {}

            class Resource {
                public static int32 released = 0;
                public ~Resource() {
                    Resource.released = Resource.released + 1;
                }
            }

            void risky() {
                tuple t = new tuple(new Resource());
                throw new Exception();
            }

            int32 main() {
                Resource.released = 0;
                try {
                    risky();
                } catch (Exception e) {
                    if (Resource.released != 1) return 2;
                    return 0;
                }
                return 1;
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 55.10: Nested Tuples Recursive Destruction") {
        const std::string code = R"(
            class Leaf {
                public static int32 count = 0;
                public ~Leaf() {
                    Leaf.count = Leaf.count + 1;
                }
            }

            void test_nested() {
                tuple inner = new tuple(new Leaf());
                tuple outer = new tuple(inner);
            }

            int32 main() {
                Leaf.count = 0;
                test_nested();
                return Leaf.count == 1 ? 0 : 1;
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 55.11: Invalid Type Cast Throws TypeCastException") {
        const std::string code = R"(
            int32 main() {
                tuple t = new tuple("not an int");
                int32 x = (int32)t[0];
                return 0;
            }
        )";
        assert_compile_success(code);
        CHECK_THROWS_WITH(run_and_evaluate_int(code), Catch::Matchers::ContainsSubstring("TypeCastException"));
    }

    SECTION("Case 55.12: Out of Bounds Index Access Throws IndexOutOfBoundsException") {
        const std::string code = R"(
            int32 main() {
                tuple t = new tuple(1, 2);
                int32 x = (int32)t[5];
                return 0;
            }
        )";
        assert_compile_success(code);
        CHECK_THROWS_WITH(run_and_evaluate_int(code), Catch::Matchers::ContainsSubstring("IndexOutOfBoundsException"));
    }

    SECTION("Case 55.13: Variadic Parameter Not Trailing") {
        const std::string code = R"(
            void invalid<args...>(args..., int32 extra) {}
        )";
        assert_compile_error(code, "Variadic parameter must be the last parameter in method signature");
    }

    SECTION("Case 55.14: Multiple Variadic Packs in Same Signature") {
        const std::string code = R"(
            void invalid<args..., others...>(args..., others...) {}
        )";
        assert_compile_error(code, "Variadic template parameter must be the last parameter in template parameter list");
    }

    SECTION("Case 55.15: Incompatible Class Cast on Tuple Element Throws TypeCastException") {
        const std::string code = R"(
            class Animal {}
            class Dog extends Animal {}
            class Cat extends Animal {}

            int32 main() {
                tuple t = new tuple(new Dog());
                Cat c = (Cat)t[0];
                return 0;
            }
        )";
        assert_compile_success(code);
        CHECK_THROWS_WITH(run_and_evaluate_int(code), Catch::Matchers::ContainsSubstring("TypeCastException"));
    }
}
