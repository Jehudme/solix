#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.collections.Set", "[solixlib][collections][set]") {
    SECTION("Case 10.1: HashSet distinct insertion, duplicate rejection, contains, remove, clear, and for_each") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.HashSet;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    HashSet<int32> set = new HashSet<int32>();
                    if (set.size() != 0) return 1;
                    if (!set.is_empty()) return 2;

                    // First insertion returns true
                    if (!set.add(10)) return 3;
                    if (!set.add(20)) return 4;
                    if (set.size() != 2) return 5;

                    // Duplicate insertion returns false
                    if (set.add(10)) return 6;
                    if (set.size() != 2) return 7;

                    if (!set.contains(10)) return 8;
                    if (!set.contains(20)) return 9;
                    if (set.contains(30)) return 10;

                    // Add third element
                    if (!set.add(30)) return 11;
                    if (set.size() != 3) return 12;

                    // for_each check
                    int32[] sum_box = new int32[1];
                    sum_box[0] = 0;
                    set.for_each([sum_box](int32 x) : void {
                        sum_box[0] = sum_box[0] + x;
                    });
                    if (sum_box[0] != 60) return 120;

                    // Remove
                    if (!set.remove(20)) return 13;
                    if (set.size() != 2) return 14;
                    if (set.contains(20)) return 15;
                    // Removing missing element returns false
                    if (set.remove(20)) return 16;

                    // Clear
                    set.clear();
                    if (set.size() != 0) return 17;
                    if (!set.is_empty()) return 18;
                    if (set.contains(10)) return 19;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 10.2: HashSet set algebra: union_with, intersect_with, difference_with") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.HashSet;

            class Main {
                public static int32 main() {
                    // 1. Union: {1, 2} union {2, 3} -> {1, 2, 3}
                    HashSet<int32> s1 = new HashSet<int32>();
                    s1.add(1);
                    s1.add(2);

                    HashSet<int32> s2 = new HashSet<int32>();
                    s2.add(2);
                    s2.add(3);

                    s1.union_with(s2);
                    if (s1.size() != 3) return 1;
                    if (!s1.contains(1) || !s1.contains(2) || !s1.contains(3)) return 2;

                    // 2. Intersection: {1, 2, 3} intersect {2, 3, 4} -> {2, 3}
                    HashSet<int32> s3 = new HashSet<int32>();
                    s3.add(2);
                    s3.add(3);
                    s3.add(4);

                    s1.intersect_with(s3);
                    if (s1.size() != 2) return 3;
                    if (s1.contains(1)) return 4;
                    if (!s1.contains(2) || !s1.contains(3)) return 5;

                    // 3. Difference: {2, 3} diff {3, 4} -> {2}
                    HashSet<int32> s4 = new HashSet<int32>();
                    s4.add(3);
                    s4.add(4);

                    s1.difference_with(s4);
                    if (s1.size() != 1) return 6;
                    if (!s1.contains(2)) return 7;
                    if (s1.contains(3)) return 8;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 10.3: HashSet subset and superset relationships") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.HashSet;

            class Main {
                public static int32 main() {
                    HashSet<int32> sub = new HashSet<int32>();
                    sub.add(10);
                    sub.add(20);

                    HashSet<int32> sup = new HashSet<int32>();
                    sup.add(10);
                    sup.add(20);
                    sup.add(30);

                    // sub is subset of sup
                    if (!sub.is_subset_of(sup)) return 1;
                    if (sup.is_subset_of(sub)) return 2;

                    // sup is superset of sub
                    if (!sup.is_superset_of(sub)) return 3;
                    if (sub.is_superset_of(sup)) return 4;

                    // A set is subset and superset of itself
                    if (!sub.is_subset_of(sub)) return 5;
                    if (!sub.is_superset_of(sub)) return 6;

                    // Empty set is subset of everything
                    HashSet<int32> empty_set = new HashSet<int32>();
                    if (!empty_set.is_subset_of(sub)) return 7;
                    if (sub.is_subset_of(empty_set)) return 8;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 10.4: TreeSet ordered uniqueness, boundary lookups, iteration, and to_string") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.TreeSet;
            import solix.collections.IIterator;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    TreeSet<int32> tree = new TreeSet<int32>([](int32 a, int32 b) => a - b);
                    if (tree.size() != 0) return 1;

                    // Insert in random order
                    tree.add(50);
                    tree.add(10);
                    tree.add(30);
                    tree.add(20);
                    tree.add(40);

                    if (tree.size() != 5) return 2;

                    // Check boundaries
                    if (tree.first() != 10) return 3;
                    if (tree.last() != 50) return 4;

                    // In-order iteration must produce 10, 20, 30, 40, 50
                    IIterator<int32> it = tree.iterator();
                    int32 expected = 10;
                    while (it.has_next()) {
                        int32 item = it.next();
                        if (item != expected) return 5;
                        expected = expected + 10;
                    }
                    if (expected != 60) return 6;

                    // for_each check
                    int32[] sum_box = new int32[1];
                    sum_box[0] = 0;
                    tree.for_each([sum_box](int32 x) : void {
                        sum_box[0] = sum_box[0] + x;
                    });
                    if (sum_box[0] != 150) return 60;

                    // Verify canonical formatted string
                    String str = tree.to_string();
                    if (!str.equals_chars("[TreeSet]")) return 7;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 10.5: TreeSet node removal, duplicate rejections, and union_with") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.TreeSet;

            class Main {
                public static int32 main() {
                    TreeSet<int32> tree = new TreeSet<int32>([](int32 a, int32 b) => a - b);

                    if (!tree.add(2)) return 1;
                    if (!tree.add(1)) return 2;
                    if (!tree.add(3)) return 3;

                    // Duplicate rejection
                    if (tree.add(2)) return 4;
                    if (tree.size() != 3) return 5;

                    // Remove min element
                    if (!tree.remove(1)) return 6;
                    if (tree.size() != 2) return 7;
                    if (tree.first() != 2) return 8;

                    // Union with another set
                    TreeSet<int32> tree2 = new TreeSet<int32>([](int32 a, int32 b) => a - b);
                    tree2.add(4);
                    tree2.add(0);

                    tree.union_with(tree2);
                    if (tree.size() != 4) return 9;
                    if (tree.first() != 0) return 10;
                    if (tree.last() != 4) return 11;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 10.6: Negative: TreeSet first() and last() on empty set throw NoSuchElementException") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.TreeSet;
            import solix.exceptions.NoSuchElementException;

            class Main {
                public static int32 main() {
                    TreeSet<int32> empty_tree = new TreeSet<int32>([](int32 a, int32 b) => a - b);
                    bool caught_first = false;
                    try {
                        empty_tree.first();
                    } catch (NoSuchElementException e) {
                        caught_first = true;
                    }
                    if (!caught_first) return 1;

                    bool caught_last = false;
                    try {
                        empty_tree.last();
                    } catch (NoSuchElementException e) {
                        caught_last = true;
                    }
                    if (!caught_last) return 2;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 10.7: Generic set interface polymorphism (ISet<T>)") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.ISet;
            import solix.collections.HashSet;
            import solix.collections.TreeSet;

            class Main {
                private static int32 test_set_contract(ISet<int32> set) {
                    set.add(10);
                    set.add(20);
                    set.add(30);

                    if (set.size() != 3) return 1;
                    if (!set.contains(20)) return 2;

                    bool added = set.add(20);
                    if (added) return 3;

                    bool removed = set.remove(20);
                    if (!removed) return 4;
                    if (set.size() != 2) return 5;
                    if (set.contains(20)) return 6;

                    set.clear();
                    if (!set.is_empty()) return 7;
                    if (set.size() != 0) return 8;

                    return 0;
                }

                public static int32 main() {
                    // Test polymorphic dispatch to HashSet
                    HashSet<int32> hs = new HashSet<int32>();
                    int32 res1 = test_set_contract(hs);
                    if (res1 != 0) return res1;

                    // Test polymorphic dispatch to TreeSet
                    TreeSet<int32> ts = new TreeSet<int32>([](int32 a, int32 b) => a - b);
                    int32 res2 = test_set_contract(ts);
                    if (res2 != 0) return 10 + res2;

                    // Test cross-set algebra: HashSet union_with TreeSet via ISet<T>
                    HashSet<int32> s1 = new HashSet<int32>();
                    s1.add(1);
                    s1.add(2);

                    TreeSet<int32> s2 = new TreeSet<int32>([](int32 a, int32 b) => a - b);
                    s2.add(2);
                    s2.add(3);

                    s1.union_with(s2);
                    if (s1.size() != 3) return 20;
                    if (!s1.contains(1) || !s1.contains(2) || !s1.contains(3)) return 21;

                    return 0;
                }
            }
        )";
        assert_compile_sources_success(sources);
        REQUIRE(run_solixlib_sources(sources) == 0);
    }
}
