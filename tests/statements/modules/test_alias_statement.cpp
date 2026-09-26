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
}
