#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.collections.List", "[solixlib][collections][list]") {
    SECTION("Case 8.1: List dynamic resizing, indexing, iteration, and filter") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.List;
            import solix.collections.ListIterator;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    List<int32> list = new List<int32>(4);
                    if (list.capacity() != 4) return 1;
                    if (list.size() != 0) return 2;
                    if (!list.is_empty()) return 3;

                    // Add items to trigger capacity expansion
                    list.add(10);
                    list.add(20);
                    list.add(30);
                    list.add(40);
                    list.add(50);

                    if (list.size() != 5) return 4;
                    if (list.capacity() < 5) return 5;
                    if (list.is_empty()) return 6;

                    // get & set
                    if (list.get(0) != 10) return 7;
                    if (list.get(4) != 50) return 8;

                    int32 old = list.set(2, 99);
                    if (old != 30) return 9;
                    if (list.get(2) != 99) return 10;

                    // for_each lambda check
                    int32[] sum_box = new int32[1];
                    sum_box[0] = 0;
                    list.for_each([sum_box](int32 item) : void {
                        sum_box[0] = sum_box[0] + item;
                    });
                    // 10 + 20 + 99 + 40 + 50 = 219
                    if (sum_box[0] != 219) return 11;

                    // filter lambda check
                    List<int32> evens = list.filter([](int32 item) => item % 2 == 0);
                    if (evens.size() != 4) return 12;
                    if (evens.get(0) != 10) return 13;
                    if (evens.get(1) != 20) return 14;
                    if (evens.get(2) != 40) return 15;
                    if (evens.get(3) != 50) return 16;

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
            import solix.core.String;

            class Main {
                public static int32 main() {
                    List<int32> list = new List<int32>();
                    list.add(1);
                    list.add(2);
                    list.add(3);

                    // Insert at index 1: [1, 99, 2, 3]
                    list.insert(1, 99);
                    if (list.size() != 4) return 1;
                    if (list.get(1) != 99) return 2;
                    if (list.index_of(99) != 1) return 3;
                    if (!list.contains(99)) return 4;

                    // remove_at(1) -> [1, 2, 3]
                    int32 removed = list.remove_at(1);
                    if (removed != 99) return 5;
                    if (list.size() != 3) return 6;

                    // remove(item)
                    bool remOk = list.remove(2);
                    if (!remOk) return 7;
                    if (list.size() != 2) return 8;
                    // list is [1, 3]
                    if (list.get(0) != 1) return 9;
                    if (list.get(1) != 3) return 10;

                    // sub_list
                    list.add(4);
                    list.add(5);
                    // list is [1, 3, 4, 5]
                    List<int32> sub = list.sub_list(1, 2); // [3, 4]
                    if (sub.size() != 2) return 11;
                    if (sub.get(0) != 3) return 12;
                    if (sub.get(1) != 4) return 13;

                    // reverse
                    list.reverse(); // [5, 4, 3, 1]
                    if (list.get(0) != 5) return 14;
                    if (list.get(3) != 1) return 15;

                    String repr = list.to_string();
                    if (!repr.equals_chars("[List]")) return 16;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 8.3: LinkedList double-ended queue operations, for_each, and to_string") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.LinkedList;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    LinkedList<int32> deque = new LinkedList<int32>();
                    if (deque.size() != 0) return 1;
                    if (!deque.is_empty()) return 2;

                    // Add: head: 5 -> 10 -> 20 -> 30 :tail
                    deque.add_last(10);
                    deque.add_last(20);
                    deque.add_first(5);
                    deque.add_last(30);

                    if (deque.size() != 4) return 3;
                    if (deque.peek_first() != 5) return 4;
                    if (deque.peek_last() != 30) return 5;

                    String s = deque.to_string();
                    if (!s.equals_chars("[LinkedList]")) return 6;

                    // for_each lambda check
                    int32[] sum_box = new int32[1];
                    sum_box[0] = 0;
                    deque.for_each([sum_box](int32 item) : void {
                        sum_box[0] = sum_box[0] + item;
                    });
                    // 5 + 10 + 20 + 30 = 65
                    if (sum_box[0] != 65) return 7;

                    // Remove ends
                    int32 rf = deque.remove_first();
                    if (rf != 5) return 8;
                    if (deque.size() != 3) return 9;

                    int32 rl = deque.remove_last();
                    if (rl != 30) return 10;
                    if (deque.size() != 2) return 11;

                    // Remaining: [10, 20]
                    if (deque.peek_first() != 10) return 12;
                    if (deque.peek_last() != 20) return 13;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 8.4: LinkedList indexed access, mutation, and filter") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.LinkedList;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    LinkedList<int32> list = new LinkedList<int32>();
                    list.add(100);
                    list.add(200);
                    list.add(300);

                    // get & set
                    if (list.get(0) != 100) return 1;
                    if (list.get(1) != 200) return 2;
                    if (list.get(2) != 300) return 3;

                    int32 old = list.set(1, 250);
                    if (old != 200) return 4;
                    if (list.get(1) != 250) return 5;

                    // insert
                    list.insert(1, 150);
                    // Now: [100, 150, 250, 300]
                    if (list.size() != 4) return 6;
                    if (list.get(1) != 150) return 7;
                    if (list.get(2) != 250) return 8;

                    // remove_at(2) -> removes 250 -> [100, 150, 300]
                    int32 remAt = list.remove_at(2);
                    if (remAt != 250) return 9;
                    if (list.size() != 3) return 10;

                    // remove(150) -> [100, 300]
                    bool remOk = list.remove(150);
                    if (!remOk) return 11;
                    if (list.size() != 2) return 12;

                    String s = list.to_string();
                    if (!s.equals_chars("[LinkedList]")) return 13;

                    // filter
                    LinkedList<int32> filtered = list.filter([](int32 val) => val > 150);
                    if (filtered.size() != 1) return 14;
                    if (filtered.get(0) != 300) return 15;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 8.5: Algorithms (reverse, swap, fill, sort, binary_search)") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.List;
            import solix.collections.Algorithms;
            import solix.core.String;

            class Main {
                public static int32 main() {
                    List<int32> list = new List<int32>();
                    list.add(40);
                    list.add(10);
                    list.add(50);
                    list.add(20);
                    list.add(30);

                    // Swap index 0 and 2
                    Algorithms.swap<int32>(list, 0, 2);
                    if (list.get(0) != 50) return 1;
                    if (list.get(2) != 40) return 2;

                    // Sort using comparator lambda
                    Algorithms.sort<int32>(list, [](int32 a, int32 b) => a - b);
                    // Now: [10, 20, 30, 40, 50]
                    if (list.get(0) != 10) return 3;
                    if (list.get(1) != 20) return 4;
                    if (list.get(2) != 30) return 5;
                    if (list.get(3) != 40) return 6;
                    if (list.get(4) != 50) return 7;

                    // Binary search
                    int32 idx30 = Algorithms.binary_search<int32>(list, 30, [](int32 a, int32 b) => a - b);
                    if (idx30 != 2) return 8;

                    int32 idx10 = Algorithms.binary_search<int32>(list, 10, [](int32 a, int32 b) => a - b);
                    if (idx10 != 0) return 9;

                    int32 idx50 = Algorithms.binary_search<int32>(list, 50, [](int32 a, int32 b) => a - b);
                    if (idx50 != 4) return 10;

                    int32 idxNone = Algorithms.binary_search<int32>(list, 999, [](int32 a, int32 b) => a - b);
                    if (idxNone != -1) return 11;

                    // Fill
                    Algorithms.fill<int32>(list, 7);
                    if (list.get(0) != 7) return 12;
                    if (list.get(4) != 7) return 13;

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

            class Main {
                public static int32 main() {
                    int32 caught = 0;

                    // Negative capacity throws IllegalArgumentException
                    try {
                        List<int32> badList = new List<int32>(-5);
                    } catch (IllegalArgumentException ex) {
                        caught = caught + 1;
                    }

                    List<int32> list = new List<int32>();
                    list.add(1);

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

            class Main {
                public static int32 main() {
                    int32 caught = 0;
                    LinkedList<int32> list = new LinkedList<int32>();

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

    SECTION("Case 8.8: List array initializer and collection copy constructors") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.List;

            class Main {
                public static int32 main() {
                    int32[] arr = new int32[3];
                    arr[0] = 10;
                    arr[1] = 20;
                    arr[2] = 30;

                    List<int32> list = new List<int32>(arr);
                    if (list.size() != 3) return 1;
                    if (list.get(0) != 10 || list.get(1) != 20 || list.get(2) != 30) return 2;

                    List<int32> copy = new List<int32>(list);
                    if (copy.size() != 3) return 3;
                    if (copy.get(0) != 10 || copy.get(2) != 30) return 4;

                    copy.set(0, 99);
                    if (list.get(0) != 10) return 5;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 8.9: Functional combinators map and reduce on List") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.List;

            class Main {
                public static int32 main() {
                    List<int32> list = new List<int32>();
                    list.add(1);
                    list.add(2);
                    list.add(3);
                    list.add(4);

                    List<int32> doubled = list.map<int32>([](int32 x) => x * 2);
                    if (doubled.size() != 4) return 1;
                    if (doubled.get(0) != 2 || doubled.get(3) != 8) return 2;

                    int32 sum = list.reduce([](int32 acc, int32 x) => acc + x);
                    if (sum != 10) return 3;

                    int32 sumInit = list.reduce(100, [](int32 acc, int32 x) => acc + x);
                    if (sumInit != 110) return 4;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 8.10: Container ecosystem array and copy constructors") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.LinkedList;
            import solix.collections.HashSet;
            import solix.collections.TreeSet;
            import solix.collections.Stack;
            import solix.collections.Queue;
            import solix.collections.Deque;

            class Main {
                public static int32 main() {
                    int32[] arr = new int32[3];
                    arr[0] = 1;
                    arr[1] = 2;
                    arr[2] = 3;

                    LinkedList<int32> ll = new LinkedList<int32>(arr);
                    if (ll.size() != 3 || ll.get(0) != 1 || ll.get(2) != 3) return 1;

                    HashSet<int32> hs = new HashSet<int32>(arr);
                    if (hs.size() != 3 || !hs.contains(2)) return 2;

                    TreeSet<int32> ts = new TreeSet<int32>(arr);
                    if (ts.size() != 3 || !ts.contains(3)) return 3;

                    Stack<int32> st = new Stack<int32>(arr);
                    if (st.size() != 3 || st.peek() != 3) return 4;

                    Queue<int32> q = new Queue<int32>(arr);
                    if (q.size() != 3 || q.peek() != 1) return 5;

                    Deque<int32> dq = new Deque<int32>(arr);
                    if (dq.size() != 3 || dq.peek_first() != 1 || dq.peek_last() != 3) return 6;

                    LinkedList<int32> llCopy = new LinkedList<int32>(ll);
                    if (llCopy.size() != 3) return 7;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }
}
