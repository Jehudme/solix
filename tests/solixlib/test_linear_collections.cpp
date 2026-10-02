#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.collections.Linear", "[solixlib][collections][linear]") {
    SECTION("Case 11.1: Stack LIFO operations") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.Stack;

            class Main {
                public static int32 main() {
                    Stack<int32> stack = new Stack<int32>();
                    if (!stack.is_empty()) return 1;
                    if (stack.size() != 0) return 2;

                    stack.push(10);
                    stack.push(20);
                    stack.push(30);

                    if (stack.size() != 3) return 3;
                    if (stack.is_empty()) return 4;

                    int32 top = stack.peek();
                    if (top != 30) return 5;

                    // for_each check
                    int32[] sum_box = new int32[1];
                    sum_box[0] = 0;
                    stack.for_each([sum_box](int32 x) : void {
                        sum_box[0] = sum_box[0] + x;
                    });
                    if (sum_box[0] != 60) return 50;

                    int32 p1 = stack.pop();
                    if (p1 != 30) return 6;
                    if (stack.size() != 2) return 600 + stack.size();

                    int32 p2 = stack.pop();
                    if (p2 != 20) return p2;

                    int32 p3 = stack.pop();
                    if (p3 != 10) return 8;

                    if (!stack.is_empty()) return 9;
                    if (stack.size() != 0) return 10;

                    stack.push(99);
                    stack.clear();
                    if (stack.size() != 0) return 11;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 11.2: Queue FIFO operations") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.Queue;

            class Main {
                public static int32 main() {
                    Queue<int32> q = new Queue<int32>();
                    if (!q.is_empty()) return 1;
                    if (q.size() != 0) return 2;

                    q.enqueue(100);
                    q.enqueue(200);
                    q.enqueue(300);

                    if (q.size() != 3) return 3;
                    if (q.peek() != 100) return 4;

                    // for_each check
                    int32[] sum_box = new int32[1];
                    sum_box[0] = 0;
                    q.for_each([sum_box](int32 x) : void {
                        sum_box[0] = sum_box[0] + x;
                    });
                    if (sum_box[0] != 600) return 40;

                    int32 d1 = q.dequeue();
                    if (d1 != 100) return 5;

                    int32 d2 = q.dequeue();
                    if (d2 != 200) return 6;

                    int32 d3 = q.dequeue();
                    if (d3 != 300) return 7;

                    if (!q.is_empty()) return 8;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 11.3: Deque double-ended operations") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.Deque;

            class Main {
                public static int32 main() {
                    Deque<int32> dq = new Deque<int32>();
                    dq.push_front(20);
                    dq.push_front(10);
                    dq.push_back(30);
                    dq.push_back(40);

                    // Order: 10, 20, 30, 40
                    if (dq.size() != 4) return 1;
                    if (dq.peek_front() != 10) return 2;
                    if (dq.peek_back() != 40) return 3;

                    // for_each check
                    int32[] sum_box = new int32[1];
                    sum_box[0] = 0;
                    dq.for_each([sum_box](int32 x) : void {
                        sum_box[0] = sum_box[0] + x;
                    });
                    if (sum_box[0] != 100) return 30;

                    if (dq.pop_front() != 10) return 4;
                    if (dq.pop_back() != 40) return 5;
                    if (dq.pop_front() != 20) return 6;
                    if (dq.pop_back() != 30) return 7;

                    if (!dq.is_empty()) return 8;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 11.4: PriorityQueue min-heap and max-heap prioritization") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.PriorityQueue;

            class Main {
                public static int32 main() {
                    // Min-heap test (comparator: a - b < 0 => a has higher priority)
                    PriorityQueue<int32> min_pq = new PriorityQueue<int32>([](int32 a, int32 b) => a - b);
                    min_pq.enqueue(40);
                    min_pq.enqueue(10);
                    min_pq.enqueue(30);
                    min_pq.enqueue(20);
                    min_pq.enqueue(50);

                    if (min_pq.size() != 5) return 1;
                    if (min_pq.peek() != 10) return 2;

                    // for_each check
                    int32[] sum_box = new int32[1];
                    sum_box[0] = 0;
                    min_pq.for_each([sum_box](int32 x) : void {
                        sum_box[0] = sum_box[0] + x;
                    });
                    if (sum_box[0] != 150) return 20;

                    if (min_pq.dequeue() != 10) return 3;
                    if (min_pq.dequeue() != 20) return 4;
                    if (min_pq.dequeue() != 30) return 5;
                    if (min_pq.dequeue() != 40) return 6;
                    if (min_pq.dequeue() != 50) return 7;
                    if (!min_pq.is_empty()) return 8;

                    // Max-heap test (comparator: b - a < 0 => b < a => a has higher priority)
                    PriorityQueue<int32> max_pq = new PriorityQueue<int32>([](int32 a, int32 b) => b - a);
                    max_pq.enqueue(10);
                    max_pq.enqueue(50);
                    max_pq.enqueue(30);
                    max_pq.enqueue(20);
                    max_pq.enqueue(40);

                    if (max_pq.peek() != 50) return 9;
                    if (max_pq.dequeue() != 50) return 10;
                    if (max_pq.dequeue() != 40) return 11;
                    if (max_pq.dequeue() != 30) return 12;
                    if (max_pq.dequeue() != 20) return 13;
                    if (max_pq.dequeue() != 10) return 14;
                    if (!max_pq.is_empty()) return 15;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 11.5: CircularBuffer ring buffer FIFO semantics and wrap-around") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.CircularBuffer;

            class Main {
                public static int32 main() {
                    // Non-overwrite mode
                    CircularBuffer<int32> cb = new CircularBuffer<int32>(3, false);
                    if (cb.capacity() != 3) return 1;
                    if (!cb.is_empty()) return 2;

                    if (!cb.write(1)) return 3;
                    if (!cb.write(2)) return 4;
                    if (!cb.write(3)) return 5;
                    if (!cb.is_full()) return 6;

                    // Overwrite disallowed
                    if (cb.write(4)) return 7;

                    // for_each check
                    int32[] sum_box = new int32[1];
                    sum_box[0] = 0;
                    cb.for_each([sum_box](int32 x) : void {
                        sum_box[0] = sum_box[0] + x;
                    });
                    if (sum_box[0] != 6) return 70;

                    if (cb.read() != 1) return 8;
                    // Now 1 slot free
                    if (!cb.write(4)) return 9;

                    if (cb.read() != 2) return 10;
                    if (cb.read() != 3) return 11;
                    if (cb.read() != 4) return 12;
                    if (!cb.is_empty()) return 13;

                    // Overwrite mode
                    CircularBuffer<int32> ow = new CircularBuffer<int32>(2, true);
                    ow.write(10);
                    ow.write(20);
                    // Overwrites 10
                    ow.write(30);

                    if (ow.size() != 2) return 14;
                    if (ow.read() != 20) return 15;
                    if (ow.read() != 30) return 16;
                    if (!ow.is_empty()) return 17;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 11.6: BitSet bit manipulation") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.BitSet;

            class Main {
                public static int32 main() {
                    BitSet bs = new BitSet();
                    if (bs.cardinality() != 0) return 1;
                    if (!bs.is_empty()) return 2;

                    bs.set(0);
                    bs.set(5);
                    bs.set(63);
                    bs.set(100);

                    if (bs.cardinality() != 4) return 3;
                    if (!bs.get(0)) return 4;
                    if (bs.get(1)) return 5;
                    if (!bs.get(5)) return 6;
                    if (!bs.get(63)) return 7;
                    if (!bs.get(100)) return 8;
                    if (bs.length() != 101) return 9;

                    bs.flip(5);
                    if (bs.get(5)) return 10;
                    if (bs.cardinality() != 3) return 11;

                    // Bitwise AND
                    BitSet a = new BitSet();
                    a.set(1);
                    a.set(2);
                    BitSet b = new BitSet();
                    b.set(2);
                    b.set(3);
                    a.and(b);
                    if (a.cardinality() != 1) return 12;
                    if (!a.get(2)) return 13;

                    // Bitwise OR
                    BitSet c = new BitSet();
                    c.set(1);
                    BitSet d = new BitSet();
                    d.set(2);
                    c.or(d);
                    if (c.cardinality() != 2) return 14;
                    if (!c.get(1) || !c.get(2)) return 15;

                    // Bitwise XOR
                    c.xor(d);
                    if (c.cardinality() != 1) return 16;
                    if (!c.get(1)) return 17;
                    if (c.get(2)) return 18;

                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 11.7: Negative: Stack empty pop and peek") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.Stack;
            import solix.exceptions.InvalidOperationException;

            class Main {
                public static int32 main() {
                    Stack<int32> s = new Stack<int32>();
                    bool pop_caught = false;
                    try {
                        s.pop();
                    } catch (InvalidOperationException ex) {
                        pop_caught = true;
                    }

                    bool peek_caught = false;
                    try {
                        s.peek();
                    } catch (InvalidOperationException ex) {
                        peek_caught = true;
                    }

                    if (!pop_caught || !peek_caught) return 1;
                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 11.8: Negative: Queue empty dequeue and peek") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.Queue;
            import solix.exceptions.InvalidOperationException;

            class Main {
                public static int32 main() {
                    Queue<int32> q = new Queue<int32>();
                    bool dequeue_caught = false;
                    try {
                        q.dequeue();
                    } catch (InvalidOperationException ex) {
                        dequeue_caught = true;
                    }

                    bool peek_caught = false;
                    try {
                        q.peek();
                    } catch (InvalidOperationException ex) {
                        peek_caught = true;
                    }

                    if (!dequeue_caught || !peek_caught) return 1;
                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 11.9: Negative: CircularBuffer empty read and full enqueue") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.CircularBuffer;
            import solix.exceptions.InvalidOperationException;

            class Main {
                public static int32 main() {
                    CircularBuffer<int32> cb = new CircularBuffer<int32>(1, false);
                    bool read_caught = false;
                    try {
                        cb.read();
                    } catch (InvalidOperationException ex) {
                        read_caught = true;
                    }

                    cb.enqueue(42);
                    bool enqueue_caught = false;
                    try {
                        cb.enqueue(43);
                    } catch (InvalidOperationException ex) {
                        enqueue_caught = true;
                    }

                    if (!read_caught || !enqueue_caught) return 1;
                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }

    SECTION("Case 11.10: Negative: BitSet negative index and null arguments") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.BitSet;
            import solix.exceptions.IndexOutOfBoundsException;
            import solix.exceptions.IllegalArgumentException;

            class Main {
                public static int32 main() {
                    BitSet bs = new BitSet();
                    bool index_caught = false;
                    try {
                        bs.set(-1);
                    } catch (IndexOutOfBoundsException ex) {
                        index_caught = true;
                    }

                    bool get_caught = false;
                    try {
                        bs.get(-5);
                    } catch (IndexOutOfBoundsException ex) {
                        get_caught = true;
                    }

                    bool null_caught = false;
                    try {
                        bs.and(null);
                    } catch (IllegalArgumentException ex) {
                        null_caught = true;
                    }

                    if (!index_caught || !get_caught || !null_caught) return 1;
                    return 0;
                }
            }
        )";

        REQUIRE(run_solixlib_sources(sources) == 0);
    }
}
