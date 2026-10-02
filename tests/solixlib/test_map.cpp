#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.collections.Map", "[solixlib][collections][map]") {
    SECTION("Case 9.1: HashMap put, get, update, contains, and remove") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.HashMap;
            import solix.core.Any;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    HashMap map = new HashMap();
                    if (map.size() != 0) return 1;
                    if (!map.is_empty()) return 2;

                    Any k1 = new Any(1);
                    Any v1 = new Any(100);
                    Any k2 = new Any(2);
                    Any v2 = new Any(200);

                    map.put(k1, v1);
                    map.put(k2, v2);

                    if (map.size() != 2) return 3;
                    if (map.is_empty()) return 4;
                    if (!map.contains_key(k1)) return 5;
                    if (!map.contains_key(k2)) return 6;
                    if (!map.contains_value(v1)) return 7;
                    if (!map.contains_value(v2)) return 8;

                    Any k99 = new Any(99);
                    if (map.contains_key(k99)) return 9;

                    if (map.get(k1).as_int32() != 100) return 10;
                    if (map.get(k2).as_int32() != 200) return 11;

                    // Update existing key
                    Any v1_new = new Any(150);
                    map.put(k1, v1_new);
                    if (map.size() != 2) return 12;
                    if (map.get(k1).as_int32() != 150) return 13;

                    // get_or_default
                    Any defVal = new Any(999);
                    Any gotDef = map.get_or_default(k99, defVal);
                    if (gotDef.as_int32() != 999) return 14;

                    // remove
                    Any rem = map.remove(k1);
                    if (rem.as_int32() != 150) return 15;
                    if (map.size() != 1) return 16;
                    if (map.contains_key(k1)) return 17;

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
            import solix.core.Any;

            class Main {
                public static int32 main() {
                    // Start with small capacity 4
                    HashMap map = new HashMap(4);

                    // Insert 20 elements
                    for (int32 i = 0; i < 20; i = i + 1) {
                        Any k = new Any(i);
                        Any v = new Any(i * 10);
                        map.put(k, v);
                    }

                    if (map.size() != 20) return 1;

                    // Verify all elements are intact
                    for (int32 i = 0; i < 20; i = i + 1) {
                        Any k = new Any(i);
                        if (!map.contains_key(k)) return 2;
                        if (map.get(k).as_int32() != i * 10) return 3;
                    }

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 9.3: HashMap keys(), values(), entries(), and to_string()") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.HashMap;
            import solix.collections.List;
            import solix.core.Any;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    HashMap map = new HashMap();
                    Any k1 = new Any(10);
                    Any v1 = new Any(100);
                    map.put(k1, v1);

                    List kList = map.keys();
                    if (kList.size() != 1) return 1;
                    if (kList.get(0).as_int32() != 10) return 2;

                    List vList = map.values();
                    if (vList.size() != 1) return 3;
                    if (vList.get(0).as_int32() != 100) return 4;

                    List entries = map.entries();
                    if (entries.size() != 1) return 5;

                    String repr = map.to_string();
                    if (!repr.equals_chars("{10: 100}")) return 6;

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
            import solix.core.Any;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    TreeMap tree = new TreeMap();
                    if (tree.size() != 0) return 1;
                    if (!tree.is_empty()) return 2;

                    // Insert in unsorted order: 50, 20, 80, 10, 30
                    Any k50 = new Any(50); Any v50 = new Any(500);
                    Any k20 = new Any(20); Any v20 = new Any(200);
                    Any k80 = new Any(80); Any v80 = new Any(800);
                    Any k10 = new Any(10); Any v10 = new Any(100);
                    Any k30 = new Any(30); Any v30 = new Any(300);

                    tree.put(k50, v50);
                    tree.put(k20, v20);
                    tree.put(k80, v80);
                    tree.put(k10, v10);
                    tree.put(k30, v30);

                    if (tree.size() != 5) return 3;
                    if (tree.first_key().as_int32() != 10) return 4;
                    if (tree.last_key().as_int32() != 80) return 5;

                    // In-order keys must be strictly sorted: 10, 20, 30, 50, 80
                    List keys = tree.keys();
                    if (keys.size() != 5) return 6;
                    if (keys.get(0).as_int32() != 10) return 7;
                    if (keys.get(1).as_int32() != 20) return 8;
                    if (keys.get(2).as_int32() != 30) return 9;
                    if (keys.get(3).as_int32() != 50) return 10;
                    if (keys.get(4).as_int32() != 80) return 11;

                    String repr = tree.to_string();
                    if (!repr.equals_chars("{10: 100, 20: 200, 30: 300, 50: 500, 80: 800}")) return 12;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 9.5: TreeMap remove, clear, and lookups") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.TreeMap;
            import solix.core.Any;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    TreeMap tree = new TreeMap();
                    Any k1 = new Any(1); Any v1 = new Any(10);
                    Any k2 = new Any(2); Any v2 = new Any(20);
                    Any k3 = new Any(3); Any v3 = new Any(30);

                    tree.put(k2, v2);
                    tree.put(k1, v1);
                    tree.put(k3, v3);

                    // Remove leaf node k3
                    Any r3 = tree.remove(k3);
                    if (r3.as_int32() != 30) return 1;
                    if (tree.size() != 2) return 2;
                    if (tree.contains_key(k3)) return 3;

                    // Remove root node k2
                    Any r2 = tree.remove(k2);
                    if (r2.as_int32() != 20) return 4;
                    if (tree.size() != 1) return 5;
                    if (tree.get(k1).as_int32() != 10) return 6;

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
            import solix.core.Any;

            class Main {
                public static int32 main() {
                    HashMap map = new HashMap();
                    map.put(new Any(1), new Any(10));

                    int32 caught = 0;
                    try {
                        map.get(new Any(99));
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
            import solix.core.Any;

            class Main {
                public static int32 main() {
                    int32 caught = 0;
                    TreeMap tree = new TreeMap();

                    try {
                        tree.first_key();
                    } catch (NoSuchElementException ex) {
                        caught = caught + 1;
                    }

                    try {
                        tree.get(new Any(10));
                    } catch (KeyNotFoundException ex) {
                        caught = caught + 1;
                    }

                    return caught == 2 ? 0 : 1;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }
}
