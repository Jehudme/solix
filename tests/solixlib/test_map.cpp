#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.collections.Map", "[solixlib][collections][map]") {
    SECTION("Case 9.1: HashMap put, get, update, contains, and remove") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.HashMap;

            class Main {
                public static int32 main() {
                    HashMap<int32, int32> map = new HashMap<int32, int32>();
                    if (map.size() != 0) return 1;
                    if (!map.is_empty()) return 2;

                    map.put(1, 100);
                    map.put(2, 200);

                    if (map.size() != 2) return 3;
                    if (map.is_empty()) return 4;
                    if (!map.contains_key(1)) return 5;
                    if (!map.contains_key(2)) return 6;
                    if (!map.contains_value(100)) return 7;
                    if (!map.contains_value(200)) return 8;

                    if (map.contains_key(99)) return 9;

                    if (map.get(1) != 100) return 10;
                    if (map.get(2) != 200) return 11;

                    // Update existing key
                    map.put(1, 150);
                    if (map.size() != 2) return 12;
                    if (map.get(1) != 150) return 13;

                    // get_or_default
                    int32 gotDef = map.get_or_default(99, 999);
                    if (gotDef != 999) return 14;

                    // remove
                    int32 rem = map.remove(1);
                    if (rem != 150) return 15;
                    if (map.size() != 1) return 16;
                    if (map.contains_key(1)) return 17;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 9.2: HashMap automatic rehashing across multi-key insertions") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.HashMap;

            class Main {
                public static int32 main() {
                    // Start with small capacity 4, using identity hasher lambda
                    HashMap<int32, int32> map = new HashMap<int32, int32>(4, [](int32 k) => k);

                    // Insert 20 elements
                    for (int32 i = 0; i < 20; i = i + 1) {
                        map.put(i, i * 10);
                    }

                    if (map.size() != 20) return 1;

                    // Verify all elements are intact
                    for (int32 i = 0; i < 20; i = i + 1) {
                        if (!map.contains_key(i)) return 2;
                        if (map.get(i) != i * 10) return 3;
                    }

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 9.3: HashMap keys(), values(), entries(), for_each(), and to_string()") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.HashMap;
            import solix.collections.KeyValuePair;
            import solix.collections.List;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    HashMap<int32, int32> map = new HashMap<int32, int32>();
                    map.put(10, 100);

                    List<int32> kList = map.keys();
                    if (kList.size() != 1) return 1;
                    if (kList.get(0) != 10) return 2;

                    List<int32> vList = map.values();
                    if (vList.size() != 1) return 3;
                    if (vList.get(0) != 100) return 4;

                    List<KeyValuePair<int32, int32>> entries = map.entries();
                    if (entries.size() != 1) return 5;
                    KeyValuePair<int32, int32> entry = entries.get(0);
                    if (entry.get_key() != 10) return 6;
                    if (entry.get_value() != 100) return 7;

                    // Test for_each lambda iteration
                    int32[] sum_box = new int32[1];
                    sum_box[0] = 0;
                    map.put(20, 200);
                    map.for_each([sum_box](int32 k, int32 v) : void {
                        sum_box[0] = sum_box[0] + k + v;
                    });
                    if (sum_box[0] != (10 + 100 + 20 + 200)) return 8;

                    String repr = map.to_string();
                    if (!repr.equals_chars("[HashMap]")) return 9;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 9.4: TreeMap sorted order, first_key, last_key, and keys() traversal") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.TreeMap;
            import solix.collections.List;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    TreeMap<int32, int32> tree = new TreeMap<int32, int32>([](int32 a, int32 b) => a - b);
                    if (tree.size() != 0) return 1;
                    if (!tree.is_empty()) return 2;

                    // Insert in unsorted order: 50, 20, 80, 10, 30
                    tree.put(50, 500);
                    tree.put(20, 200);
                    tree.put(80, 800);
                    tree.put(10, 100);
                    tree.put(30, 300);

                    if (tree.size() != 5) return 3;
                    if (tree.first_key() != 10) return 4;
                    if (tree.last_key() != 80) return 5;

                    // In-order keys must be strictly sorted: 10, 20, 30, 50, 80
                    List<int32> keys = tree.keys();
                    if (keys.size() != 5) return 6;
                    if (keys.get(0) != 10) return 7;
                    if (keys.get(1) != 20) return 8;
                    if (keys.get(2) != 30) return 9;
                    if (keys.get(3) != 50) return 10;
                    if (keys.get(4) != 80) return 11;

                    String repr = tree.to_string();
                    if (!repr.equals_chars("[TreeMap]")) return 12;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 9.5: TreeMap remove, clear, for_each, and lookups") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.TreeMap;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    TreeMap<int32, int32> tree = new TreeMap<int32, int32>([](int32 a, int32 b) => a - b);
                    tree.put(2, 20);
                    tree.put(1, 10);
                    tree.put(3, 30);

                    // for_each check
                    int32[] sum_box = new int32[1];
                    sum_box[0] = 0;
                    tree.for_each([sum_box](int32 k, int32 v) : void {
                        sum_box[0] = sum_box[0] + k + v;
                    });
                    if (sum_box[0] != (1 + 10 + 2 + 20 + 3 + 30)) return 90;

                    // Remove leaf node 3
                    int32 r3 = tree.remove(3);
                    if (r3 != 30) return 1;
                    if (tree.size() != 2) return 2;
                    if (tree.contains_key(3)) return 3;

                    // Remove root node 2
                    int32 r2 = tree.remove(2);
                    if (r2 != 20) return 4;
                    if (tree.size() != 1) return 5;
                    if (tree.get(1) != 10) return 6;

                    // Clear
                    tree.clear();
                    if (!tree.is_empty()) return 7;
                    if (tree.size() != 0) return 8;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 9.6: Negative: HashMap get missing key throws KeyNotFoundException") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.HashMap;
            import solix.exceptions.KeyNotFoundException;

            class Main {
                public static int32 main() {
                    HashMap<int32, int32> map = new HashMap<int32, int32>();
                    map.put(1, 10);

                    int32 caught = 0;
                    try {
                        map.get(99);
                    } catch (KeyNotFoundException ex) {
                        caught = 1;
                    }

                    return caught == 1 ? 0 : 1;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 9.7: Negative: TreeMap missing key and empty first_key throw exceptions") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.TreeMap;
            import solix.exceptions.KeyNotFoundException;
            import solix.exceptions.NoSuchElementException;

            class Main {
                public static int32 main() {
                    int32 caught = 0;
                    TreeMap<int32, int32> tree = new TreeMap<int32, int32>([](int32 a, int32 b) => a - b);

                    try {
                        tree.first_key();
                    } catch (NoSuchElementException ex) {
                        caught = caught + 1;
                    }

                    try {
                        tree.get(10);
                    } catch (KeyNotFoundException ex) {
                        caught = caught + 1;
                    }

                    return caught == 2 ? 0 : 1;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 9.8: Generic associative map interface polymorphism (IMap<K, V>)") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.IMap;
            import solix.collections.HashMap;
            import solix.collections.TreeMap;

            class Main {
                private static int32 test_map_contract(IMap<int32, int32> map) {
                    map.put(1, 100);
                    map.put(2, 200);
                    map.put(3, 300);

                    if (map.size() != 3) return 1;
                    if (!map.contains_key(2)) return 2;
                    if (map.get(2) != 200) return 3;

                    int32 removed = map.remove(2);
                    if (removed != 200) return 4;
                    if (map.size() != 2) return 5;
                    if (map.contains_key(2)) return 6;

                    map.clear();
                    if (!map.is_empty()) return 7;
                    if (map.size() != 0) return 8;

                    return 0;
                }

                public static int32 main() {
                    // Test polymorphic dispatch to HashMap
                    HashMap<int32, int32> hm = new HashMap<int32, int32>();
                    int32 res1 = test_map_contract(hm);
                    if (res1 != 0) return res1;

                    // Test polymorphic dispatch to TreeMap
                    TreeMap<int32, int32> tm = new TreeMap<int32, int32>([](int32 a, int32 b) => a - b);
                    int32 res2 = test_map_contract(tm);
                    if (res2 != 0) return 10 + res2;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }
}
