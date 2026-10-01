#include "test_helper.hpp"

using namespace solix::test;

TEST_CASE("AliasStatement - Modules", "[modules][alias]") {
    SECTION("Case 3.1: Primitive Synonym") {
        const std::string code = R"(
            alias Byte = uint8;

            class Main {
                public static int32 main() {
                    Byte b = 255;
                    return 0;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 3.2: Parameterized Generic Alias") {
        const std::string code = R"(
            class Map<K, V> {
                public Map() {}
            }
            alias IntMap<V> = Map<int32, V>;

            class Main {
                public static int32 main() {
                    IntMap<int32> map = new IntMap<int32>();
                    return 0;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 4.1: Circular Alias Definition") {
        const std::string code = R"(
            alias A = B;
            alias B = A;
        )";
        assert_compile_error(code, "Circular alias detected");
    }

    SECTION("Case 4.2: Generic Parameter Arity Mismatch") {
        const std::string code = R"(
            class Map<K, V> {}
            alias Pair<K, V> = Map<K, V>;

            class Main {
                public static int32 main() {
                    Pair<int32> bad;
                    return 0;
                }
            }
        )";
        assert_compile_error(code);
    }

    SECTION("Case 3.3: Class and Qualified Type Alias") {
        std::unordered_map<std::string, std::string> sources;
        sources["helper.slx"] = R"(
            package my.pkg;

            public class Helper {
                public int32 val;
                public Helper(int32 v) { this.val = v; }
                public int32 getVal() { return this.val; }
            }
        )";
        sources["main.slx"] = R"(
            alias MyHelper = my.pkg.Helper;

            class Main {
                public static int32 main() {
                    MyHelper h = new MyHelper(42);
                    return h.getVal() == 42 ? 0 : 1;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_sources(sources) == 0);
    }

    SECTION("Case 3.4: Alias in Method Signature") {
        const std::string code = R"(
            alias ID = int64;

            class User {
                public ID id;
                public ID getId() { return this.id; }
            }

            class Main {
                public static int32 main() {
                    User u = new User();
                    u.id = 42;
                    return u.getId() == 42 ? 0 : 1;
                }
            }
        )";
        assert_compile_success(code);
        REQUIRE(run_and_evaluate_int(code) == 0);
    }

    SECTION("Case 4.3: Aliasing Undeclared Type") {
        const std::string code = R"(
            alias Bad = NonExistentType;
        )";
        assert_compile_error(code, "Cannot resolve alias target 'NonExistentType'");
    }

    SECTION("Case 4.4: Duplicate Alias in Same Scope") {
        const std::string code = R"(
            alias Value = int32;
            alias Value = float64;
        )";
        assert_compile_error(code, "Duplicate declaration of alias 'Value'");
    }
}

