#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.collections.Set", "[solixlib][collections][set]") {
    SECTION("Case 10.1: HashSet distinct insertion, duplicate rejection, contains, remove, and clear") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.HashSet;
            import solix.core.Any;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    HashSet set = new HashSet();
                    if (set.size() != 0) return 1;
                    if (!set.is_empty()) return 2;

                    Any a = new Any(10);
                    Any b = new Any(20);
                    Any c = new Any(30);

                    // First insertion returns true
                    if (!set.add(a)) return 3;
                    if (!set.add(b)) return 4;
                    if (set.size() != 2) return 5;

                    // Duplicate insertion returns false
                    if (set.add(a)) return 6;
                    if (set.size() != 2) return 7;

                    if (!set.contains(a)) return 8;
                    if (!set.contains(b)) return 9;
                    if (set.contains(c)) return 10;

                    // Add third element
                    if (!set.add(c)) return 11;
                    if (set.size() != 3) return 12;

                    // Remove
                    if (!set.remove(b)) return 13;
                    if (set.size() != 2) return 14;
                    if (set.contains(b)) return 15;
                    // Removing missing element returns false
                    if (set.remove(b)) return 16;

                    // Clear
                    set.clear();
                    if (set.size() != 0) return 17;
                    if (!set.is_empty()) return 18;
                    if (set.contains(a)) return 19;

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
            import solix.core.Any;

            class Main {
                public static int32 main() {
                    Any one = new Any(1);
                    Any two = new Any(2);
                    Any three = new Any(3);
                    Any four = new Any(4);

                    // 1. Union: {1, 2} union {2, 3} -> {1, 2, 3}
                    HashSet s1 = new HashSet();
                    s1.add(one);
                    s1.add(two);

                    HashSet s2 = new HashSet();
                    s2.add(two);
                    s2.add(three);

                    s1.union_with(s2);
                    if (s1.size() != 3) return 1;
                    if (!s1.contains(one) || !s1.contains(two) || !s1.contains(three)) return 2;

                    // 2. Intersection: {1, 2, 3} intersect {2, 3, 4} -> {2, 3}
                    HashSet s3 = new HashSet();
                    s3.add(two);
                    s3.add(three);
                    s3.add(four);

                    s1.intersect_with(s3);
                    if (s1.size() != 2) return 3;
                    if (s1.contains(one)) return 4;
                    if (!s1.contains(two) || !s1.contains(three)) return 5;

                    // 3. Difference: {2, 3} diff {3, 4} -> {2}
                    HashSet s4 = new HashSet();
                    s4.add(three);
                    s4.add(four);

                    s1.difference_with(s4);
                    if (s1.size() != 1) return 6;
                    if (!s1.contains(two)) return 7;
                    if (s1.contains(three)) return 8;

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
            import solix.core.Any;

            class Main {
                public static int32 main() {
                    Any a = new Any(10);
                    Any b = new Any(20);
                    Any c = new Any(30);

                    HashSet sub = new HashSet();
                    sub.add(a);
                    sub.add(b);

                    HashSet sup = new HashSet();
                    sup.add(a);
                    sup.add(b);
                    sup.add(c);

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
                    HashSet empty_set = new HashSet();
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
            import solix.core.Any;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    TreeSet tree = new TreeSet();
                    if (tree.size() != 0) return 1;

                    Any v50 = new Any(50);
                    Any v10 = new Any(10);
                    Any v30 = new Any(30);
                    Any v20 = new Any(20);
                    Any v40 = new Any(40);

                    // Insert in random order
                    tree.add(v50);
                    tree.add(v10);
                    tree.add(v30);
                    tree.add(v20);
                    tree.add(v40);

                    if (tree.size() != 5) return 2;

                    // Check boundaries
                    if (tree.first().as_int32() != 10) return 3;
                    if (tree.last().as_int32() != 50) return 4;

                    // In-order iteration must produce 10, 20, 30, 40, 50
                    IIterator it = tree.iterator();
                    int32 expected = 10;
                    while (it.has_next()) {
                        Any item = it.next();
                        if (item.as_int32() != expected) return 5;
                        expected = expected + 10;
                    }
                    if (expected != 60) return 6;

                    // Verify canonical formatted string
                    String str = tree.to_string();
                    if (!str.equals_chars("{10, 20, 30, 40, 50}")) return 7;

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
            import solix.core.Any;

            class Main {
                public static int32 main() {
                    TreeSet tree = new TreeSet();
                    Any v1 = new Any(1);
                    Any v2 = new Any(2);
                    Any v3 = new Any(3);

                    if (!tree.add(v2)) return 1;
                    if (!tree.add(v1)) return 2;
                    if (!tree.add(v3)) return 3;

                    // Duplicate rejection
                    if (tree.add(v2)) return 4;
                    if (tree.size() != 3) return 5;

                    // Remove min element
                    if (!tree.remove(v1)) return 6;
                    if (tree.size() != 2) return 7;
                    if (tree.first().as_int32() != 2) return 8;

                    // Union with another set
                    TreeSet tree2 = new TreeSet();
                    Any v4 = new Any(4);
                    Any v0 = new Any(0);
                    tree2.add(v4);
                    tree2.add(v0);

                    tree.union_with(tree2);
                    if (tree.size() != 4) return 9;
                    if (tree.first().as_int32() != 0) return 10;
                    if (tree.last().as_int32() != 4) return 11;

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
            import solix.core.Any;

            class Main {
                public static int32 main() {
                    TreeSet empty_tree = new TreeSet();
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
}
