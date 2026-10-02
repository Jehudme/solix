#include "solixlib_test_helper.hpp"

using namespace solix::test;

TEST_CASE("Standard Library - solix.collections.Linear", "[solixlib][collections][linear]") {
    SECTION("Case 11.1: Stack LIFO operations") {
        auto sources = load_solixlib_sources();
        sources["main.slx"] = R"(
            import solix.collections.Stack;
            import solix.core.Any;

            class Main {
                public static int32 main() {
                    Stack stack = new Stack();
                    if (!stack.is_empty()) return 1;
                    if (stack.size() != 0) return 2;

                    Any a1 = new Any(10);
                    Any a2 = new Any(20);
                    Any a3 = new Any(30);
                    stack.push(a1);
                    stack.push(a2);
                    stack.push(a3);

                    if (stack.size() != 3) return 3;
                    if (stack.is_empty()) return 4;

                    Any top = stack.peek();
                    if (top.as_int32() != 30) return 5;

                    Any p1 = stack.pop();
                    if (p1.as_int32() != 30) return 6;
                    if (stack.size() != 2) return 600 + stack.size();

                    Any p2 = stack.pop();
                    if (p2.as_int32() != 20) return p2.as_int32();

                    Any p3 = stack.pop();
                    if (p3.as_int32() != 10) return 8;

                    if (!stack.is_empty()) return 9;
                    if (stack.size() != 0) return 10;

                    stack.push(new Any(99));
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
            import solix.core.Any;

            class Main {
                public static int32 main() {
                    Queue q = new Queue();
                    if (!q.is_empty()) return 1;
                    if (q.size() != 0) return 2;

                    q.enqueue(new Any(100));
                    q.enqueue(new Any(200));
                    q.enqueue(new Any(300));

                    if (q.size() != 3) return 3;
                    if (q.peek().as_int32() != 100) return 4;

                    Any d1 = q.dequeue();
                    if (d1.as_int32() != 100) return 5;

                    Any d2 = q.dequeue();
                    if (d2.as_int32() != 200) return 6;

                    Any d3 = q.dequeue();
                    if (d3.as_int32() != 300) return 7;

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
            import solix.core.Any;

            class Main {
                public static int32 main() {
                    Deque dq = new Deque();
                    dq.push_front(new Any(20));
                    dq.push_front(new Any(10));
                    dq.push_back(new Any(30));
                    dq.push_back(new Any(40));

                    // Order: 10, 20, 30, 40
                    if (dq.size() != 4) return 1;
                    if (dq.peek_front().as_int32() != 10) return 2;
                    if (dq.peek_back().as_int32() != 40) return 3;

                    if (dq.pop_front().as_int32() != 10) return 4;
                    if (dq.pop_back().as_int32() != 40) return 5;
                    if (dq.pop_front().as_int32() != 20) return 6;
                    if (dq.pop_back().as_int32() != 30) return 7;

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
            import solix.core.Any;

            class Main {
                public static int32 main() {
                    // Min-heap test
                    PriorityQueue min_pq = new PriorityQueue();
                    Any m1 = new Any(40);
                    Any m2 = new Any(10);
                    Any m3 = new Any(30);
                    Any m4 = new Any(20);
                    Any m5 = new Any(50);
                    min_pq.enqueue(m1);
                    min_pq.enqueue(m2);
                    min_pq.enqueue(m3);
                    min_pq.enqueue(m4);
                    min_pq.enqueue(m5);

                    if (min_pq.size() != 5) return 1;
                    if (min_pq.peek().as_int32() != 10) return 2;

                    if (min_pq.dequeue().as_int32() != 10) return 3;
                    if (min_pq.dequeue().as_int32() != 20) return 4;
                    if (min_pq.dequeue().as_int32() != 30) return 5;
                    if (min_pq.dequeue().as_int32() != 40) return 6;
                    if (min_pq.dequeue().as_int32() != 50) return 7;
                    if (!min_pq.is_empty()) return 8;

                    // Max-heap test
                    PriorityQueue max_pq = new PriorityQueue(true);
                    Any x1 = new Any(10);
                    Any x2 = new Any(50);
                    Any x3 = new Any(30);
                    Any x4 = new Any(20);
                    Any x5 = new Any(40);
                    max_pq.enqueue(x1);
                    max_pq.enqueue(x2);
                    max_pq.enqueue(x3);
                    max_pq.enqueue(x4);
                    max_pq.enqueue(x5);

                    if (max_pq.peek().as_int32() != 50) return 9;
                    if (max_pq.dequeue().as_int32() != 50) return 10;
                    if (max_pq.dequeue().as_int32() != 40) return 11;
                    if (max_pq.dequeue().as_int32() != 30) return 12;
                    if (max_pq.dequeue().as_int32() != 20) return 13;
                    if (max_pq.dequeue().as_int32() != 10) return 14;
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
            import solix.core.Any;

            class Main {
                public static int32 main() {
                    // Non-overwrite mode
                    CircularBuffer cb = new CircularBuffer(3, false);
                    if (cb.capacity() != 3) return 1;
                    if (!cb.is_empty()) return 2;

                    Any c1 = new Any(1);
                    Any c2 = new Any(2);
                    Any c3 = new Any(3);
                    Any c4 = new Any(4);

                    if (!cb.write(c1)) return 3;
                    if (!cb.write(c2)) return 4;
                    if (!cb.write(c3)) return 5;
                    if (!cb.is_full()) return 6;

                    // Overwrite disallowed
                    if (cb.write(c4)) return 7;

                    if (cb.read().as_int32() != 1) return 8;
                    // Now 1 slot free
                    if (!cb.write(c4)) return 9;

                    if (cb.read().as_int32() != 2) return 10;
                    if (cb.read().as_int32() != 3) return 11;
                    if (cb.read().as_int32() != 4) return 12;
                    if (!cb.is_empty()) return 13;

                    // Overwrite mode
                    CircularBuffer ow = new CircularBuffer(2, true);
                    Any o1 = new Any(10);
                    Any o2 = new Any(20);
                    Any o3 = new Any(30);
                    ow.write(o1);
                    ow.write(o2);
                    // Overwrites 10
                    ow.write(o3);

                    if (ow.size() != 2) return 14;
                    if (ow.read().as_int32() != 20) return 15;
                    if (ow.read().as_int32() != 30) return 16;
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
                    Stack s = new Stack();
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
                    Queue q = new Queue();
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
            import solix.core.Any;
            import solix.exceptions.InvalidOperationException;

            class Main {
                public static int32 main() {
                    CircularBuffer cb = new CircularBuffer(1, false);
                    bool read_caught = false;
                    try {
                        cb.read();
                    } catch (InvalidOperationException ex) {
                        read_caught = true;
                    }

                    cb.enqueue(new Any(42));
                    bool enqueue_caught = false;
                    try {
                        cb.enqueue(new Any(43));
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
