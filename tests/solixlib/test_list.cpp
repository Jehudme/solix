#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.collections.List", "[solixlib][collections][list]") {
    SECTION("Case 8.1: List dynamic resizing, indexing, and iteration") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.List;
            import solix.collections.IIterator;
            import solix.core.Any;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    List list = new List(4);
                    if (list.capacity() != 4) return 1;
                    if (list.size() != 0) return 2;
                    if (!list.is_empty()) return 3;

                    // Add items to trigger capacity expansion
                    Any a1 = new Any(10);
                    Any a2 = new Any(20);
                    Any a3 = new Any(30);
                    Any a4 = new Any(40);
                    Any a5 = new Any(50);
                    list.add(a1);
                    list.add(a2);
                    list.add(a3);
                    list.add(a4);
                    list.add(a5);

                    if (list.size() != 5) return 4;
                    if (list.capacity() < 5) return 5;
                    if (list.is_empty()) return 6;

                    // get & set
                    if (list.get(0).as_int32() != 10) return 7;
                    if (list.get(4).as_int32() != 50) return 8;

                    Any a99 = new Any(99);
                    Any old = list.set(2, a99);
                    if (old.as_int32() != 30) return 9;
                    if (list.get(2).as_int32() != 99) return 10;

                    // IIterator check
                    IIterator it = list.iterator();
                    int32 sum = 0;
                    while (it.has_next()) {
                        Any item = it.next();
                        sum = sum + item.as_int32();
                    }
                    // 10 + 20 + 99 + 40 + 50 = 219
                    if (sum != 219) return 11;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 8.2: List mutation (insert, remove, sub_list, reverse, to_string)") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.List;
            import solix.core.Any;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    List list = new List();
                    Any a1 = new Any(1);
                    Any a2 = new Any(2);
                    Any a3 = new Any(3);
                    list.add(a1);
                    list.add(a2);
                    list.add(a3);

                    // Insert at index 1: [1, 99, 2, 3]
                    Any a99 = new Any(99);
                    list.insert(1, a99);
                    if (list.size() != 4) return 1;
                    if (list.get(1).as_int32() != 99) return 2;
                    if (list.index_of(a99) != 1) return 3;
                    if (!list.contains(a99)) return 4;

                    // remove_at(1) -> [1, 2, 3]
                    Any removed = list.remove_at(1);
                    if (removed.as_int32() != 99) return 5;
                    if (list.size() != 3) return 6;

                    // remove(item)
                    bool remOk = list.remove(a2);
                    if (!remOk) return 7;
                    if (list.size() != 2) return 8;
                    // list is [1, 3]
                    if (list.get(0).as_int32() != 1) return 9;
                    if (list.get(1).as_int32() != 3) return 10;

                    // sub_list
                    Any a4 = new Any(4);
                    Any a5 = new Any(5);
                    list.add(a4);
                    list.add(a5);
                    // list is [1, 3, 4, 5]
                    List sub = list.sub_list(1, 2); // [3, 4]
                    if (sub.size() != 2) return 11;
                    if (sub.get(0).as_int32() != 3) return 12;
                    if (sub.get(1).as_int32() != 4) return 13;

                    // reverse
                    list.reverse(); // [5, 4, 3, 1]
                    if (list.get(0).as_int32() != 5) return 14;
                    if (list.get(3).as_int32() != 1) return 15;

                    String repr = list.to_string();
                    if (!repr.equals_chars("[5, 4, 3, 1]")) return 16;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 8.3: LinkedList double-ended queue operations and to_string") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.LinkedList;
            import solix.core.Any;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    LinkedList deque = new LinkedList();
                    if (deque.size() != 0) return 1;
                    if (!deque.is_empty()) return 2;

                    Any a10 = new Any(10);
                    Any a20 = new Any(20);
                    Any a5 = new Any(5);
                    Any a30 = new Any(30);

                    // Add: head: 5 -> 10 -> 20 -> 30 :tail
                    deque.add_last(a10);
                    deque.add_last(a20);
                    deque.add_first(a5);
                    deque.add_last(a30);

                    if (deque.size() != 4) return 3;
                    if (deque.peek_first().as_int32() != 5) return 4;
                    if (deque.peek_last().as_int32() != 30) return 5;

                    String s = deque.to_string();
                    if (!s.equals_chars("[5, 10, 20, 30]")) return 6;

                    // Remove ends
                    Any rf = deque.remove_first();
                    if (rf.as_int32() != 5) return 7;
                    if (deque.size() != 3) return 8;

                    Any rl = deque.remove_last();
                    if (rl.as_int32() != 30) return 9;
                    if (deque.size() != 2) return 10;

                    // Remaining: [10, 20]
                    if (deque.peek_first().as_int32() != 10) return 11;
                    if (deque.peek_last().as_int32() != 20) return 12;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 8.4: LinkedList indexed access, mutation, and removal") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.LinkedList;
            import solix.core.Any;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    LinkedList list = new LinkedList();
                    Any a1 = new Any(100);
                    Any a2 = new Any(200);
                    Any a3 = new Any(300);
                    list.add(a1);
                    list.add(a2);
                    list.add(a3);

                    // get & set
                    if (list.get(0).as_int32() != 100) return 1;
                    if (list.get(1).as_int32() != 200) return 2;
                    if (list.get(2).as_int32() != 300) return 3;

                    Any a250 = new Any(250);
                    Any old = list.set(1, a250);
                    if (old.as_int32() != 200) return 4;
                    if (list.get(1).as_int32() != 250) return 5;

                    // insert
                    Any a150 = new Any(150);
                    list.insert(1, a150);
                    // Now: [100, 150, 250, 300]
                    if (list.size() != 4) return 6;
                    if (list.get(1).as_int32() != 150) return 7;
                    if (list.get(2).as_int32() != 250) return 8;

                    // remove_at(2) -> removes 250 -> [100, 150, 300]
                    Any remAt = list.remove_at(2);
                    if (remAt.as_int32() != 250) return 9;
                    if (list.size() != 3) return 10;

                    // remove(a150) -> [100, 300]
                    bool remOk = list.remove(a150);
                    if (!remOk) return 11;
                    if (list.size() != 2) return 12;

                    String s = list.to_string();
                    if (!s.equals_chars("[100, 300]")) return 13;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 8.5: Algorithms (reverse, swap, fill, sort_int32, binary_search)") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.List;
            import solix.collections.Algorithms;
            import solix.core.Any;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    List list = new List();
                    Any a40 = new Any(40);
                    Any a10 = new Any(10);
                    Any a50 = new Any(50);
                    Any a20 = new Any(20);
                    Any a30 = new Any(30);
                    list.add(a40);
                    list.add(a10);
                    list.add(a50);
                    list.add(a20);
                    list.add(a30);

                    // Swap index 0 and 2
                    Algorithms.swap(list, 0, 2);
                    if (list.get(0).as_int32() != 50) return 1;
                    if (list.get(2).as_int32() != 40) return 2;

                    // Sort
                    Algorithms.sort_int32(list);
                    // Now: [10, 20, 30, 40, 50]
                    if (list.get(0).as_int32() != 10) return 3;
                    if (list.get(1).as_int32() != 20) return 4;
                    if (list.get(2).as_int32() != 30) return 5;
                    if (list.get(3).as_int32() != 40) return 6;
                    if (list.get(4).as_int32() != 50) return 7;

                    // Binary search
                    int32 idx30 = Algorithms.binary_search_int32(list, 30);
                    if (idx30 != 2) return 8;

                    int32 idx10 = Algorithms.binary_search_int32(list, 10);
                    if (idx10 != 0) return 9;

                    int32 idx50 = Algorithms.binary_search_int32(list, 50);
                    if (idx50 != 4) return 10;

                    int32 idxNone = Algorithms.binary_search_int32(list, 999);
                    if (idxNone != -1) return 11;

                    // Fill
                    Any a7 = new Any(7);
                    Algorithms.fill(list, a7);
                    if (list.get(0).as_int32() != 7) return 12;
                    if (list.get(4).as_int32() != 7) return 13;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 8.6: Negative: List index out of bounds and invalid capacity throw exceptions") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.List;
            import solix.exceptions.IndexOutOfBoundsException;
            import solix.exceptions.IllegalArgumentException;
            import solix.core.Any;

            class Main {
                public static int32 main() {
                    int32 caught = 0;

                    // Negative capacity throws IllegalArgumentException
                    try {
                        List badList = new List(-5);
                    } catch (IllegalArgumentException ex) {
                        caught = caught + 1;
                    }

                    List list = new List();
                    list.add(new Any(1));

                    // Negative index throws IndexOutOfBoundsException
                    try {
                        list.get(-1);
                    } catch (IndexOutOfBoundsException ex) {
                        caught = caught + 1;
                    }

                    // Index >= size throws IndexOutOfBoundsException
                    try {
                        list.get(1);
                    } catch (IndexOutOfBoundsException ex) {
                        caught = caught + 1;
                    }

                    return caught == 3 ? 0 : 1;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 8.7: Negative: LinkedList empty access and out of bounds throw exceptions") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.LinkedList;
            import solix.exceptions.IndexOutOfBoundsException;
            import solix.exceptions.NoSuchElementException;
            import solix.core.Any;

            class Main {
                public static int32 main() {
                    int32 caught = 0;
                    LinkedList list = new LinkedList();

                    // remove_first on empty throws NoSuchElementException
                    try {
                        list.remove_first();
                    } catch (NoSuchElementException ex) {
                        caught = caught + 1;
                    }

                    // remove_last on empty throws NoSuchElementException
                    try {
                        list.remove_last();
                    } catch (NoSuchElementException ex) {
                        caught = caught + 1;
                    }

                    // peek_first on empty throws NoSuchElementException
                    try {
                        list.peek_first();
                    } catch (NoSuchElementException ex) {
                        caught = caught + 1;
                    }

                    // get(0) on empty throws IndexOutOfBoundsException
                    try {
                        list.get(0);
                    } catch (IndexOutOfBoundsException ex) {
                        caught = caught + 1;
                    }

                    return caught == 4 ? 0 : 1;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }
}
