#include "test_helper.hpp"
#include <catch2/catch_all.hpp>

using namespace solix::test;

TEST_CASE("GenericsAndTemplates - Declarations", "[declarations][generics]") {

    SECTION("Case 48.1: Single-Parameter Generic Class (Box<T>)") {
        const std::string code = R"(
            public class Box<T> {
                public T val;
                public Box(T v) { this.val = v; }
                public T get() { return this.val; }
                public void set(T v) { this.val = v; }
            }

            public class Main {
                public static int32 main() {
                    Box<int32> ib = new Box<int32>(42);
                    ib.set(100);
                    return ib.get() == 100 ? 0 : 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 48.2: Multi-Parameter Generic Class (Pair<K, V>)") {
        const std::string code = R"(
            public class Pair<K, V> {
                public K key;
                public V val;
                public Pair(K k, V v) {
                    this.key = k;
                    this.val = v;
                }
            }

            public class Main {
                public static int32 main() {
                    Pair<int32, bool> p = new Pair<int32, bool>(10, true);
                    if (p.key == 10 && p.val == true) {
                        return 0;
                    }
                    return 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 48.3: Generic Class Implementing Interface (interface IContainer)") {
        const std::string code = R"(
            public interface IContainer {
                int32 getVal();
            }

            public class Holder<T> implements IContainer {
                public T item;
                public Holder(T item) { this.item = item; }
                public int32 getVal() { return 99; }
            }

            public class Main {
                public static int32 main() {
                    IContainer container = new Holder<int32>(42);
                    return container.getVal() == 99 ? 0 : 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 48.4: Generic Method with Explicit Type Arguments") {
        const std::string code = R"(
            public class Utils {
                public static T convert<T>(T val) {
                    return val;
                }
            }

            public class Main {
                public static int32 main() {
                    int32 v = Utils.convert<int32>(55);
                    return v == 55 ? 0 : 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 48.5: Generic Method with Implicit Template Argument Deduction") {
        const std::string code = R"(
            public class Deduce {
                public static T identity<T>(T item) {
                    return item;
                }
            }

            public class Main {
                public static int32 main() {
                    int32 res = Deduce.identity(123);
                    return res == 123 ? 0 : 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 48.6: Generic Method with Deductions from Multiple Arguments") {
        const std::string code = R"(
            public class DeduceMulti {
                public static T selectFirst<T>(T a, T b) {
                    return a;
                }
            }

            public class Main {
                public static int32 main() {
                    int32 chosen = DeduceMulti.selectFirst(77, 88);
                    return chosen == 77 ? 0 : 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 48.7: Nested Generic Types") {
        const std::string code = R"(
            public class Cell<T> {
                public T content;
                public Cell(T c) { this.content = c; }
            }

            public class Main {
                public static int32 main() {
                    Cell<Cell<int32>> nested = new Cell<Cell<int32>>(new Cell<int32>(777));
                    return nested.content.content == 777 ? 0 : 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 48.8: Generic Class with Function Pointer Fields / Lambdas") {
        const std::string code = R"(
            public class Processor<T> {
                public T(*)(T) transform;
                public Processor(T(*)(T) fn) {
                    this.transform = fn;
                }
                public T execute(T input) {
                    return this.transform(input);
                }
            }

            public class Main {
                public static int32 main() {
                    Processor<int32> p = new Processor<int32>([](int32 x) => x * 2);
                    return p.execute(21) == 42 ? 0 : 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 48.9: Generic Type Argument Count Arity Mismatch (Compile-Time Error)") {
        const std::string code = R"(
            public class Map<K, V> {
                public K key;
                public V val;
            }

            public class Main {
                public static void test() {
                    Map<int32> m = null;
                }
            }
        )";
        assert_compile_error(code, "expects 2 arguments");
    }

    SECTION("Case 48.10: Incompatible Type Assignment Between Specialized Generic Instances (Compile-Time Error)") {
        const std::string code = R"(
            public class Item<T> {
                public T value;
            }

            public class Main {
                public static void test() {
                    Item<int32> a = new Item<bool>();
                }
            }
        )";
        assert_compile_error(code, "Type mismatch");
    }

    SECTION("Case 48.11: Conflicting Template Deduction at Call Site (Compile-Time Error)") {
        const std::string code = R"(
            public class Matcher {
                public static T choose<T>(T a, T b) { return a; }
            }

            public class Main {
                public static void test() {
                    Matcher.choose(10, true);
                }
            }
        )";
        assert_compile_error(code, "No matching method");
    }

    SECTION("Case 48.12: Unbound Type Parameter Identifier (Compile-Time Error)") {
        const std::string code = R"(
            public class BadGeneric {
                public static void doSomething() {
                    T invalidVar = null;
                }
            }
        )";
        assert_compile_error(code, "Unknown type");
    }
}
