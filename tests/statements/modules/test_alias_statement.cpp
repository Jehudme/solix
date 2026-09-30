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
        const std::string code = R"(
            alias Text = solix.core.String;

            class Main {
                public static int32 main() {
                    Text t = new Text("hello");
                    return t.length() == 5 ? 0 : 1;
                }
            }
        )";
        assert_compile_success(code, true);
        REQUIRE(run_and_evaluate_int(code, true) == 0);
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

